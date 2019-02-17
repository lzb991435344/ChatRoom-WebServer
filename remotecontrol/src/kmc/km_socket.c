/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "km_socket.h"
#include "main_frame_client_op.h"
#include "m_frame_file.h"
#include "m_frame_screen.h"
#include "m_frame_cap_screen.h"
#include "m_frame_file_event.h"

kms_sock_func sock_loop_call[] = {
    kms_sock_re_connect,
    kms_sock_recv,
    kms_sock_close,
    kms_sock_recv_header
};

int
kms_sock_re_connect(pio_socket i_socket)
{
    GUID guid_connect_ex = WSAID_CONNECTEX;
    LPFN_CONNECTEX lpfn_connectex;
    DWORD dwBytes;

    i_socket->socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    if (i_socket->socket == INVALID_SOCKET)
        goto _exit_err;

    if (CreateIoCompletionPort((HANDLE)i_socket->socket, g_share_main->h_completion_port, (ULONG_PTR)i_socket, 0) == NULL)
        goto _exit_err;

    if (bind(i_socket->socket, (struct sockaddr * far)&i_socket->local_addr, sizeof(i_socket->local_addr)) == SOCKET_ERROR)
        goto _exit_err;

    if (WSAIoctl(i_socket->socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid_connect_ex, sizeof(guid_connect_ex), &lpfn_connectex, sizeof(lpfn_connectex), &dwBytes, 0, 0) == SOCKET_ERROR)
        goto _exit_err;

    if (!lpfn_connectex(i_socket->socket, (struct sockaddr* far)&i_socket->remote_addr, sizeof(i_socket->remote_addr), NULL, 0, NULL, &i_socket->overlapped)) {
        if (GetLastError() != WSA_IO_PENDING)
            goto _exit_err;
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
    printf("kms_socket_close\n");
    closesocket(i_socket->socket);

    if (i_socket->f_call_exit != NULL) {
        ((kms_sock_func)i_socket->f_call_exit)(i_socket);
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
    if (ret == -1) {
        closesocket(i_socket->socket);
        return ret;
    }
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
        ret = GetQueuedCompletionStatus(g_share_main->h_completion_port, &dwTrans, (void*)&i_socket, &lpoverlapped, INFINITE);
        if (i_socket != NULL) {
            //printf("kms_sock_loop:f_call_work:%x\n", i_socket->f_call_work);
            ret = ((f_work)i_socket->f_call_work)(ret, dwTrans, i_socket, lpoverlapped);

            if (ret < (sizeof(sock_loop_call)/sizeof(kms_sock_func))) {
                sock_loop_call[ret](i_socket);
            }
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
    i_socket->remote_addr.sin_addr.s_addr = inet_addr(host);

    if (!kms_sock_re_connect(i_socket))
        goto _exit_err;

    return 1;
_exit_err:
    closesocket(i_socket->socket);
    free(i_socket);
    return 0;
}

int
kms_sock_ret_connect(char *host,int port,f_work f_connect_event)
{
    struct hostent *hoste;

    pio_socket i_socket = malloc(sizeof(io_socket));
    memset(i_socket, 0x0, sizeof(io_socket));

    i_socket->socket = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,0,0,0);
    if(i_socket->socket == INVALID_SOCKET)
        goto _exit_err;

    hoste = gethostbyname(host);
    if(hoste == NULL)
        i_socket->remote_addr.sin_addr.s_addr = inet_addr(host);
    else
        memcpy(&i_socket->remote_addr.sin_addr.s_addr,hoste->h_addr_list[0],hoste->h_length);

    i_socket->remote_addr.sin_family      = AF_INET;
    i_socket->remote_addr.sin_port        = htons(port);

    if(connect(i_socket->socket,(struct sockaddr*far)&i_socket->remote_addr,sizeof(i_socket->remote_addr)) == SOCKET_ERROR)
        goto _exit_err;

    f_connect_event(1, 0, i_socket, NULL);
    return 1;
_exit_err:
    closesocket(i_socket->socket);
    free(i_socket);
    return 0;
}

int
kms_sock_accept(pio_socket pi_socket,pio_socket_accept pi_socket_accept,f_work _work)
{
    memset(pi_socket_accept, 0x0, sizeof(io_socket_accept));

    pi_socket_accept->socket  =   WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    pi_socket_accept->dwBytes =   sizeof(struct sockaddr_in)+16;

    if (!lpfn_acceptex(pi_socket->socket, pi_socket_accept->socket, pi_socket_accept->buffer, 0, pi_socket_accept->dwBytes, pi_socket_accept->dwBytes, &pi_socket_accept->dwBytes, (LPOVERLAPPED)pi_socket_accept)) {
        if(GetLastError() != WSA_IO_PENDING){
            return 0;
        }
    }

    return 10;
}

int
kms_sock_listen(int port,f_work _work)
{
    DWORD dw_bytes;
    GUID guid_acceptex = WSAID_ACCEPTEX;
    pio_socket_accept pi_socket_accept;
    int i=0;

    int accept_num = 20;

    pio_socket pi_socket = malloc(sizeof(io_socket));
    memset(pi_socket, 0x0, sizeof(io_socket));

    pi_socket->io_type = C_LISTEN;

    pi_socket_accept = malloc(sizeof(io_socket_accept) * accept_num);
    pi_socket->extend = pi_socket_accept;

    pi_socket->f_call_work = _work;

    pi_socket->socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    if (pi_socket->socket == INVALID_SOCKET)
        goto _exit_error;

    if (NULL == CreateIoCompletionPort((HANDLE)pi_socket->socket, g_share_main->h_completion_port, (DWORD)pi_socket, 0))
        goto _exit_error;

    pi_socket->local_addr.sin_family = AF_INET;
    pi_socket->local_addr.sin_port   = htons(port);
    pi_socket->local_addr.sin_addr.s_addr   = INADDR_ANY;

    if (bind(pi_socket->socket, (struct sockaddr far*)&pi_socket->local_addr, sizeof(pi_socket->local_addr)) == SOCKET_ERROR)
        goto _exit_error;

    if (listen(pi_socket->socket, 0) == SOCKET_ERROR)
        goto _exit_error;

    if (WSAIoctl(pi_socket->socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid_acceptex, sizeof(guid_acceptex), &lpfn_acceptex, sizeof(lpfn_acceptex), &dw_bytes, NULL, NULL) == SOCKET_ERROR)
        goto _exit_error;

    for (i=0; i<accept_num; i++) {
        kms_sock_accept(pi_socket, (pio_socket_accept)pi_socket_accept + i, _work);
    }

    CloseHandle((HANDLE)_beginthreadex(NULL, 0, main_frame_client_check, NULL, 0, NULL));
    return 1;
_exit_error:
    //_log("_socket_listen_tcp ERROR:%d->%d\n", GetLastError(), port);
    closesocket(pi_socket->socket);
    free(pi_socket->extend);
    free(pi_socket);
    return 0;
}

int
km_main_accept(int ret, DWORD dwTrans, pio_socket i_socket, LPOVERLAPPED lpoverlapped)
{
    int len;
    pio_socket pi_socket;
    pio_socket_accept pi_socket_accept = (pio_socket_accept)lpoverlapped;
    if (!ret)
        goto _exit_err;

    pi_socket = malloc(sizeof(io_socket));
    memset(pi_socket, 0x0, sizeof(io_socket));

    if(NULL == CreateIoCompletionPort((HANDLE)pi_socket_accept->socket, g_share_main->h_completion_port, (DWORD)pi_socket, 0)) {
        goto _exit_err;
    }

    setsockopt(pi_socket_accept->socket, SOL_SOCKET, 0x700B, ( char* )&i_socket->socket, sizeof(i_socket->socket) );

    pi_socket->f_call_work = (void*)km_main_parse;
    pi_socket->f_call_exit = (void*)km_main_close;

    pi_socket->socket = pi_socket_accept->socket;

    len = sizeof(pi_socket->remote_addr);
    getpeername(pi_socket->socket, (struct sockaddr far*)&pi_socket->remote_addr, &len);

    pi_socket->i_length = 4086000;
    pi_socket->i_buffer = malloc(pi_socket->i_length);

    kms_sock_recv_header(pi_socket);
    printf("km_main_accept: %d\n",ret);
_exit_err:
    return kms_sock_accept(i_socket, pi_socket_accept, i_socket->f_call_work);
}

int
km_main_parse(int ret, DWORD dwTrans, pio_socket i_socket, LPOVERLAPPED lpoverlapped)
{
    pio_data_header pi_data_header;

    if ( (ret == 0) || (dwTrans == 0) ) {
        printf("km_main_parse =======0\n");
        goto _exit_err;
    }
    i_socket->r_count += dwTrans;
    if (i_socket->r_count < sizeof(io_data_header)) {
        i_socket->r_buf.buf = i_socket->i_buffer + i_socket->r_count;
        i_socket->r_buf.len = sizeof(io_data_header) - i_socket->r_count;
        return SOCK_LOOP_RECV_NEXT;
    }
    pi_data_header = (pio_data_header)i_socket->i_buffer;
    if (i_socket->r_count < pi_data_header->length) {
        i_socket->r_buf.buf = i_socket->i_buffer + i_socket->r_count;
        i_socket->r_buf.len = pi_data_header->length - i_socket->r_count;
        return SOCK_LOOP_RECV_NEXT;
    }
    //i_socket->i_buffer[i_socket->r_count] = '\0';
    memset(&i_socket->i_buffer[i_socket->r_count], 0x0, 0x20);

    printf("%d:%d %d:%d\n", ret, (int)dwTrans, pi_data_header->major_cmd, pi_data_header->minor_cmd);
    //printf("%x\n", g_share_main->event_funcs[CLIENT_MAIN].event_funcs);
    //printf("%x\n", g_share_main->event_funcs[0].event_funcs);
    return g_share_main->event_funcs[pi_data_header->major_cmd].event_funcs[pi_data_header->minor_cmd]
                                (i_socket, i_socket->i_buffer, i_socket->r_count);
_exit_err:
    return SOCK_LOOP_CLOSE;
}

int
km_main_close(pio_socket i_socket)
{
    pkm_client p_client = i_socket->extend;
    if (i_socket->io_type == C_MAIN) {
        destroy_frame_file(i_socket);
        destroy_frame_screen(i_socket);
        destroy_frame_cap_screen(i_socket);
        main_frame_client_delete(i_socket, &p_client->c_info);
    } else if (i_socket->io_type == C_SCREEN) {
        free_frame_screen(i_socket);
    } else if (i_socket->io_type == C_UPDOWNFILE) {
        destroy_m_file_up_down(i_socket);
    } else if (i_socket->io_type == C_CAP_SCREEN) {
        free_frame_cap_screen(i_socket);
    }
    free(i_socket->i_buffer);
    return 0;
}
