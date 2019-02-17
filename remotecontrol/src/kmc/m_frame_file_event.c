/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "m_frame_file_event.h"
#include "m_frame_file_updown.h"

kms_event_func   file_event_funcs[] =
{
    event_m_file_drive,
    event_m_file_net,
    event_m_file_net_resource,
    event_m_file_directory,
    event_m_file_up,
    event_m_file_down_name,
    event_m_file_down,
    event_m_directory_up,
    event_m_directory_down,
    event_m_directory_new
};

int initialize_km_file_event()
{
    g_share_main->event_funcs[CLIENT_FILE].count          = sizeof(file_event_funcs)/sizeof(kms_event_func);
    g_share_main->event_funcs[CLIENT_FILE].event_funcs    = file_event_funcs;
    return 0;
}

int event_m_file_drive(pio_socket i_socket,char *buffer,int lengthdd)
{
    unsigned long length;
    length      = lengthdd - sizeof(io_data_header);
    pkm_client pclient = (pkm_client)i_socket->extend;

    pclient->frame_file.m_data.drivenum = length/sizeof(DRIVE);
    if (pclient->frame_file.m_data.drive != NULL)
        free(pclient->frame_file.m_data.drive);
    pclient->frame_file.m_data.drive = malloc(length);

    memcpy(pclient->frame_file.m_data.drive, buffer + sizeof(io_data_header), length);
    SendMessage(pclient->frame_file.m_controls.hWnd, WM_RECV_DRIVER, 0, (LPARAM)&pclient->frame_file);
    return SOCK_LOOP_RECV_HEADER;
}

int event_m_file_net(pio_socket i_socket,char *buffer,int length)
{
    pkm_client pclient = (pkm_client)i_socket->extend;
    SendMessage(pclient->frame_file.m_controls.hWnd, WM_RECV_NET, (WPARAM)i_socket, (LPARAM)buffer);
    return SOCK_LOOP_RECV_HEADER;
}

int event_m_file_net_resource(pio_socket i_socket,char *buffer,int length)
{
    return SOCK_LOOP_RECV_HEADER;
}

int event_m_file_directory(pio_socket i_socket,char *buffer,int length)
{
    pkm_client pclient = (pkm_client)i_socket->extend;
    SendMessage(pclient->frame_file.m_controls.hWnd, WM_RECV_DIRECTORY, (WPARAM)i_socket, (LPARAM)buffer);
    return SOCK_LOOP_RECV_HEADER;
}

