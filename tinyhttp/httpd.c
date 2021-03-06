/* J. David's webserver */
/* This is a simple webserver.
 * Created November 1999 by J. David Blackstone.
 * CSE 4344 (Network concepts), Prof. Zeigler
 * University of Texas at Arlington
 */
/* This program compiles for Sparc Solaris 2.6.
 * To compile for Linux:
 *  1) Comment out the #include <pthread.h> line.
 *  2) Comment out the line that defines the variable newthread.
 *  3) Comment out the two lines that run pthread_create().
 *  4) Uncomment the line that runs accept_request().
 *  5) Remove -lsocket from the Makefile.
 */
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>

//宏定义
#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"

/**
在linux下搭建教程：
   https://blog.csdn.net/yzhang6_10/article/details/51533890?_t_t_t=0.9113585329573218

    工作的具体过程：
   （1） 服务器启动，在指定端口或随机选取端口绑定 httpd 服务。
   （2）收到一个 HTTP 请求时（其实就是 listen 的端口 accpet 的时候），派生一个线程运行 accept_request 函数。
   （3）取出 HTTP 请求中的 method (GET 或 POST) 和 url,。对于 GET 方法，如果有携带参数，则 query_string 指针
     指向 url 中 ？ 后面的 GET 参数。
   （4） 格式化 url 到 path 数组，表示浏览器请求的服务器文件路径，在 tinyhttpd 中服务器文件是在 htdocs 文件夹下。
     当 url 以 / 结尾，或 url 是个目录，则默认在 path 中加上 index.html，表示访问主页。
   （5）如果文件路径合法，对于无参数的 GET 请求，直接输出服务器文件到浏览器，即用 HTTP 格式写到套接字上，跳到（10）。
     其他情况（带参数 GET，POST 方式，url 为可执行文件），则调用 excute_cgi 函数执行 cgi 脚本。
   （6）读取整个 HTTP 请求并丢弃，如果是 POST 则找出 Content-Length. 把 HTTP 200  状态码写到套接字。
   （7） 建立两个管道，cgi_input 和 cgi_output, 并 fork 一个进程。
   （8） 在子进程中，把 STDOUT 重定向到 cgi_outputt 的写入端，把 STDIN 重定向到 cgi_input 的读取端，关闭 cgi_input 
     的写入端 和 cgi_output 的读取端，设置 request_method 的环境变量，GET 的话设置 query_string 的环境变量，POST 的
     话设置 content_length 的环境变量，这些环境变量都是为了给 cgi 脚本调用，接着用 execl 运行 cgi 程序。
   （9） 在父进程中，关闭 cgi_input 的读取端 和 cgi_output 的写入端，如果 POST 的话，把 POST 数据写入 cgi_input，
     已被重定向到 STDIN，读取 cgi_output 的管道输出到客户端，该管道输入是 STDOUT。接着关闭所有管道，等待子进程结束
   （10） 关闭与浏览器的连接，完成了一次 HTTP 请求与回应，因为 HTTP 是无连接的。
*/

void accept_request(int);//接受请求并处理

void bad_request(int);  //无法处理请求，回写错误码400到client

void cat(int, FILE *);//将文件内容发送到客户端

void cannot_execute(int);//执行CGI程序错误的处理

void error_die(const char *);//输出错误信息

//运行Cgi的处理函数
void execute_cgi(int, const char *, const char *, const char *);

//读取套接字的一行，把回车，换行等情况统一为换行符结束
int get_line(int, char *, int);

void headers(int, const char *);//把http响应的头部写到套接字

void not_found(int);//找不到请求的文件的处理

void serve_file(int, const char *);//调用cat把服务器文件返回给浏览器

//初始化httpd的服务，包含建立套接字，绑定端口，进行监听等。
int startup(u_short *);

void unimplemented(int);// 返回501状态给客户端

