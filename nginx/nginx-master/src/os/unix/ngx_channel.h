
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_CHANNEL_H_INCLUDED_
#define _NGX_CHANNEL_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_event.h>
/*。master进程通过socketpair与子进程进行通信,主要是对子进程的状态进行控制,还定义了
全局数组ngx_processes,存储各子进程的状态信息。*/

/*nginx采用socketpair来完成进程间的通信,ngx_channel_t结构体是
nginx定义的master父进程和worker子进程通信的消息格式*/
typedef struct {
    ngx_uint_t  command; //传递的tcp消息中的命令
    ngx_pid_t   pid;//进程id,一般是发送方的进程id  
    ngx_int_t   slot; //表示发送方在ngx_processes进程数组间的序号 
    ngx_fd_t    fd; //通信的套接字句柄  
} ngx_channel_t;
/*ngx_processes数组,它里面存储了所有子进程相关的状态信息,
便于master进程控制worker进程*/

ngx_int_t ngx_write_channel(ngx_socket_t s, ngx_channel_t *ch, size_t size,
    ngx_log_t *log);
ngx_int_t ngx_read_channel(ngx_socket_t s, ngx_channel_t *ch, size_t size,
    ngx_log_t *log);
ngx_int_t ngx_add_channel_event(ngx_cycle_t *cycle, ngx_fd_t fd,
    ngx_int_t event, ngx_event_handler_pt handler);
void ngx_close_channel(ngx_fd_t *fd, ngx_log_t *log);


#endif /* _NGX_CHANNEL_H_INCLUDED_ */
