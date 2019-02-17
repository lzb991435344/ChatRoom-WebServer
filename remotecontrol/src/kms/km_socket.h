/*
��������:����
��������:2010.8
Զ�̿��Ʊ������Ⱥ:30660169,6467438
*/

#ifndef KM_SOCKET_H_INCLUDED
#define KM_SOCKET_H_INCLUDED

#include "km_head.h"

#include <stdio.h>
#include <stdlib.h>
#include <process.h>

int kms_main_initialize();

unsigned long str_ip_to_long(char * ip);

unsigned __stdcall kms_sock_loop(void *param);

int kms_sock_re_connect(pio_socket i_socket);
int kms_sock_recv(pio_socket i_socket);
int kms_sock_close(pio_socket i_socket);
int kms_sock_recv_header(pio_socket i_socket);

int kms_sock_send(pio_socket i_socket,unsigned char major_cmd,unsigned char minor_cmd,unsigned long crc32,char *buffer,int length);

int kms_sock_connect(char *host,int port,f_work f_connect_event,kms_sock_func f_exit_event,void *extend);
int kms_sock_ret_connect(char *host,int port,f_work f_connect_event);

extern pshare_main g_share_main;

#endif // KM_SOCKET_H_INCLUDED