/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
void accept_request(int client)
{
    char buf[1024];//缓存数据使用
    int numchars;
    char method[255];//请求的方法
    char url[255];//资源定位符
    char path[512];//文件路径
    size_t i, j;
    struct stat st;//文件结构体
    //cgi=1时表示这是个cgi程序
    int cgi = 0;      /* becomes true if server decides this is a CGI program */
    char *query_string = NULL;

    /*得到请求的第一行*/
    numchars = get_line(client, buf, sizeof(buf));
    i = 0; 
    j = 0;
    /*把客户端的请求方法存到 method 数组*/
    while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
    {
        method[i] = buf[j];
        i++; j++;
    }
    method[i] = '\0';

    /*如果既不是 GET 又不是 POST 则无法处理 */
    /**
      strcasecmp（忽略大小写比较字符串）
      函数原型 int strcasecmp (const char *s1, const char *s2);
      函数说明 strcasecmp()用来比较参数s1和s2字符串，比较时会自动忽略大小写的差异
      返回值 若参数s1和s2字符串相等则返回0。s1大于s2则返回大于0 的值，s1 小于s2 则返回小于0的值
    */
    //都为真的时候说明字符串不匹配
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        unimplemented(client);
        return;
    }
    /* POST 的时候开启 cgi */
    if (strcasecmp(method, "POST") == 0)
        cgi = 1;

    /*读取 url 地址*/
    i = 0;

    //忽略空格
    while (ISspace(buf[j]) && (j < sizeof(buf)))
        j++;

    
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
    {
        /*存下 url */
        url[i] = buf[j];
        i++;
        j++;
    }
    //字符数组后加‘\0’
    url[i] = '\0';

    /*处理 GET 方法*/
    if (strcasecmp(method, "GET") == 0)
    {
        /* 待处理请求为 url */
        //字符的指针指向字符数组的首地址
        query_string = url;

        //使用一个指针对url进行解析，读取参数
        while ((*query_string != '?') && (*query_string != '\0'))
            query_string++;

        /* GET 方法特点，? 后面为参数*/
        if (*query_string == '?')
        {
            /*开启 cgi */
            cgi = 1;
            *query_string = '\0';
            query_string++;
        }
    }

    /*格式化 url 到 path 数组，html 文件都在 htdocs 中*/
    sprintf(path, "htdocs%s", url);

    /*默认情况为 index.html */
    if (path[strlen(path) - 1] == '/')
        strcat(path, "index.html");

    /*根据路径找到对应文件 */
    //int stat(const char * file_name, struct stat *buf)
    //stat()用来将参数file_name 所指的文件状态, 复制到参数buf 所指的结构中。
    //执行成功则返回0，失败返回-1，错误代码存于errno。
    if (stat(path, &st) == -1) {
        /*把所有 headers 的信息都丢弃*/
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
           //从buf中读取一行
            numchars = get_line(client, buf, sizeof(buf));
        /*回应客户端找不到*/
        not_found(client);
    }
    else
    {
        /*如果是个目录，则默认使用该目录下 index.html 文件*/
      if ((st.st_mode & S_IFMT) == S_IFDIR)  //判断是否是一个目录
            strcat(path, "/index.html"); //连接html的页面到指定的路径

      //文件所有者,用户组,其他用户组具有可执行权限,启动cgi程序
      if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
          cgi = 1;

      /*不是 cgi,直接把服务器文件返回，否则执行 cgi */
      if (!cgi)
          serve_file(client, path);
      else
          execute_cgi(client, path, method, query_string);
    }

    /*断开与客户端的连接（HTTP 特点：无连接）*/
    close(client);
}

