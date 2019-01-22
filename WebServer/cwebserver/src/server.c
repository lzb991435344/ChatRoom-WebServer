#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netinet/in.h>

#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "server.h"
#include "log.h"
#include "connection.h"
#include "config.h"

// 默认端口号
#define DEFAULT_PORT 8080
#define BACKLOG 10

// 子进程信号处理函数
static void sigchld_handler(int s) {
    pid_t pid;
	//pid>0 等待任何子进程识别码为pid的子进程。
	//WNOHANG 如果没有任何已经结束的子进程则马上返回，不予以等待。
   /**
	 pid_t waitpid(pid_t pid,int * status,int options);
    函数说明:waitpid()会暂时停止目前进程的执行，直到有信号来到或子进程结束。
	  如果在调用wait()时子进程已经结束，则wait()会立即返回子进程结束状态值。
	  子进程的结束状态值会由参数status返回，而子进程的进程识别码也会一块返回。
	  如果不在意结束状态值，则参数status可以设成NULL。参数pid为欲等待的子进程识别码，
    返回值：如果执行成功则返回子进程识别码(PID)，如果有错误发生则返回-1。失败原因存于errno中。
   */
    while((pid = waitpid(-1, NULL, WNOHANG)) > 0);
}

// 初始化服务器结构体
static server* server_init() {
    server *serv;
    serv = malloc(sizeof(*serv));//分配内存
    memset(serv, 0, sizeof(*serv));//全部初始化为0
    return serv;
}

// 释放服务器结构体
static void server_free(server *serv) {
    config_free(serv->conf);
    free(serv);
}

