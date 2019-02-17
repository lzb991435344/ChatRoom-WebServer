#ifndef M_FRAME_DDOS_H_INCLUDED
#define M_FRAME_DDOS_H_INCLUDED

#include "km_head.h"

typedef struct tag_ddos_task{
    int listview;
    int status;
    int type;
    int thread;
    char addr[512];
    struct tag_ddos_task *next;
}ddos_task, *pddos_task;

int  main_frame_draw_ddos_col();
int  main_frame_draw_ddos_col_width(int right);

LRESULT __stdcall main_frame_event_notify_ddos_listview(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

int make_ddos_initialize(HWND hWnd);

int main_frame_event_ddos_del();
int main_frame_event_ddos_start();
int main_frame_event_ddos_stop();

int main_frame_ddos_send_task(pio_socket i_temp);

#endif // M_FRAME_DDOS_H_INCLUDED