/**********************************************************************/
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */
/**********************************************************************/
void bad_request(int client)
{
    char buf[1024];

    /*回应客户端错误的 HTTP 请求 */
    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

/**********************************************************************/
/* Put the entire contents of a file out on a socket.  This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the client socket descriptor
 *             FILE pointer for the file to cat */
/**********************************************************************/
void cat(int client, FILE *resource)
{
    char buf[1024];

    /*读取文件中的所有数据写到 socket */
    /**
    (1)函数：char *fgets(char *buf, int bufsize, FILE *stream);
    (2)参数：
    *buf: 字符型指针，指向用来存储所得数据的地址。
    bufsize: 整型数据，指明存储数据的大小。
    *stream: 文件结构体指针，将要读取的文件流。
    (3)返回值：成功，则返回第一个参数buf；
    在读字符时遇到end-of-file，则eof指示器被设置，如果还没读入任何字符就遇到这种情况，则buf保持原来的内容，返回NULL；
    如果发生读入错误，error指示器被设置，返回NULL，buf的值可能被改变。
    (4)作用：从文件中读入字符串
    */
    fgets(buf, sizeof(buf), resource);
    while (!feof(resource))
    {
        //发送数据到客户端
        send(client, buf, strlen(buf), 0);//给客户端发送数据
        //从文件中读取到buf数组
        fgets(buf, sizeof(buf), resource);//读取文件中的数据
    }
}

/**********************************************************************/
/* Inform the client that a CGI script could not be executed.
 * Parameter: the client socket descriptor. */
/**********************************************************************/
void cannot_execute(int client)
{
    char buf[1024];

    /* 回应客户端 cgi 无法执行*/
    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
/**********************************************************************/
void error_die(const char *sc)
{
    /*出错信息处理 */
    perror(sc);
    exit(1);
}

/**********************************************************************/
/* Execute a CGI script.  Will need to set environment variables as
 * appropriate.
 * Parameters: client socket descriptor
 *             path to the CGI script */
/**********************************************************************/
void execute_cgi(int client, const char *path, const char *method, const char *query_string)
{
    char buf[1024];
    int cgi_output[2];
    int cgi_input[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars = 1;
    int content_length = -1;

    buf[0] = 'A'; buf[1] = '\0';
    if (strcasecmp(method, "GET") == 0)  //GET请求
        /*把所有的 HTTP header 读取并丢弃*/
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
            //一行行读取buf中的数据
            numchars = get_line(client, buf, sizeof(buf));
    else    /* POST */
    {
        /* 对 POST 的 HTTP 请求中找出 content_length */
        numchars = get_line(client, buf, sizeof(buf));
        while ((numchars > 0) && strcmp("\n", buf))
        {
            /*利用 \0 进行分隔 */
            buf[15] = '\0';
            /* HTTP 请求的特点*/
            if (strcasecmp(buf, "Content-Length:") == 0)
            //字符串转换成数字
                content_length = atoi(&(buf[16]));
            numchars = get_line(client, buf, sizeof(buf));
        }
        /*没有找到 content_length */
		if (content_length == -1) {
            /*错误请求*/
            bad_request(client);
            return;
        }
    }

    /* 正确，HTTP 状态码 200 */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);

   /**建立输入输出的管道*/
    /* 建立管道*/
    if (pipe(cgi_output) < 0) {
        /*错误处理*/
        cannot_execute(client);
        return;
    }
    /*建立管道*/
    if (pipe(cgi_input) < 0) {
        /*错误处理*/
        cannot_execute(client);
        return;
    }


    //fork()失败
    if ((pid = fork()) < 0 ) {
        /*错误处理*/
        cannot_execute(client);
        return;
    }
    //子进程
    if (pid == 0)  /* child: CGI script */
    {
        char meth_env[255];
        char query_env[255];
        char length_env[255];

        /* 把 STDOUT 重定向到 cgi_output 的写入端 */
        dup2(cgi_output[1], 1);
        /* 把 STDIN 重定向到 cgi_input 的读取端 */
        dup2(cgi_input[0], 0);
        /* 关闭 cgi_input 的写入端 和 cgi_output 的读取端 */
        close(cgi_output[0]);
        close(cgi_input[1]);
        /*设置 request_method 的环境变量*/
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        putenv(meth_env);
        if (strcasecmp(method, "GET") == 0) {
            /*设置 query_string 的环境变量*/
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        }
        else {   /* POST */
            /*设置 content_length 的环境变量*/
            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }
        /*用 execl 运行 cgi 程序*/
        execl(path, path, NULL);
        exit(0);
    } 
    //父进程
    else {    /* parent */
        /* 关闭 cgi_input 的读取端 和 cgi_output 的写入端 */
        close(cgi_output[1]);
        close(cgi_input[0]);
        if (strcasecmp(method, "POST") == 0)
            /*接收 POST 过来的数据*/
            for (i = 0; i < content_length; i++) 
            {
                recv(client, &c, 1, 0);
                /*把 POST 数据写入 cgi_input，现在重定向到 STDIN */
                write(cgi_input[1], &c, 1);
            }
        /*读取 cgi_output 的管道输出到客户端，该管道输入是 STDOUT */
        while (read(cgi_output[0], &c, 1) > 0)
            send(client, &c, 1, 0);

        /*关闭管道*/
        close(cgi_output[0]);
        close(cgi_input[1]);
        /*等待子进程*/
        waitpid(pid, &status, 0);
    }
}

/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    /*把终止条件统一为 \n 换行符，标准化 buf 数组*/
    while ((i < size - 1) && (c != '\n'))
    {
        /*一次仅接收一个字节*/
        //函数原型int recv( _In_ SOCKET s, _Out_ char *buf, _In_ int len, _In_ int flags);
        //param:socket套接字,buf数组,读取长度,指定调用方式
        n = recv(sock, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0)
        {
            /*收到 \r 则继续接收下个字节，因为换行符可能是 \r\n */ //回车换行 \r\n
            if (c == '\r')
            {
                /*使用 MSG_PEEK 标志使下一次读取依然可以得到这次读取的内容，可认为接收窗口不滑动*/
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */

                /*但如果是换行符则把它吸收掉*/
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            /*存到缓冲区*/
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    //字符数组末尾添加'\0'
    buf[i] = '\0';

    /*返回 buf 数组大小*/
    return(i);
}

/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
/**********************************************************************/
void headers(int client, const char *filename)
{
    char buf[1024];
    (void)filename;  /* could use filename to determine file type */

    /*正常的 HTTP header */
    //http头写进buf数组，并发送给客户端
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    
    /*服务器信息*/
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void not_found(int client)
{
    char buf[1024];

    /* 404 页面 */
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    /*服务器信息*/
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
void serve_file(int client, const char *filename)
{
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];

    /*读取并丢弃 header */
    buf[0] = 'A'; buf[1] = '\0';
    while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
        numchars = get_line(client, buf, sizeof(buf));

    /*打开 sever 的文件*/
    resource = fopen(filename, "r");//返回一个文件指针
    if (resource == NULL)
        not_found(client);
    else
    {
        /*写 HTTP header */
        headers(client, filename);
        /*复制文件*/
        cat(client, resource);
    }
    fclose(resource);
}

/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
int startup(u_short *port)
{
    int httpd = 0;
	//套接字结构体对象
	/**
struct sockaddr {
	sa_family_t     sa_family;    //address family, AF_xxx       
	char            sa_data[14];  //14 bytes of protocol address 
	};
struct sockaddr_in
{
	__SOCKADDR_COMMON (sin_);
	in_port_t sin_port;  //Port number. 
	//struct in_addr{unsigned long s_addr ;};32位IP地址
	struct in_addr sin_addr;  //Internet address. 

	// Pad to size of `struct sockaddr'. 
	unsigned char sin_zero[sizeof(struct sockaddr) -__SOCKADDR_COMMON_SIZE -sizeof(in_port_t) - sizeof(struct in_addr)];
	//字符数组sin_zero[8]的存在是为了保证结构体struct sockaddr_in的大小和结构体struct sockaddr的大小相等 
};
	
*/
    struct sockaddr_in name;

    /*建立 socket */
	/**
int  socket(int protofamily, int type, int protocol);//返回sockfd
  sockfd是描述符。
  socket函数对应于普通文件的打开操作。普通文件的打开操作返回一个文件描述字，
  而socket()用于创建一个socket描述符（socket descriptor），它唯一标识一个socket。
  这个socket描述字跟文件描述字一样，后续的操作都有用到它，把它作为参数，通过它来进行一些读写操作。
正如可以给fopen的传入不同参数值，以打开不同的文件。创建socket的时候，也可以指定不同的参数创建不同的socket描述符，
socket函数的三个参数分别为：

(1)protofamily：即协议域，又称为协议族（family）。常用的协议族有，AF_INET(IPV4)、AF_INET6(IPV6)、AF_LOCAL（或称AF_UNIX，Unix域socket）、AF_ROUTE等等。
协议族决定了socket的地址类型，在通信中必须采用对应的地址，如AF_INET决定了要用ipv4地址（32位的）与端口号（16位的）的组合、AF_UNIX决定了要用一个绝对路径名作为地址。
(2)type：指定socket类型。常用的socket类型有，SOCK_STREAM、SOCK_DGRAM、SOCK_RAW、SOCK_PACKET、SOCK_SEQPACKET等等（socket的类型有哪些？）。
(3)protocol：故名思意，就是指定协议。常用的协议有，IPPROTO_TCP、IPPTOTO_UDP、IPPROTO_SCTP、IPPROTO_TIPC等，它们分别对应TCP传输协议、UDP传输协议、STCP传输协议、TIPC传输协议（这个协议我将会单独开篇讨论！）。
注意：并不是上面的type和protocol可以随意组合的，如SOCK_STREAM不可以跟IPPROTO_UDP组合。
当protocol为0时，会自动选择type类型对应的默认协议。

  当我们调用socket创建一个socket时，返回的socket描述字它存在于协议族（address family，AF_XXX）空间中，但没有一个具体的地址。
如果想要给它赋值一个地址，就必须调用bind()函数，否则就当调用connect()、listen()时系统会自动随机分配一个端口。	
	*/
    httpd = socket(PF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
        error_die("socket");
	//结构体对象全部初始化为0
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);//字节序的转换
	//linux下的socket INADDR_ANY表示的是一个服务器上所有的网卡
	//（服务器可能不止一个网卡）
    //多个本地ip地址都进行绑定端口号，进行侦听。
    name.sin_addr.s_addr = htonl(INADDR_ANY);
	//绑定socket到指定的地址

	/**
	  int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
	   (1)sockfd：即socket描述字，它是通过socket()函数创建了，唯一标识一个socket
	   (2)addr：一个const struct sockaddr *指针，指向要绑定给sockfd的协议地址。
	   这个地址结构根据地址创建socket时的地址协议族的不同而不同，
	   (3)addrlen：对应的是地址的长度
	*/
    if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
        error_die("bind");
    /*如果当前指定端口是 0，则动态随机分配一个端口*/
    if (*port == 0)  /* if dynamically allocating a port */
    {
        int namelen = sizeof(name);
        if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
            error_die("getsockname");
        //uint16_t ntohs(uint16_t netshort);
        //将一个16位数由网络字节顺序转换为主机字节顺序
        *port = ntohs(name.sin_port);
    }
    /*开始监听*/
    if (listen(httpd, 5) < 0)
        error_die("listen");

    /*返回 socket id */
    return(httpd);
}

/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
void unimplemented(int client)
{
	//buff用于写入错误信息的字符数组
    char buf[1024];

    /* HTTP method 不被支持*/
    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    /*服务器信息*/
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/

int main(void)
{
    int server_sock = -1;
    u_short port = 0;
    int client_sock = -1;
    struct sockaddr_in client_name;
    int client_name_len = sizeof(client_name);
    pthread_t newthread;//线程对象

    /*在对应端口建立 httpd 服务*/
    server_sock = startup(&port);
    printf("httpd running on port %d\n", port);

    while (1)
    {
        /*套接字收到客户端连接请求*/
        client_sock = accept(server_sock,(struct sockaddr *)&client_name,&client_name_len);
        if (client_sock == -1)
            error_die("accept");
        /*派生新线程用 accept_request 函数处理新请求*/
        /* accept_request(client_sock); */
        if (pthread_create(&newthread , NULL, accept_request, client_sock) != 0)
            perror("pthread_create");
    }

    close(server_sock);

    return(0);
}
