#ifndef  CHATROOM_COMMON_H
#define CHATROOM_COMMON_H

#include <iostream>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 默认服务器端IP地址
#define SERVER_IP "127.0.0.1"

// 服务器端口号
#define SERVER_PORT 8888

// int epoll_create(int size)中的size
// 为epoll支持的最大句柄数
#define EPOLL_SIZE 5000

// 缓冲区大小65535
#define BUF_SIZE 0xFFFF
    
// 新用户登录后的欢迎信息
#define SERVER_WELCOME "Welcome you join to the chat room! Your chat ID is: Client #%d"

// 其他用户收到消息的前缀
#define SERVER_MESSAGE "ClientID %d say >> %s"

// 退出系统
#define EXIT "EXIT"

// 提醒你是聊天室中唯一的客户
#define CAUTION "There is only one int the char room!"


// 注册新的fd到epollfd中
// 参数enable_et表示是否启用ET模式，如果为True则启用，否则使用LT模式
static void addfd( int epollfd, int fd, bool enable_et )
{
	
	/**
	struct epoll_event {
		uint32_t     events;      // Epoll events 
		epoll_data_t data;        // User data variable 
	};
	a.events成员描述事件类型
	EPOLLIN :表示对应的文件描述符可以读(包括对端SOCKET正常关闭)
	EPOLLET: 将EPOLL设为边缘触发(Edge Triggered)模式,这是相对于水平触发
	(Level Triggered)来说的
	b.data用于存储用户数据，是epoll_data_t结构类型

	typedef union epoll_data {
	void        *ptr;
	int          fd;
	uint32_t     u32;
	uint64_t     u64;
	} epoll_data_t;

	*/
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;//可读
    if( enable_et )
        ev.events = EPOLLIN | EPOLLET;

	/**
	int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
	epfd是创建的句柄，op包含三种：
    a.EPOLL_CTL_ADD，向epfd注册fd的上的event
	b.EPOLL_CTL_MOD，修改fd已注册的event
	c.EPOLL_CTL_DEL，从epfd上删除fd的event
	  fd是文件描述符，event指定内核要监听事件,它是struct epoll_event
	结构类型的指针。
	*/
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    // 设置socket为nonblocking模式
    // 执行完就转向下一条指令，不管函数有没有返回。

	/**


fcntl（文件描述词操作）
相关函数  open，flock

表头文件  #include<unistd.h>
#include<fcntl.h>

定义函数  int fcntl(int fd , int cmd);
int fcntl(int fd,int cmd,long arg);
int fcntl(int fd,int cmd,struct flock * lock);

函数说明  fcntl()用来操作文件描述词的一些特性。参数fd代表欲设置的文件描述词，参数cmd代表欲操作的指令。
有以下几种情况:
F_DUPFD用来查找大于或等于参数arg的最小且仍未使用的文件描述词，并且复制参数fd的文件描述词。
 执行成功则返回新复制的文件描述词。请参考dup2()。F_GETFD取得close-on-exec旗标。若此旗标的FD_CLOEXEC位为0，代表在调用exec()相关函数时文件将不会关闭。
F_SETFD 设置close-on-exec 旗标。该旗标以参数arg 的FD_CLOEXEC位决定。
F_GETFL 取得文件描述词状态旗标，此旗标为open（）的参数flags。
F_SETFL 设置文件描述词状态旗标，参数arg为新旗标，但只允许O_APPEND、O_NONBLOCK和O_ASYNC位的改变，其他位的改变将不受影响。
F_GETLK 取得文件锁定的状态。
F_SETLK 设置文件锁定的状态。此时flcok 结构的l_type 值必须是F_RDLCK、F_WRLCK或F_UNLCK。如果无法建立锁定，则返回-1，错误代码为EACCES 或EAGAIN。
F_SETLKW F_SETLK 作用相同，但是无法建立锁定时，此调用会一直等到锁定动作成功为止。若在等待锁定的过程中被信号中断时，会立即返回-1，错误代码为EINTR。参数lock指针为flock 结构指针，定义如下
struct flcok
{
short int l_type; // 锁定的状态
	short int l_whence;//决定l_start位置
	off_t l_start; //锁定区域的开头位置
	off_t l_len; //锁定区域的大小
	pid_t l_pid; //锁定动作的进程
};
l_type 有三种状态 :
   F_RDLCK 建立一个供读取用的锁定
   F_WRLCK 建立一个供写入用的锁定
   F_UNLCK 删除之前建立的锁定
l_whence 也有三种方式 :
   SEEK_SET 以文件开头为锁定的起始位置。
   SEEK_CUR 以目前文件读写位置为锁定的起始位置
   SEEK_END 以文件结尾为锁定的起始位置。

返回值  成功则返回0，若有错误则返回 - 1，错误原因存于errno.

	*/

	//F_SETFL 设置文件描述词状态旗标，参数arg为新旗标，
	//但只允许O_APPEND、O_NONBLOCK和O_ASYNC位的改变，其他位的改变
	//将不受影响。

	//F_GETFL 取得文件描述词状态旗标，此旗标为open（）的参数flags。

	//设置文件描述符的状态旗标
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0)| O_NONBLOCK);
    printf("fd added to epoll!\n\n");
}

#endif // CHATROOM_COMMON_H