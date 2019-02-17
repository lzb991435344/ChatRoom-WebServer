/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#ifndef M_FRAME_FILE_H_INCLUDED
#define M_FRAME_FILE_H_INCLUDED

#include "km_head.h"
#include "..\\share\\event.h"

#define     WM_RECV_DRIVER      WM_APP+1
#define     WM_RECV_NET         WM_APP+2
#define     WM_RECV_DIRECTORY   WM_APP+3

#define LVM_GETSELECTIONMARK	(LVM_FIRST+66)

typedef struct tag_file_ico{
    char name[12];
    int  image;
}FILE_ICO,*PFILE_ICO;

LRESULT __stdcall frame_file(HWND hWnd,UINT Message,WPARAM wParam,LPARAM lParam);
LRESULT __stdcall frame_file_list(HWND hWnd,UINT Message,WPARAM wParam,LPARAM lParam);

int get_list_file_img(char *name);
int initialize_frame_file();

int initialize_frame_file_tools(HWND hWnd, pm_frame_file_control pm_controls);

int create_frame_file(HWND hWndRoot,pio_socket i_socket);
int destroy_frame_file(pio_socket i_socket);

int frame_combox_event(HWND,UINT,WPARAM,LPARAM);
int frame_menu_event(HWND,UINT,WPARAM,LPARAM);
int frame_list_menu_event(HWND,UINT,WPARAM,LPARAM);

int view_combox_disk(pm_frame_file pm_frame_file);
int view_list_net(pio_socket i_socket,char *buffer);
int view_list_directory(pio_socket i_socket,char *buffer);

int insert_foo_item(pkm_client pclient, pm_frame_file_control pm_controls, char *pstrItem);

#endif // M_FRAME_FILE_H_INCLUDED
