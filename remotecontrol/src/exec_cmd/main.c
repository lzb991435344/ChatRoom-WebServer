/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>
#include <process.h>

HANDLE hExit = NULL;

unsigned __stdcall wordk_recv(void *param)
{
    char buffer[4096];
    WSANETWORKEVENTS  events;
    WSAEVENT WSAEvent = WSACreateEvent();
    SOCKET sock = (SOCKET)param;

    WSAEventSelect(sock, WSAEvent, FD_READ | FD_CLOSE);

    for (;;) {
        if (WaitForSingleObject(WSAEvent, INFINITE) == -1)
            break;
        if (WSAEnumNetworkEvents(sock, WSAEvent, &events) != 0)
            break;

        if(events.lNetworkEvents & FD_CLOSE) {
            break;
        } else if (events.lNetworkEvents & FD_READ) {
            memset(buffer, 0x0, 4096);
            if (recv(sock, buffer, 4096, 0) > 0) {
                printf(buffer);
            }
        }

    }
    SetEvent(hExit);
    return 0;
}

unsigned __stdcall wordk_cmd(void *param)
{
    char cmd[4096];
    SOCKET sock = (SOCKET)param;
    for (;;) {
        memset(cmd, 0x0, 4096);
        gets(cmd);
        strcat(cmd, "\r\n");
        send(sock, cmd, strlen(cmd), 0);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        return -1;

    DialogBox(NULL, NULL, NULL, NULL);

    u_long port = atol(argv[1]);

    WSADATA WSAData;
    WSAStartup(0x202,&WSAData);
    struct hostent *host;

    struct sockaddr_in server_in;
    struct sockaddr_in client_in;
    int client_in_len;
    SOCKET socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 0);
    SOCKET client_sock;

    host = gethostbyname(NULL);
    if (host == NULL)
        return -1;

   // printf("Wait %s\n",argv[1]);

    server_in.sin_family = AF_INET;
    server_in.sin_port = htons(port);
    server_in.sin_addr.s_addr = INADDR_ANY;
    memcpy(&server_in.sin_addr.S_un.S_addr,host->h_addr_list[0],host->h_length);

    if(bind(socket,(struct sockaddr far*)&server_in,sizeof(server_in)) == SOCKET_ERROR){
        return -1;
    }
    listen(socket,10);

    client_in_len = sizeof(client_in);
    client_sock = accept(socket,(struct sockaddr far*)&client_in,&client_in_len);
    closesocket(socket);

    if(client_sock != INVALID_SOCKET){
        hExit = CreateEvent(NULL,FALSE,FALSE,NULL);
        CloseHandle((HANDLE)_beginthreadex(NULL,0,wordk_recv,(void*)client_sock,0,NULL));
        CloseHandle((HANDLE)_beginthreadex(NULL,0,wordk_cmd,(void*)client_sock,0,NULL));
        WaitForSingleObject(hExit,INFINITE);
        CloseHandle(hExit);
        closesocket(client_sock);
    }

    return 0;
}
