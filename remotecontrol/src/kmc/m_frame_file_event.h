/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#ifndef EVENT_M_FRAME_FILE_H_INCLUDED
#define EVENT_M_FRAME_FILE_H_INCLUDED

#include "km_head.h"
#include "..\\share\\event.h"
#include "m_frame_file.h"
#include "km_socket.h"

int initialize_km_file_event();
int event_m_file_drive(pio_socket i_socket,char *buffer,int length);
int event_m_file_net(pio_socket i_socket,char *buffer,int length);

int event_m_file_net_resource(pio_socket i_socket,char *buffer,int length);
int event_m_file_directory(pio_socket i_socket,char *buffer,int length);

int event_m_file_up_down(pio_socket i_socket);
int event_m_file_up(pio_socket i_socket,char *buffer,int length);

int event_m_file_down_name(pio_socket i_socket,char *buffer,int length);
int event_m_file_down(pio_socket i_socket,char *buffer,int length);

int event_m_directory_up(pio_socket i_socket,char *buffer,int length);
int event_m_enum_up_directory(p_up_down_file p_file,char *startpath,char *findpath);

int event_m_directory_down(pio_socket i_socket,char *buffer,int length);
int event_m_directory_new(pio_socket i_socket,char *buffer,int length);

int destroy_m_file_up_down(pio_socket i_socket);
#endif // EVENT_M_FRAME_FILE_H_INCLUDED
