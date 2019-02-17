/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include "../kms/km_head.h"

typedef struct tag_ddos_task{
    int listview;
    int status;
    int type;
    int thread;
    char addr[512];
    struct tag_ddos_task *next;
}ddos_task, *pddos_task;

typedef struct tag_task_list{
    ddos_task d_task;
    HANDLE *phandle;
    struct sockaddr_in client_in;
    char s_buf[4096];
    int s_len;
    int status;
    struct tag_task_list *next;
}task_list, *ptask_list;

#endif // MAIN_H_INCLUDED
