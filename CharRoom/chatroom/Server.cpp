#include <iostream>

#include "Server.h"

using namespace std;

// 服务端类成员函数

/**Tcp服务端通信步骤
(1)使用socket()创建TCP套接字（socket）
(2)将创建的套接字绑定到一个本地地址和端口上（Bind）
(3)将套接字设为监听模式，准备接收客户端请求（listen）
(4)等待客户请求到来: 当请求到来后，接受连接请求，返回一个对应于此次连接的新的套接字（accept）
(5)用accept返回的套接字和客户端进行通信（使用write()/send()或send()/recv() )
(6)返回，等待另一个客户请求
(7)关闭套接字
*/

// 服务端类构造函数
Server::Server(){
    
    // 初始化服务器地址和端口
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // 初始化socket
    listener = 0;
    
    // epool fd
    epfd = 0;
}

// 初始化服务端并启动监听
//socket(),bind(),listen()
void Server::Init() {
    cout << "Init Server..." << endl;
    
     //创建监听socket,返回一个socket的文件描述符
	/**
	int socket(int protofamily, int type, int protocol);
	 参数1:协议族
	 参数2:socket的类型
	 参数3:指定的协议
	*/
    listener = socket(PF_INET, SOCK_STREAM, 0);
    if(listener < 0) { perror("listener"); exit(-1);}
    
    //绑定地址
	/**
	 int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
	参数1:socket返回的套接字
	参数2:要绑定的协议地址
	参数3:对应地址的长度
	*/
    if( bind(listener, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind error");
        exit(-1);
    }

    //监听
	/**
	int listen(int sockfd, int backlog);
	 listen函数的第一个参数即为要监听的socket描述字，第二个参数为相应socket可以排队的最大连接个数。socket()函数创建的socket默认是一个主动类型的，
	 listen函数将socket变为被动类型的，等待客户的连接请求。


	*/
    int ret = listen(listener, 5);
    if(ret < 0) {
        perror("listen error"); 
        exit(-1);
    }

    cout << "Start to listen: " << SERVER_IP << endl;

    //在内核中创建事件表
    epfd = epoll_create(EPOLL_SIZE);
    
    if(epfd < 0) {
        perror("epfd error");
        exit(-1);
    }

    //往事件表里添加监听事件
    addfd(epfd, listener, true);

}

// 关闭服务，清理并关闭文件描述符
void Server::Close() {

    //关闭socket
    close(listener);
    
    //关闭epoll监听
    close(epfd);
}

// 发送广播消息给所有客户端
int Server::SendBroadcastMessage(int clientfd)
{
    // buf[BUF_SIZE] 接收新消息
    // message[BUF_SIZE] 保存格式化的消息
    char buf[BUF_SIZE], message[BUF_SIZE];
	/**
	原型：extern void bzero（void *s, int n）;
　　用法：#include <string.h>
　　功能：置字节字符串s的前n个字节为零且包括‘\0’。
　　说明：bzero无返回值，并且使用strings.h头文件，strings.h曾经是posix标准的一部分，
  但是在POSIX.1-2001标准里面，这些函数被标记为了遗留函数而不推荐使用。在POSIX.1-2008
  标准里已经没有这些函数了。推荐使用memset替代bzero。
	*/
    bzero(buf, BUF_SIZE);
    bzero(message, BUF_SIZE);

    // 接收新消息
    cout << "read from client(clientID = " << clientfd << ")" << endl;
	/** 用于已连接的数据报或流式套接口进行数据的接收。
	int recv( _In_ SOCKET s, _Out_ char *buf, _In_ int len, _In_ int flags);
    int recv( SOCKET s,     char FAR *buf,      int len,     int flags     );

　不论是客户还是服务器应用程序都用recv函数从TCP连接的另一端接收数据。
　该函数的第一个参数指定接收端套接字描述符；
　第二个参数指明一个缓冲区，该缓冲区用来存放recv函数接收到的数据；
　第三个参数指明buf的长度；
　第四个参数一般置0。
	*/

	//接收数据,返回数据的长度
    int len = recv(clientfd, buf, BUF_SIZE, 0);

    // 如果客户端关闭了连接
    if(len == 0) 
    {
        close(clientfd);
        
        // 在客户端列表中删除该客户端
        clients_list.remove(clientfd);
        cout << "ClientID = " << clientfd 
             << " closed.\n now there are " 
             << clients_list.size()
             << " client in the char room"
             << endl;
    }
    // 发送广播消息给所有客户端
    else 
    {
        // 判断是否聊天室还有其他客户端
        if(clients_list.size() == 1) { 
            // 发送提示消息
            send(clientfd, CAUTION, strlen(CAUTION), 0);
            return len;
        }
        // 格式化发送的消息内容
        sprintf(message, SERVER_MESSAGE, clientfd, buf);

        // 遍历客户端列表依次发送消息，需要判断不要给来源客户端发
        list<int>::iterator it;
		//迭代整个客户端的列表
        for(it = clients_list.begin(); it != clients_list.end(); ++it) {
           if(*it != clientfd){
			   //
                if( send(*it, message, BUF_SIZE, 0) < 0 ) {
                    return -1;
                }
           }
        }
    }
    return len;
}

// 启动服务端
void Server::Start() {

    // epoll 事件队列
    static struct epoll_event events[EPOLL_SIZE]; 

    // 初始化服务端
    Init();

    //主循环
    while(1)
    {
        //epoll_events_count表示就绪事件的数目
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);

        if(epoll_events_count < 0) {
            perror("epoll failure");
            break;
        }

        cout << "epoll_events_count =\n" << epoll_events_count << endl;

        //处理这epoll_events_count个就绪事件
        for(int i = 0; i < epoll_events_count; ++i)
        {
            int sockfd = events[i].data.fd;
            //新用户连接
            if(sockfd == listener)//listener是创建监听的socket
            {
                struct sockaddr_in client_address;//结构体对象
				//获取结构体的长度
                socklen_t client_addrLength = sizeof(struct sockaddr_in);

				/**
				accept(int socket, sockaddr *name, int *addrlen)
		       第一个参数，是一个已设为监听模式的socket的描述符。
		       第二个参数，是一个返回值，它指向一个struct sockaddr类型的结构体的变量，保存了发起连接的客户端得IP地址信息和端口信息。
		       第三个参数，也是一个返回值，指向整型的变量，保存了返回的地址信息的长度。
		       accept函数返回值是一个客户端和服务器连接的SOCKET类型的描述符，在服务器端标识着这个客户端。	
				*/
                int clientfd = accept( listener, ( struct sockaddr* )&client_address, &client_addrLength );

                cout << "client connection from: "
					//char *inet_ntoa(struct in_addr in);将网络地址转换成“.”点隔的字符串格式。
					//将一个32位网络字节序的二进制IP地址转换成相应的点分十进制的IP地址（返回点分十进制的字符串在静态内存中的指针）。
                    //返回值是一个char*的指针
					<< inet_ntoa(client_address.sin_addr) << ":"
					//uint16_t ntohs(uint16_t netshort);
					//netshort：一个以网络字节顺序表达的16位数。
					//ntohs()返回一个以主机字节顺序表达的数。
                     << ntohs(client_address.sin_port) << ", clientfd = "
                     << clientfd << endl;

				//注册新的fd到epollfd中
                addfd(epfd, clientfd, true);

                // 服务端用list保存用户连接
                clients_list.push_back(clientfd);
                cout << "Add new clientfd = " << clientfd << " to epoll" << endl;
                cout << "Now there are " << clients_list.size() << " clients int the chat room" << endl;

                // 服务端发送欢迎信息  
                cout << "welcome message" << endl;                
                char message[BUF_SIZE];
                bzero(message, BUF_SIZE);

				//格式化欢迎消息字符串
                sprintf(message, SERVER_WELCOME, clientfd);
				//发送消息
                int ret = send(clientfd, message, BUF_SIZE, 0);
                if(ret < 0) {
                    perror("send error");
                    Close();
                    exit(-1);
                }
            }
            //处理用户发来的消息，并广播，使其他用户收到信息
            else {   
                int ret = SendBroadcastMessage(sockfd);
                if(ret < 0) {
                    perror("error");
                    Close();
                    exit(-1);
                }
            }
        }
    }

    // 关闭服务
    Close();
}