// 以守护进程的方式启动
static void daemonize(server *serv, int null_fd) {
	/**
	(1)定义函数 int sigaction(int signum,const struct sigaction *act ,struct sigaction *oldact);
    (2)函数说明 sigaction()会依参数signum指定的信号编号来设置该信号的处理函数。
   参数signum可以指定SIGKILL和SIGSTOP以外的所有信号。
	(3)
	struct sigaction {
	    //以下两个函数都可以作为处理函数
	    //如果sa_flags设置为SA_SIGINFO,使用第二个函数
        void (*sa_handler)(int);//默认使用,向处理函数发送信号的数值
        void (*sa_sigaction)(int, siginfo_t *, void *);//可以向处理函数发送附加信息

        sigset_t sa_mask;
        int sa_flags;
        void (*sa_restorer)(void);
       };
	*/
    struct sigaction sa;
    int fd0, fd1, fd2;

	/**
	  当我们登录系统之后创建一个文件总是有一个默认权限的，那么这个权限是怎么来的呢？
	这就是umask干的事情。umask设置了用户创建文件的默认 权限，它与chmod的效果刚好相反，
	umask设置的是权限“补码”，而chmod设置的是文件权限码。
	#include <sys/stat.h>
     mode_t umask(mode_t cmask);
	 其中cmask对应下面的9个位的“或”值：
    S_IRUSR 用户读
    S_IWUSR 用户写
    S_IXUSR 用户执行
    S_IRGRP 组读
    S_IWGRP 组写
    S_IXGRP 组执行
    S_IROTH 其他读
    S_IWOTH 其他写
    S_IXOTH 其他执行
   这9个位对应linux文件的权限位。
	*/
	//作用:就是设置允许当前进程创建文件或者目录最大可操作的权限，
	//比如这里设置为0，它的意思就是0取反再创建文件时权限相与，也就是：
	//(~0) & mode 等于八进制的值0777 & mode了，这样就是给后面的代码调用
	//函数mkdir给出最大的权限，避免了创建目录或文件的权限不确定性。
    umask(0);

    // fork()新的进程
    switch(fork()) {
        case 0://子进程
            break;
        case -1://出错
            log_error(serv, "daemon fork 1: %s", strerror(errno));
            exit(1);
        default:
            exit(0);
    }
	/**
 1. 在后台运行。
  为避免挂起控制终端将Daemon放入后台执行。方法是在进程中调用fork使父进程终止，让Daemon在子进
程中后台执行。
   if(pid=fork())
      exit(0);//是父进程，结束父进程，子进程继续
 2. 脱离控制终端，登录会话和进程组
  有必要先介绍一下Linux中的进程与控制终端，登录会话和进程组之间的关系：进程属于一个进程组，
进程组号（GID）就是进程组长的进程号（PID）。登录会话可以包含多个进程组。这些进程组共享一个
控制终端。这个控制终端通常是创建进程的登录终端。
控制终端，登录会话和进程组通常是从父进程继承下来的。我们的目的就是要摆脱它们，使之不受它们
的影响。方法是在第1点的基础上，调用setsid()使进程成为会话组长：
setsid();
说明：当进程是会话组长时setsid()调用失败。但第一点已经保证进程不是会话组长。setsid()调用成
功后，进程成为新的会话组长和新的进程组长，并与原来的登录会话和进程组脱离。由于会话过程对控
制终端的独占性，进程同时与控制终端脱离。
 3. 禁止进程重新打开控制终端
  现在，进程已经成为无终端的会话组长。但它可以重新申请打开一个控制终端。可以通过使进程不再成
为会话组长来禁止进程重新打开控制终端：
if(pid=fork())
exit(0);//结束第一子进程，第二子进程继续（第二子进程不再是会话组长）
 4. 关闭打开的文件描述符
  进程从创建它的父进程那里继承了打开的文件描述符。如不关闭，将会浪费系统资源，造成进程所在的
文件系统无法卸下以及引起无法预料的错误。按如下方法关闭它们：
for(i=0;i 关闭打开的文件描述符close(i);>
 5. 改变当前工作目录
  进程活动时，其工作目录所在的文件系统不能卸下。一般需要将工作目录改变到根目录。对于需要转储
核心，写运行日志的进程将工作目录改变到特定目录如/tmpchdir("/")
 6. 重设文件创建掩模
  进程从创建它的父进程那里继承了文件创建掩模。它可能修改守护进程所创建的文件的存取位。为防止
这一点，将文件创建掩模清除：umask(0);
 7. 处理SIGCHLD信号
  处理SIGCHLD信号并不是必须的。但对于某些进程，特别是服务器进程往往在请求到来时生成子进程处理
请求。如果父进程不等待子进程结束，子进程将成为僵尸进程（zombie）从而占用系统资源。如果父进程
等待子进程结束，将增加父进程的负担，影响服务器进程的并发性能。在Linux下可以简单地将SIGCHLD信
号的操作设为SIG_IGN。
  signal(SIGCHLD,SIG_IGN);
这样，内核在子进程结束时不会产生僵尸进程。这一点与BSD4不同，BSD4下必须显式等待子进程结束才能
释放僵尸进程。
	*/
    setsid();
	/**
	struct sigaction {
		//以下两个函数都可以作为处理函数
		//如果sa_flags设置为SA_SIGINFO,使用第二个函数
		void (*sa_handler)(int);//默认使用,向处理函数发送信号的数值
		void (*sa_sigaction)(int, siginfo_t *, void *);//可以向处理函数发送附加信息

		sigset_t sa_mask;
		int sa_flags;
		void (*sa_restorer)(void);
	   };
	*/
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);//设置文件掩模
    sa.sa_flags = 0;

    if (sigaction(SIGHUP,  &sa, NULL) < 0) {
        log_error(serv, "SIGHUP: %s", strerror(errno));
        exit(1);
    }

    switch(fork()) {
        case 0://子进程
            break;
        case -1://fork失败
            log_error(serv, "daemon fork 2: %s", strerror(errno));
            exit(1);
        default:
            exit(0);//退出
    }

    chdir("/");//工作目录改变到根目录

    // 关闭标准输入，输出，错误
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
   //复制文件描述符
    fd0 = dup(null_fd);
    fd1 = dup(null_fd);
    fd2 = dup(null_fd);

    if (null_fd != -1)
        close(null_fd);

    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
        log_error(serv, "unexpected fds: %d %d %d", fd0, fd1, fd2);
        exit(1);
    }

    log_info(serv, "pid: %d", getpid());
}

/*
 * 检查日志文件和web文件目录是否在chroot_path下，如果在则执行chroot限制web服务器访问的目录
 */
