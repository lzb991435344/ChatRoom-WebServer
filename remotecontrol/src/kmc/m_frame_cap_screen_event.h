/*
��������:����
��������:2010.8
Զ�̿��Ʊ������Ⱥ:30660169,6467438
*/

#ifndef M_FRAME_CAP_SCREEN_EVENT_H_INCLUDED
#define M_FRAME_CAP_SCREEN_EVENT_H_INCLUDED

#include "km_head.h"
#include "..\\share\\event.h"

int initialize_km_cap_screen_event();

int event_m_cap_screen_count(pio_socket i_socket, char *buffer, int lengthdd);
int event_m_cap_screen_frame(pio_socket i_socket, char *buffer, int lengthdd);

#endif // M_FRAME_CAP_SCREEN_EVENT_H_INCLUDED
