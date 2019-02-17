/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#ifndef FRAME_TOOLS_H_INCLUDED
#define FRAME_TOOLS_H_INCLUDED

#include "km_socket.h"

int open_file_path(char *path, char *file, HWND hWnd);
int save_file_path(char *path, HWND hWnd);
int open_file_directory(char *path, char *title, HWND hWnd);

int get_combox_text(HWND,int,char*);
int insert_combo_item(HWND hWnd,int iItem,int iIndent,int iImage,char *lpStr);
int set_combo_item(HWND hWnd,int iItem,int iIndent,int iImage,char *lpStr);
int insert_list_item(HWND hWndList,char *str,int iItem,int image);

int insert_list_column_text(HWND hList,int col,int width,char *pszText);

pio_socket get_frame_io_socket(HWND hWnd);

void SetWindowFont(HWND hWnd);

#endif // FRAME_TOOLS_H_INCLUDED
