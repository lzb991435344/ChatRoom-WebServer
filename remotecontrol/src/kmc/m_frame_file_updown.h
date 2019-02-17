/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#ifndef CLIENT_LIST_FILE_GUI_H_INCLUDED
#define CLIENT_LIST_FILE_GUI_H_INCLUDED

#include "km_head.h"

typedef int (* insert_file_list)(HWND hWndListFile,p_up_down_file,int col,int row);

typedef struct tag_client_list_file_col{
    enum  list_col_state    col_state;
    char                    col_name[64];
    int                     width;
    insert_file_list        insert_file_list_text;
}CLIENT_LIST_FILE_COL;

int insert_client_list_file_item(pio_socket pio_handle);
int update_client_list_file_item(p_up_down_file pclient_file);

int client_insert_list_file_ip(HWND hWndListFile,pio_socket i_socket,int col,int row);

int insert_file_list_file_max_num(HWND hWndListFile,p_up_down_file,int col,int row);
int insert_file_list_file_cur_num(HWND hWndListFile,p_up_down_file,int col,int row);
int insert_file_list_file_max_size(HWND hWndListFile,p_up_down_file,int col,int row);
int insert_file_list_file_cur_size(HWND hWndListFile,p_up_down_file,int col,int row);
int insert_file_list_file_remote_path(HWND hWndListFile,p_up_down_file,int col,int row);
int insert_file_list_file_local_path(HWND hWndListFile,p_up_down_file,int col,int row);

int  draw_client_list_file_col_width(HWND hWndListFile,int right);
int  draw_client_list_file_col(HWND hWndListFile);

#endif // CLIENT_LIST_FILE_GUI_H_INCLUDED