static void jail_server(server *serv, char *logfile, const char *chroot_path) {
    size_t root_len = strlen(chroot_path);
    size_t doc_len = strlen(serv->conf->doc_root);//web文件目录长度
    size_t log_len = strlen(logfile);//logfile文件长度

    // 检查web文件目录是否在chroot_path下
    if (root_len < doc_len && strncmp(chroot_path, serv->conf->doc_root, root_len) == 0) {
        // 更新web文件目录为chroot_path的相对路径
        strncpy(serv->conf->doc_root, &serv->conf->doc_root[0] + root_len, doc_len - root_len + 1);
    } else {
		//标准错误输出
		//chroot_path和web文件目录不匹配
        fprintf(stderr, "document root %s is not a sub-directory in chroot %s\n", serv->conf->doc_root, chroot_path);
        exit(1);
    }

    // 检查日志文件是否在chroot_path下
    if (serv->use_logfile) {
        if (logfile[0] != '/')
            fprintf(stderr, "warning: log file is not an absolute path, opening it will fail if it's not in chroot\n");
        else if (root_len < log_len && strncmp(chroot_path, logfile, root_len) == 0) {
            // 更新日志文件为chroot_path的相对路径
			//参数,指向logfile的指针(目的地址),源地址,复制的长度
            strncpy(logfile, logfile + root_len, log_len - root_len + 1);
        } else {
            fprintf(stderr, "log file %s is not in chroot\n", logfile);
            exit(1);
        }
    }

    // 执行chroot
	//chroot(PATH)这个function必须具有 root的身份才能执行，
	//执行后会将根目录切换到PATH 所指定的地方。
    if (chroot(chroot_path) != 0) {
        perror("chroot");
        exit(1);
    }

    chdir("/");
}

// bind()绑定端口并listen()监听
static void bind_and_listen(server *serv) {
    struct sockaddr_in serv_addr;

    // 创建socket
    serv->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serv->sockfd < 0) {
        perror("socket");
        log_error(serv, "socket: %s", strerror(errno));
        exit(1);
    }

    int yes = 0;
    if ((setsockopt(serv->sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) == -1) {
        perror("setsockopt");
        log_error(serv, "socket: %s", strerror(errno));
        exit(1);
    }

	//初始化服务结构体
    memset(&serv_addr, 0, sizeof serv_addr);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
	//htons是将整型变量从主机字节顺序转变成网络字节顺序， 就是整数在地址空间
	//存储方式变为高位字节存放在内存的低地址处。
    serv_addr.sin_port = htons(serv->port);//地址转换

    // bind() 绑定
    if (bind(serv->sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        log_error(serv, "bind: %s", strerror(errno));
        exit(1);
    }

    // listen() 监听，等待客户端连接
    if (listen(serv->sockfd, BACKLOG) < 0) {
        perror("listen");
        log_error(serv, "listen: %s", strerror(errno));
        exit(1);
    }
}

/** 启动并初始化服务器
 参数1:服务结构体的指针
 参数2：指向配置的指针
 参数3：chroot的文件路径的指针
 参数4：指向logfile的指针
 */
static void start_server(server *serv, const char *config, const char *chroot_path, char *logfile) {
    int null_fd = -1;
    
    // 1. 加载配置文件
    serv->conf = config_init();
    config_load(serv->conf, config);

    // 2. 设置端口号
    if (serv->port == 0 && serv->conf->port != 0) {
        serv->port = serv->conf->port;
    }
    else if (serv->port == 0) {
        serv->port = DEFAULT_PORT;//使用默认的端口号
    }

    printf("port: %d\n", serv->port);
    

    if (serv->is_daemon) {
		/**
		int open( const char * pathname, int flags);
		  O_RDONLY 以只读方式打开文件
          O_WRONLY 以只写方式打开文件
          O_RDWR 以可读写方式打开文件。

		  两个特殊的设备：
		  (1)/dev/null  ： 在类Unix系统中，/dev/null，或称空设备，
		  是一个特殊的设备文件，它丢弃一切写入其中的数据（但报告写入操作成功），
		  读取它则会立即得到一个EOF。
		  (2)/dev/zero也是一个伪文件，但它实际上产生连续不断的null的流（二进制的零流，
		  而不是ASCII型的）。写入它的输出会丢失不见，/dev/zero主要的用处是用来创建一
		  个指定长度用于初始化的空文件，像临时交换文件。
		*/
        null_fd = open("/dev/null", O_RDWR);//可读写的空设备
    }

    // 3. 判断是否chroot
    if (serv->do_chroot) {
        jail_server(serv, logfile, chroot_path);
    }

    // 4. 打开日志文件
    log_open(serv, logfile);
    
    // 5. 判断是否以守护进程方式启动
    if (serv->is_daemon) {//1表示以守护进程运动
        daemonize(serv, null_fd);
    }

    // 6. 绑定并监听
    bind_and_listen(serv);
}

/*
 * 使用fork()新的进程来处理新的客户端连接和HTTP请求
 */
static void do_fork_strategy(server *serv) {
    pid_t pid;//进程pid
    struct sigaction sa;//依照编号设置处理函数
    connection *con;//客户端连接结构体的指针

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);

	/**
	a.SA_INTERRUPT:由此信号中断的系统调用不会自动重启动(针对sigaction的XSI默认处理方式不会
	自动重启)
    b.SA_RESTART:  由此信号中断的系统调用会自动重启。
	*/
    sa.sa_flags = SA_RESTART;

	/**
	(1)定义函数 int sigaction(int signum,const struct sigaction *act ,struct sigaction *oldact);
	(2)函数说明 sigaction()会依参数signum指定的信号编号来设置该信号的处理函数。
   参数signum可以指定SIGKILL和SIGSTOP以外的所有信号。
    (3)返回值：成功返回0，失败返回-1
	*/
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    while (1) {
		//说明当前的连接无效
        if ((con = connection_accept(serv)) == NULL) {
            continue;//继续下一个循环
        }

		//fork()处理客户端连接
        if ((pid = fork()) == 0) {
            
            // 子进程中处理HTTP请求
            close(serv->sockfd);

            connection_handler(serv, con);//处理请求
            connection_close(con);//关闭连接

            exit(0);
        }
        printf("child process: %d\n", pid);
        connection_close(con);
    }
}

