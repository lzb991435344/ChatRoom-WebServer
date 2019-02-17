/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#ifndef EVENT_M_FRAME_SCREEN_H_INCLUDED
#define EVENT_M_FRAME_SCREEN_H_INCLUDED

#include "km_head.h"
#include "..\\share\\event.h"

typedef struct tag_screen_ver{
    unsigned short wVersion;
    unsigned short wSystemVersion;
    unsigned long c_type;
    unsigned short  wXscreen;
    unsigned short  wYscreen;
}screen_ver,*pscreen_ver;

int initialize_km_screen_event();
int event_m_screen(pio_socket i_socket,pm_frame_screen pframe_screen, pscreen_ver pc_ver);

int event_m_screen_count(pio_socket i_socket, char *buffer, int lengthdd);
int event_m_screen_frame(pio_socket i_socket, char *buffer, int lengthdd);

#endif // EVENT_M_FRAME_SCREEN_H_INCLUDED
