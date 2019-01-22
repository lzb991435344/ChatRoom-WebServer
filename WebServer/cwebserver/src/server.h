#ifndef _BASE_H_
#define _BASE_H_

#include <unistd.h>
#include <netdb.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include "stringutils.h"
#include "config.h"

// HTTP请求的方法
typedef enum {
    HTTP_METHOD_UNKNOWN = -1,
    HTTP_METHOD_NOT_SUPPORTED = 0,
    HTTP_METHOD_GET = 1,
    HTTP_METHOD_HEAD = 2
} http_method;

// HTTP协议版本
typedef enum {
    HTTP_VERSION_UNKNOWN,
    HTTP_VERSION_09,
    HTTP_VERSION_10,
    HTTP_VERSION_11
} http_version;

// HTTP头部中使用的键值对
typedef struct {
    string *key;
    string *value;
} keyvalue;

// HTTP头部，包含若干个键值对，键值对的数量和头部长度
typedef struct {
    keyvalue *ptr;//键值对结构体的指针
    size_t len;//头部的长度
    size_t size;//键值对的数量
} http_headers;

// HTTP请求结构体，包含HTTP方法，版本，URI，HTTP头，内容长度
typedef struct {
	//方法
    http_method method;

	//版本
    http_version version;
    
	char *method_raw;
    char *version_raw;

	//URI
    char *uri;

	//HTTP头
    http_headers *headers;

	//内容长度
    int content_length;
} http_request;

// HTTP响应结构体，包含内容长度，内容，HTTP头部
typedef struct {
	//内容的长度
    int content_length;

	//实体的字符串指针
    string *entity_body;

	//指向HTTP头部的指针
    http_headers *headers;
} http_response;

//定义接收的长度
typedef enum {
    HTTP_RECV_STATE_WORD1,
    HTTP_RECV_STATE_WORD2,
    HTTP_RECV_STATE_WORD3,
    HTTP_RECV_STATE_SP1,
    HTTP_RECV_STATE_SP2,
    HTTP_RECV_STATE_LF,
    HTTP_RECV_STATE_LINE
} http_recv_state;

// 客户端连接结构体
typedef struct {
    // 客户端连接的socket
    int sockfd;

    // 状态码
    int status_code;

    // 接收队列，字符串数组
    string *recv_buf;

    // HTTP请求
    http_request *request;

    // HTTP响应
    http_response *response;

    // 接收状态
    http_recv_state recv_state;

    // 客户端地址信息
    struct sockaddr_in addr;

    // 请求长度
    size_t request_len;

    // 请求文件的真实路径
    char real_path[PATH_MAX];
} connection;

// 服务器结构体
typedef struct {
    // 包含日志文件
    FILE *logfp;

    // 监听的socket
    int sockfd;

    // 监听的端口
    short port;

    // 是否使用日志文件
    int use_logfile;

    // 是否以守护进程的方式启动
    int is_daemon;

    // 是否chroot
    int do_chroot;

    // 配置信息 
    config *conf;//记得释放该结构体的内存空间
} server;

#endif