int event_m_file_up_down(pio_socket i_socket)  //判断当前的任务是上传还是下载
{
    p_up_down_file pclient_file = (p_up_down_file)i_socket->extend;
    if (pclient_file->f_op_type == F_UPFILE){
        pclient_file->hFile = CreateFile(pclient_file->server_directory, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        pclient_file->nMaxSize = GetFileSize(pclient_file->hFile, NULL);
        pclient_file->nCurSize = 0;
        if (pclient_file->hFile != NULL){
            kms_sock_send(i_socket, CLIENT_FILE, CLIENT_RECV_UP_FILE_NAME, pclient_file->nMaxSize, pclient_file->client_directory, strlen(pclient_file->client_directory));
        }else{
          printf("open:%s error\n", pclient_file->server_directory);
        }
    }
    else if (pclient_file->f_op_type == F_DOWNFILE){
        kms_sock_send(i_socket, CLIENT_FILE, CLIENT_RECV_DOWN_FILE_NAME, 0, pclient_file->client_directory, strlen(pclient_file->client_directory));
    }
    else if (pclient_file->f_op_type == F_UPDIRECTORY){
        pclient_file->nFileMax = event_m_enum_up_directory(pclient_file, "\\", pclient_file->server_directory);
        kms_sock_send(i_socket, CLIENT_FILE, CLIENT_UP_DIRECTORY, pclient_file->nFileMax, pclient_file->client_directory, strlen(pclient_file->client_directory));
    }
    else if (pclient_file->f_op_type == F_DOWNDIRECTORY){
        printf("%s\n","F_DOWNDIRECTORY");
        CreateDirectory(pclient_file->server_directory, NULL);
        kms_sock_send(i_socket, CLIENT_FILE, CLIENT_DOWN_DIRECTORY, pclient_file->nFileMax, pclient_file->client_directory, strlen(pclient_file->client_directory));
    }else{
        printf("%s\n","else event_m_file_up_down");
        return 0;
    }

    insert_client_list_file_item(i_socket);
    return 1;
}

int event_m_file_up(pio_socket i_socket,char *buffer,int length)        //接受客户已传输的字节数,读取文件,然后发送
{
    DWORD dwSize = 0;
    pio_data_header    pi_data_header    = (pio_data_header)buffer;
    p_up_down_file     pclient_file        = (p_up_down_file)i_socket->extend;

    if (pclient_file->nCurSize != pi_data_header->crc32){
        return -1;
    }

    if (ReadFile(pclient_file->hFile, buffer, 65536, &dwSize, NULL)){
        pclient_file->nCurSize += dwSize;
        kms_sock_send(i_socket, CLIENT_FILE, CLIENT_RECV_FILE, 0, buffer, dwSize);
    }

    if (pclient_file->nCurSize == pclient_file->nMaxSize){
        pclient_file->nFileCur++;
        CloseHandle(pclient_file->hFile);
    }

    update_client_list_file_item(pclient_file);
    return SOCK_LOOP_RECV_HEADER;
}

int event_m_file_down_name(pio_socket i_socket,char *buffer,int length) //从客户端下载文件，接受长度
{
    char directory[1024]={0};

    pio_data_header pi_data_header = (pio_data_header)buffer;
    p_up_down_file p_client_file = (p_up_down_file)i_socket->extend;

    p_client_file->nMaxSize = pi_data_header->crc32;
    p_client_file->nCurSize = 0;

 //   printf("event_m_file_down_name:%s\n",p_client_file->server_directory);

    if (p_client_file->f_op_type == F_DOWNFILE){
        p_client_file->hFile = CreateFile(p_client_file->server_directory, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    }else if (p_client_file->f_op_type == F_DOWNDIRECTORY){
        sprintf(directory,"%s%s", p_client_file->server_directory, buffer + sizeof(io_data_header));
        p_client_file->hFile = CreateFile(directory, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    }
    if(p_client_file->hFile != NULL){
       kms_sock_send(i_socket, CLIENT_FILE, CLIENT_DOWN_FILE, p_client_file->nCurSize, NULL, 0);
    }
    return SOCK_LOOP_RECV_HEADER;
}

int event_m_file_down(pio_socket i_socket,char *buffer,int lengthdd)  //开始下载文件,然后写入
{
    DWORD dwSize = 0;
//  P_IO_DATA_HEADER p_io_data_header = (P_IO_DATA_HEADER)pio_data->buffer;
    p_up_down_file p_client_file = (p_up_down_file)i_socket->extend;

    int length = lengthdd - sizeof(io_data_header);
   // if(length > 0){
        WriteFile(p_client_file->hFile, buffer + sizeof(io_data_header), length, &dwSize, 0);
        p_client_file->nCurSize += length;

        if (p_client_file->nCurSize >= p_client_file->nMaxSize){

            CloseHandle(p_client_file->hFile);
            p_client_file->nFileCur++;

            if(p_client_file->f_op_type == F_DOWNDIRECTORY){
                kms_sock_send(i_socket, CLIENT_FILE, CLIENT_DIRECTORY_NEXT, p_client_file->nFileCur, NULL, 0);
            }
        }else{
           // printf("down_file %d\n",p_client_file->nCurSize);
            kms_sock_send(i_socket, CLIENT_FILE, CLIENT_DOWN_FILE, p_client_file->nCurSize, NULL, 0);
        }
    //}
    update_client_list_file_item(p_client_file);
    return SOCK_LOOP_RECV_HEADER;
}

int event_m_directory_up(pio_socket i_socket,char *buffer,int length)
{
    char path[1024]={0};
    pio_data_header pi_data_header = (pio_data_header)buffer;
    p_up_down_file pclient_file = (p_up_down_file)i_socket->extend;
    if (pi_data_header->crc32 == 0){
        pclient_file->c_op_file = pclient_file->h_op_file;
    }
    else{
        pclient_file->c_op_file = pclient_file->c_op_file->next;
    }

    if (pclient_file->c_op_file != NULL){
        if( pclient_file->c_op_file->attributes & FILE_ATTRIBUTE_DIRECTORY){
            pclient_file->nFileCur++;
            kms_sock_send(i_socket, CLIENT_FILE, CLIENT_NEW_DIRECTORY, 0, pclient_file->c_op_file->filename, strlen(pclient_file->c_op_file->filename));
        }
        else{
            sprintf(path, "%s%s", pclient_file->server_directory, pclient_file->c_op_file->filename);

            pclient_file->hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            pclient_file->nMaxSize = GetFileSize(pclient_file->hFile, NULL);
            pclient_file->nCurSize = 0;

            if (pclient_file->hFile != NULL){
                //memcpy(pio_data->curbuffer,pclient_file->c_file_list->filename,strlen(pclient_file->c_file_list->filename));
                kms_sock_send(i_socket, CLIENT_FILE, CLIENT_RECV_UP_FILE_NAME, pclient_file->nMaxSize, pclient_file->c_op_file->filename, strlen(pclient_file->c_op_file->filename));
            }
            else{
              printf("open:%s error\n", pclient_file->server_directory);
            //send_socket
            }
        }
    }
    else{
        closesocket(i_socket->socket);
    }
    return SOCK_LOOP_RECV_HEADER;
}

int event_m_directory_down(pio_socket i_socket,char *buffer,int length)
{
    pio_data_header pi_data_header = (pio_data_header)buffer;
    p_up_down_file pclient_file = (p_up_down_file)i_socket->extend;

    pclient_file->nFileMax = pi_data_header->crc32;
    kms_sock_send(i_socket, CLIENT_FILE, CLIENT_DIRECTORY_NEXT, 0, NULL, 0);

    //update_client_list_file_item(pclient_file);
    return SOCK_LOOP_RECV_HEADER;
}

int event_m_directory_new(pio_socket i_socket, char *buffer, int length)
{
    char directory[1024]={0};
    p_up_down_file pclient_file = (p_up_down_file)i_socket->extend;
    sprintf(directory, "%s%s", pclient_file->server_directory, buffer + sizeof(io_data_header));
    CreateDirectory(directory, NULL);

    pclient_file->nFileCur++;

    kms_sock_send(i_socket, CLIENT_FILE, CLIENT_DIRECTORY_NEXT, pclient_file->nFileMax, NULL, 0);

    update_client_list_file_item(pclient_file);
    return SOCK_LOOP_RECV_HEADER;
}

int event_m_enum_up_directory(p_up_down_file pclient_file, char *startpath, char *findpath)
{
    char find_path[MAX_PATH]={0};
    char find_directory[MAX_PATH]={0};
    char start_path[MAX_PATH]={0};
    int filecount = 0;
    HANDLE hFind;
    WIN32_FIND_DATA fData;
    sprintf(find_path,"%s*",findpath);

    if ((hFind = FindFirstFile(find_path, &fData)) == INVALID_HANDLE_VALUE)
        return 0;
    do {
        if(fData.cFileName[0] != '.'){
            p_op_file pclient_file_list = (p_op_file)malloc(sizeof(op_file));
            if (pclient_file_list != NULL){
                memset(pclient_file_list, 0x0, sizeof(op_file));
                pclient_file_list->attributes = fData.dwFileAttributes;
                sprintf(pclient_file_list->filename, "%s%s", startpath, fData.cFileName);
                printf("%s\n", pclient_file_list->filename);

                if(pclient_file->h_op_file == NULL)
                    pclient_file->h_op_file = pclient_file_list;
                else
                    pclient_file->c_op_file->next = pclient_file_list;

                pclient_file->c_op_file = pclient_file_list;
            }

            filecount++;

            if(fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
                sprintf(find_directory,"%s\\%s\\", findpath, fData.cFileName);
                sprintf(start_path,"%s%s\\", startpath, fData.cFileName);
                filecount += event_m_enum_up_directory(pclient_file, start_path, find_directory);
            }
        }
    } while (FindNextFile(hFind,&fData));

    return filecount;
}

int destroy_m_file_up_down(pio_socket i_socket)
{
    printf("%s\n", "destroy_m_file_up_down");
    p_up_down_file pclient_file = (p_up_down_file)i_socket->extend;
    p_op_file pclient_file_list = pclient_file->h_op_file;
    p_op_file pclient_file_temp;
    while(pclient_file_list){
        pclient_file_temp = pclient_file_list;
        pclient_file_list = pclient_file_list->next;
        free(pclient_file_temp);
    }
    free(pclient_file);
    return 0;
}
