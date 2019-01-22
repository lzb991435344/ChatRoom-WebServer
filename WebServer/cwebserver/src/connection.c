#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "log.h"
#include "connection.h"
#include "request.h"
#include "response.h"
#include "stringutils.h"

// 关闭连接,参数传一个指向连接的指针
void connection_close(connection *con) {
    if (!con) return;

    // 释放连接对应的请求和相应
    http_request_free(con->request);
    http_response_free(con->response);
    
    // 释放客户端连接中的缓存
    string_free(con->recv_buf);
    
    // 关闭连接socket
    if (con->sockfd > -1)
        close(con->sockfd);

    free(con);
}

/*
 * 等待并接受新的连接
 */
connection* connection_accept(server *serv) {
    struct sockaddr_in addr;
    connection *con;
    int sockfd;
    socklen_t addr_len = sizeof(addr);

    // accept() 接受新的连接
    sockfd = accept(serv->sockfd, (struct sockaddr *) &addr, &addr_len);
    
    if (sockfd < 0) {
        log_error(serv, "accept: %s", strerror(errno));
        perror("accept");
        return NULL;
    }

    // 创建连接结构实例
    con = malloc(sizeof(*con));

    // 初始化连接结构
    con->status_code = 0;
    con->request_len = 0;
    con->sockfd = sockfd;
    con->real_path[0] = '\0';

    con->recv_state = HTTP_RECV_STATE_WORD1;//状态
    con->request = http_request_init();//请求
    con->response = http_response_init();//响应
    con->recv_buf = string_init();//接收队列
    memcpy(&con->addr, &addr, addr_len);//对嵌套的客户端的结构体赋值

    return con;
}

/*
 * HTTP请求处理函数
 * - 1.从socket中读取数据并解析HTTP请求
 * - 2.解析请求
 * - 3.发送响应
 * - 4.记录请求日志
 */
int connection_handler(server *serv, connection *con) {
    char buf[512];
    int nbytes;
    int ret;

	//打印客户端连接的socket
    printf("socket: %d\n", con->sockfd);
	/**
	 函数原型int recv( _In_ SOCKET s, _Out_ char *buf, _In_ int len, _In_ int flags);
	返回值：recv函数返回其实际copy的字节数。如果recv在copy时出错，那么它返回SOCKET_ERROR；
	如果recv函数在等待协议接收数据时网络中断了，那么它返回0。
    注意：在Unix系统下，如果recv函数在等待协议接收数据时网络断开了，那么调用recv的进程会接
    收到一个SIGPIPE信号，进程对该信号的默认处理是进程终止。
	*/

    while ((nbytes = recv(con->sockfd, buf, sizeof(buf), 0)) > 0) {
        string_append_len(con->recv_buf, buf, nbytes);

		//检查请求是否合理
        if (http_request_complete(con) != 0)//出错，退出当前循环
            break;
    }

    if (nbytes <= 0) {
        ret = -1;
		//网络中断
        if (nbytes == 0) {//socket关闭
            printf("socket %d closed\n", con->sockfd);
            log_info(serv, "socket %d closed", con->sockfd);
        
        } else if (nbytes < 0) {//没有数据，接收数据中断
            perror("read");
            log_error(serv, "read: %s", strerror(errno));
        }
    } else {//有数据
        ret = 0;
    }

    http_request_parse(serv, con); //解析HTTP
    http_response_send(serv, con);//发送响应
    log_request(serv, con);//记录http请求

    return ret;
}
