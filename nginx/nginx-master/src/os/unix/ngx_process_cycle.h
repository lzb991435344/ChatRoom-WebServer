
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_PROCESS_CYCLE_H_INCLUDED_
#define _NGX_PROCESS_CYCLE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


#define NGX_CMD_OPEN_CHANNEL   1 	//打开,使用这种方式通信前必须发送的命令
#define NGX_CMD_CLOSE_CHANNEL  2	//关闭套接字
#define NGX_CMD_QUIT           3	//要求接收方正常退出进程
#define NGX_CMD_TERMINATE      4	//要求接收方强制结束进程
#define NGX_CMD_REOPEN         5	//要求接收方重新打开进程已经打开过的文件


#define NGX_PROCESS_SINGLE     0
#define NGX_PROCESS_MASTER     1
#define NGX_PROCESS_SIGNALLER  2
#define NGX_PROCESS_WORKER     3
#define NGX_PROCESS_HELPER     4


typedef struct {
    ngx_event_handler_pt       handler;
    char                      *name;
    ngx_msec_t                 delay;
} ngx_cache_manager_ctx_t;


void ngx_master_process_cycle(ngx_cycle_t *cycle);
void ngx_single_process_cycle(ngx_cycle_t *cycle);


extern ngx_uint_t      ngx_process;
extern ngx_uint_t      ngx_worker;
extern ngx_pid_t       ngx_pid;
extern ngx_pid_t       ngx_new_binary;
extern ngx_uint_t      ngx_inherited;
extern ngx_uint_t      ngx_daemonized;
extern ngx_uint_t      ngx_exiting;

extern sig_atomic_t    ngx_reap;
extern sig_atomic_t    ngx_sigio;
extern sig_atomic_t    ngx_sigalrm;
extern sig_atomic_t    ngx_quit;
extern sig_atomic_t    ngx_debug_quit;
extern sig_atomic_t    ngx_terminate;
extern sig_atomic_t    ngx_noaccept;
extern sig_atomic_t    ngx_reconfigure;
extern sig_atomic_t    ngx_reopen;
extern sig_atomic_t    ngx_change_binary;


#endif /* _NGX_PROCESS_CYCLE_H_INCLUDED_ */
