#ifndef CLIENT_LIST_GUI_H_INCLUDED
#define CLIENT_LIST_GUI_H_INCLUDED

#include "km_head.h"

typedef int (* insert_list)(pkm_client,int col,int row);

typedef struct tag_client_list_col{
    enum  list_col_state    col_state;
    char                    col_name[64];
    int                     width;
    insert_list             insert_list_text;
}CLIENT_LIST_COL;

int  main_frame_draw_client_list_col();
int  main_frame_draw_client_list_col_width(int right);

int  main_frame_client_insert(pio_socket pi_socket);
int  main_frame_client_delete(pio_socket i_socket, pclient_info c_info);
pio_socket main_frame_client_find_addr(char *addr1, char *addr2);

int  client_insert_list_ip(pio_socket pi_socket,int col,int row);
int  client_insert_list_localip(pkm_client pclient,int col,int row);
int  client_insert_list_system(pkm_client pclient,int col,int row);
int  client_insert_list_processor_type(pkm_client pclient,int col,int row);
int  client_insert_list_processor_hmz(pkm_client pclient,int col,int row);
int  client_insert_list_processor_num(pkm_client pclient,int col,int row);
int  client_insert_list_memory(pkm_client pclient,int col,int row);
int  client_insert_list_connect_time(pkm_client pclient,int col,int row);
int  client_insert_list_screen(pkm_client pclient,int col,int row);
int  client_insert_list_cap(pkm_client pclient,int col,int row);
int  client_insert_list_adapter_descriptor(pkm_client pclient,int col,int row);
int  client_insert_list_ipaddr(pkm_client pclient,int col,int row);
int  client_insert_list_descriptor(pkm_client pclient,int col,int row);

unsigned __stdcall main_frame_client_check(void *param);

#endif // CLIENT_LIST_GUI_H_INCLUDED
