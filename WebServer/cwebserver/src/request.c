#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>
#include "request.h"
#include "http_header.h"

// 初始化HTTP请求
http_request* http_request_init() {
    http_request *req;//请求结构体指针
	
    //分配内存空间
    req = malloc(sizeof(*req));
    req->content_length = 0;
    req->version = HTTP_VERSION_UNKNOWN;
    req->content_length = -1;

	//递归函数,得到HTTP的头部
    req->headers = http_headers_init();
    return req; 
}

// 释放HTTP请求,参数是一个指向请求结构体的指针
void http_request_free(http_request *req) {
    if (!req) return;

	//释放头
	http_headers_free(req->headers);
	free(req);
}


// 一直读取字符直到遇到delims字符串，将先前读取的返回
// 可以用在解析HTTP请求时获取HTTP方法名（第一段字符串）
static char* match_until(char **buf, const char *delims) {
    char *match = *buf;
    char *end_match = match + strcspn(*buf, delims);
    char *end_delims = end_match + strspn(end_match, delims);

    for (char *p = end_match; p < end_delims; p++) {
        *p = '\0';
    }

    *buf = end_delims;

    return (end_match != end_delims) ? match : NULL;
}

// 根据字符串获得请求中的HTTP方法，目前只支持GET和HEAD
static http_method get_method(const char *method) {
    if (strcasecmp(method, "GET") == 0)
        return HTTP_METHOD_GET;
    else if (strcasecmp(method, "HEAD") == 0)
        return HTTP_METHOD_HEAD;
    else if(strcasecmp(method, "POST") == 0 || strcasecmp(method, "PUT") == 0)
        return HTTP_METHOD_NOT_SUPPORTED;
    
    return HTTP_METHOD_UNKNOWN;
}

// 解析URI，获得文件在服务器上的绝对路径
static int resolve_uri(char *resolved_path, char *root, char *uri) {
    int ret = 0;
    string *path = string_init_str(root);
    string_append(path, uri);

    char *res = realpath(path->ptr, resolved_path);
    
    if (!res) {
        ret = -1;
        goto cleanup;
    }

    size_t resolved_path_len = strlen(resolved_path);
    size_t root_len = strlen(root);

    if (resolved_path_len < root_len) {
        ret = -1;
    } else if (strncmp(resolved_path, root, root_len) != 0) {
        ret = -1;
    } else if(uri[0] == '/' && uri[1] == '\0') {
        strcat(resolved_path, "/index.html");
    }

cleanup:
    string_free(path);
    return ret;
}

/*
 * 设置状态码
 */
static void try_set_status(connection *con, int status_code) {
    if (con->status_code == 0)
        con->status_code = status_code;
}

// 解析HTTP 请求消息
void http_request_parse(server *serv, connection *con) {
    http_request *req = con->request;
    char *buf = con->recv_buf->ptr;

    req->method_raw = match_until(&buf, " ");
    
    if (!req->method_raw) {
        con->status_code = 400;
        return;
    }

    // 获得HTTP方法
    req->method = get_method(req->method_raw);

    if (req->method == HTTP_METHOD_NOT_SUPPORTED) {
        try_set_status(con, 501);
    } else if(req->method == HTTP_METHOD_UNKNOWN) {
        con->status_code = 400;
        return;
    }

    // 获得URI
    req->uri = match_until(&buf, " \r\n");

    if (!req->uri) {
        con->status_code = 400;
        return;
    }

    /*
     * 判断访问的资源是否在服务器上
     *
     */
    if (resolve_uri(con->real_path, serv->conf->doc_root, req->uri) == -1) {
        try_set_status(con, 404);
    } 
    
    // 如果版本为HTTP_VERSION_09立刻退出
    if (req->version == HTTP_VERSION_09) {
        try_set_status(con, 200);
        req->version_raw = "";
        return;
    }

    // 获得HTTP版本
    req->version_raw = match_until(&buf, "\r\n");

    if (!req->version_raw) {
        con->status_code = 400;
        return;
    }

    // 支持HTTP/1.0或HTTP/1.1
    if (strcasecmp(req->version_raw, "HTTP/1.0") == 0) {
        req->version = HTTP_VERSION_10;
    } else if (strcasecmp(req->version_raw, "HTTP/1.1") == 0) {
        req->version = HTTP_VERSION_11;
    } else {
        try_set_status(con, 400);
    }

    if (con->status_code > 0)
        return;

    // 解析HTTP请求头部

    char *p = buf;
    char *endp = con->recv_buf->ptr + con->request_len;

    while (p < endp) {
        const char *key = match_until(&p, ": ");
        const char *value = match_until(&p, "\r\n");

        if (!key || !value) {
            con->status_code = 400;
            return;
        }

        http_headers_add(req->headers, key, value);
    }

    con->status_code = 200;
}

/*
 * 根据HTTP协议验证HTTP请求是否合法
 * 
 * 返回值
 * -1 不合法
 * 0 不完整
 * 1 完整
 */
//参数是:客户端连接的指针
int http_request_complete(connection *con) {
    char c;
	//循环遍历接收队列中的字符
    for (; con->request_len < con->recv_buf->len; con->request_len++) {
		//得到http头的数据
        c = con->recv_buf->ptr[con->request_len];

		//多路case判定接收的状态
        switch (con->recv_state) {
            case HTTP_RECV_STATE_WORD1:
                if (c == ' ')//字符是空格时设置头部
                    con->recv_state = HTTP_RECV_STATE_SP1;
                else if (!isalpha(c))//不是空格
                    return -1;
            break;

            case HTTP_RECV_STATE_SP1:
                if (c == ' ')
                    continue;
				//换行，空格等
                if (c == '\r' || c == '\n' || c == '\t')
                    return -1;
                con->recv_state = HTTP_RECV_STATE_WORD2;
            break;

            case HTTP_RECV_STATE_WORD2:
                if (c == '\n') {
                    con->request_len++;
                    con->request->version = HTTP_VERSION_09;
                    return 1;
                } else if (c == ' ')
                    con->recv_state = HTTP_RECV_STATE_SP2;
                else if (c == '\t')
                    return -1;
            break;

            case HTTP_RECV_STATE_SP2:
                if (c == ' ')
                    continue;    
                if (c == '\r' || c == '\n' || c == '\t')
                    return -1;
                con->recv_state = HTTP_RECV_STATE_WORD3;
            break;

            case HTTP_RECV_STATE_WORD3:
                if (c == '\n')
                    con->recv_state = HTTP_RECV_STATE_LF;
                else if (c == ' ' || c == '\t')//空格或横向跳到下一制表符的位置
                    return -1;
            break;

            case HTTP_RECV_STATE_LF:
                if (c == '\n') {
                    con->request_len++;
                    return 1;
                } else if (c != '\r')//回车
                    con->recv_state = HTTP_RECV_STATE_LINE;
            break;

            case HTTP_RECV_STATE_LINE:
                if (c == '\n')//回车换行
                    con->recv_state = HTTP_RECV_STATE_LF;
            break;
        }
    }

    return 0;
}
