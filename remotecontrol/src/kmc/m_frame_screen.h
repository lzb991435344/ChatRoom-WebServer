/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#ifndef M_FRAME_SCREEN_H_INCLUDED
#define M_FRAME_SCREEN_H_INCLUDED

#include "km_head.h"
#include "..\\share\\event.h"

#define     WM_R_SCREEN         WM_APP+1
#define     WM_R_SCREEN_COUNT   WM_APP+2

int initialize_frame_screen();
LRESULT __stdcall frame_screen(HWND,UINT,WPARAM,LPARAM);
int create_frame_screen(HWND hWndRoot, pio_socket i_socket);

int free_frame_screen(pio_socket i_socket);
int destroy_frame_screen(pio_socket i_socket);
int frame_screen_xy(pm_frame_screen pframe_screen, int x, int y);

#endif // M_FRAME_SCREEN_H_INCLUDED
