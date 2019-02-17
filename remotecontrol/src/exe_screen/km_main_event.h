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

int kms_main_screen_frame(pio_socket i_socket, char *buffer, int length);
int kms_main_screen_frame_exit(pio_socket i_socket, char *buffer, int length);
int screen_socket_close(pio_socket i_socket);
int kms_main_screen_connect(pio_socket i_socket, char *buffer, int length);

int kms_main_screen_mouse_click(pio_socket i_socket, char *buffer, int length);
int kms_main_screen_mouse_right(pio_socket i_socket, char *buffer, int length);
int kms_main_screen_char(pio_socket i_socket, char *buffer, int length);
int kms_main_screen_count(pio_socket i_socket, char *buffer, int length);
int kms_main_screen_mouse_click_up(pio_socket i_socket, char *buffer, int length);
int kms_main_screen_mouse_right_up(pio_socket i_socket, char *buffer, int length);

#endif // KM_MAIN_EVENT_H_INCLUDED
