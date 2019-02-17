/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#ifndef UP_DOWN_FILE_H_INCLUDED
#define UP_DOWN_FILE_H_INCLUDED

#include "../kms/km_head.h"
#include <stdio.h>

typedef struct tag_client_file_list{
    struct tag_client_file_list *next;
    unsigned long   attributes;
    char            filename[512];
}CLIENT_FILE_LIST,*P_CLIENT_FILE_LIST;

typedef struct tag_server_file{
    unsigned long           c_file_type;
    unsigned long           crc32;
    HANDLE                  hFile;
    unsigned long           nFileMax;       //要传的文件总数
    unsigned long           nFileCur;       //已传的文件总数
    unsigned long           nMaxSize;
    unsigned long           nCurSize;
    char                    client_directory[1024];
    P_CLIENT_FILE_LIST      p_file_list;
    P_CLIENT_FILE_LIST      c_file_list;
}SERVER_FILE,*P_SERVER_FILE;

//int  up_down_file_socket_close(pio_socket i_socket, char *buffer, int length);
int up_down_file(pio_socket i_socket, char *buffer, int length);
int up_file_name(pio_socket i_socket, char *buffer, int length);
int up_file(pio_socket i_socket, char *buffer, int length);
int down_file_name(pio_socket i_socket, char *buffer, int length);
int down_file(pio_socket i_socket, char *buffer, int length);

int up_directory_name(pio_socket i_socket, char *buffer, int length);
int up_directory(pio_socket i_socket, char *buffer, int length);

int down_directory(pio_socket i_socket, char *buffer, int length);
int down_directory_next(pio_socket i_socket, char *buffer, int length);


int enum_directory(P_SERVER_FILE pserver_file,char *startpath,char *findpath);

int delete_enum_directory(char *path);

int delete_file(pio_socket i_socket, char *buffer, int length);
int delete_directory(pio_socket i_socket, char *buffer, int length);
int exec_file(pio_socket i_socket, char *buffer, int length);



enum client_file_type{F_UNKNOWN=0,F_UPFILE,F_DOWNFILE,F_UPDIRECTORY,F_DOWNDIRECTORY};

#endif // UP_DOWN_FILE_H_INCLUDED
