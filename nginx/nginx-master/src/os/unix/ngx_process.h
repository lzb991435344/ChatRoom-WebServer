
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_PROCESS_H_INCLUDED_
#define _NGX_PROCESS_H_INCLUDED_


#include <ngx_setaffinity.h>
#include <ngx_setproctitle.h>


typedef pid_t       ngx_pid_t;

#define NGX_INVALID_PID  -1

typedef void (*ngx_spawn_proc_pt) (ngx_cycle_t *cycle, void *data);

typedef struct {
    ngx_pid_t           pid;//进程的id号
    int                 status;//通过waitpid系统调用获取到的进程状态
    
    //通过socketpair系统调用产生的用于进程间通信的一对socket,用于
    //相互通信。
    ngx_socket_t        channel[2];

    //子进程的工作循环
    ngx_spawn_proc_pt   proc;

    //proc的第二个参数,可能需要传入一些数据
    void               *data;

    //进程的名称
    char               *name;

    //标识位

    unsigned            respawn:1;
    unsigned            just_spawn:1;
    unsigned            detached:1;
    unsigned            exiting:1;
    unsigned            exited:1;
} ngx_process_t;


typedef struct {
    char         *path;
    char         *name;
    char *const  *argv;
    char *const  *envp;
} ngx_exec_ctx_t;


#define NGX_MAX_PROCESSES         1024

//子进程退出时,父进程不会再次创建(在创建cache loader process时使用) 
#define NGX_PROCESS_NORESPAWN     -1

//区别旧/新进程的标识位
#define NGX_PROCESS_JUST_SPAWN    -2

//子进程异常退出时,父进程重新生成子进程的标识位
#define NGX_PROCESS_RESPAWN       -3

//区别旧/新进程的标识位
#define NGX_PROCESS_JUST_RESPAWN  -4

//热代码替换,父、子进程分离的标识位
#define NGX_PROCESS_DETACHED      -5


#define ngx_getpid   getpid
#define ngx_getppid  getppid

#ifndef ngx_log_pid
#define ngx_log_pid  ngx_pid
#endif


ngx_pid_t ngx_spawn_process(ngx_cycle_t *cycle,
    ngx_spawn_proc_pt proc, void *data, char *name, ngx_int_t respawn);
ngx_pid_t ngx_execute(ngx_cycle_t *cycle, ngx_exec_ctx_t *ctx);
ngx_int_t ngx_init_signals(ngx_log_t *log);
void ngx_debug_point(void);


#if (NGX_HAVE_SCHED_YIELD)
#define ngx_sched_yield()  sched_yield()
#else
#define ngx_sched_yield()  usleep(1)
#endif


extern int            ngx_argc;
extern char         **ngx_argv;
extern char         **ngx_os_argv;

extern ngx_pid_t      ngx_pid;
extern ngx_pid_t      ngx_parent;
extern ngx_socket_t   ngx_channel;

//当前操作的进程在ngx_processes数组中的下标
extern ngx_int_t      ngx_process_slot;

//ngx_processes数组中有意义的ngx_processes_t元素中最大的下标
extern ngx_int_t      ngx_last_process;

//存储所有子进程信息的数组
extern ngx_process_t  ngx_processes[NGX_MAX_PROCESSES];


#endif /* _NGX_PROCESS_H_INCLUDED_ */