// 主函数
int main(int argc, char** argv) {
    
    // 初始化服务器
    server *serv;
    char logfile[PATH_MAX];
    char chroot_path[PATH_MAX];
    int opt;
    
    serv = server_init();

    // 解析命令行参数
	/**
	  int getopt(int argc, char * const argv[], const char *optstring);
	  参数：argc：main()函数传递过来的参数的个数
            argv：main()函数传递过来的参数的字符串指针数组
            optstring：选项字符串，告知 getopt()可以处理哪个选项以及哪个选项需要参数

			char*optstring = “ab:c::”;
           单个字符a         表示选项a没有参数            格式：-a即可，不加参数
           单字符加冒号b:     表示选项b有且必须加参数      格式：-b 100或-b100,但-b=100错
           单字符加2冒号c::   表示选项c可以有，也可以无     格式：-c200，其它格式错误

		   optarg —— 指向当前选项参数(如果有)的指针。
           optind —— 再次调用 getopt() 时的下一个 argv指针的索引。
           optopt —— 最后一个未知选项。
           opterr ­—— 如果不希望getopt()打印出错信息，则只要将全域变量opterr设为0即可。
	  返回值：
         如果选项成功找到，返回选项字母；如果所有命令行选项都解析完毕，返回 -1；
		如果遇到选项字符不在 optstring 中，返回字符 '?'；如果遇到丢失参数，那么返回值
		依赖于 optstring 中第一个字符，如果第一个字符是 ':' 则返回':'，否则返回'?'并提
		示出错误信息。
	*/
    while((opt = getopt(argc, argv, "p:l:r:d")) != -1) {
        switch(opt) {
            // 设置端口号
            case 'p':
                serv->port = atoi(optarg);
                if (serv->port == 0) {
                    fprintf(stderr, "error: port must be an integer\n");
                    exit(1); 
                }
                break;
            // 以守护进程方式启动
            case 'd':
                serv->is_daemon = 1;
                break;
            // 使用日志文件
            case 'l':
                strcpy(logfile, optarg);
                serv->use_logfile = 1;
                break;
            // 使用chroot
            case 'r':
                if (realpath(optarg, chroot_path) == NULL) {
                    perror("chroot");
                    exit(1);
                }
                serv->do_chroot = 1;
                break;
        }
    }

    // 启动服务
    start_server(serv, "web.conf", chroot_path, logfile);
    
    // 进入主循环等待并处理客户端连接
    do_fork_strategy(serv);
    
    // 关闭日志文件
    log_close(serv);
    
    // 释放服务器结构体
    server_free(serv);

    return 0;
}
