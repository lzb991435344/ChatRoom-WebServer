/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#ifndef MAINFRAME_H_INCLUDED
#define MAINFRAME_H_INCLUDED
#include "../share/head.h"

int initialize_main_frame();

LRESULT __stdcall main_frame_proc(HWND,UINT,WPARAM,LPARAM);
LRESULT __stdcall main_frame_event_notify_tools(HWND,UINT,WPARAM,LPARAM);
LRESULT __stdcall main_frame_event_notify_listview(HWND,UINT,WPARAM,LPARAM);
LRESULT __stdcall main_frame_right_menu(HWND,UINT,WPARAM,LPARAM);


#endif // MAINFRAME_H_INCLUDED
