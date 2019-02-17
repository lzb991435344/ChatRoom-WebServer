/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#ifndef M_MAKE_FILE_H_INCLUDED
#define M_MAKE_FILE_H_INCLUDED

#include "km_head.h"
#include "..\\share\\event.h"

int make_file_initialize(HWND hWnd);
int make_file(char *addr, char *port, char *server, char *server_name);

#endif // M_MAKE_FILE_H_INCLUDED
