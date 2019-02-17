#include "km_socket.h"
#include "main_frame_client_op.h"
#include "module_manger.h"
#include "main_frame_event.h"
#include "ip_info.h"
#include "m_frame_file_event.h"
#include "m_frame_screen_event.h"

extern unsigned long TSyssize;
extern char * TSysbuff;

kms_event_func main_event_funcs[] = {
    kmc_main_version_h,
    kmc_main_version,
    kmc_main_module,
    kmc_main_info,
    kmc_main_error,
    kmc_main_module_ok
};

int initialize_km_main_event()
{
    g_share_main->event_funcs[CLIENT_MAIN].count          = sizeof(main_event_funcs)/sizeof(kms_event_func);
    g_share_main->event_funcs[CLIENT_MAIN].event_funcs    = main_event_funcs;
    return 0;
}

int kmc_main_version_h(pio_socket i_socket, char *buffer, int lengthdd)
{
    char mod_buffer[1024]={0};
    pkm_client  p_client = NULL;
    int length = 0;
    unsigned long ip;
    pmanger_module p_temp           = p_manger_client_module;

    pio_data_header    pi_data_header    = (pio_data_header)buffer;
    pclient_ver        pc_ver        = (pclient_ver)(buffer + sizeof(io_data_header));

    printf("kmc_main_version_h:FF:%d:%d:%d:%s\n", (int)pc_ver->c_type, (int)pc_ver->wVersion, C_SCREEN, inet_ntoa(i_socket->remote_addr.sin_addr));

    if (pc_ver->c_type == C_MAIN) {
        if (g_share_main->c_ver.wVersion == pc_ver->wVersion) {
            p_client = malloc(sizeof(km_client));
            if (p_client != NULL) {
                memset(p_client,0x0,sizeof(km_client));
                ip = str2ip(inet_ntoa(i_socket->remote_addr.sin_addr));
                find_ip_info(ip,p_client->ipaddr);

                i_socket->io_type  = C_MAIN;
                i_socket->extend   = p_client;
                memcpy(&p_client->c_ver,pc_ver,sizeof(client_ver));
                //printf("%s %x\n%s\n","client_main_version 11",p_client_ver->wVersion,lpSystemVersion[p_client_ver->wSystemVersion]);
                //发送所有模块
                while (p_temp) {
                    printf("crc32:%x\n",(int)p_temp->c_module.crc32);
                    memcpy(&mod_buffer[length], (void*)&p_temp->c_module, sizeof(client_module));
                    p_temp = p_temp->next;
                    length += sizeof(client_module);
                }
                kms_sock_send(i_socket, CLIENT_MAIN, CLIENT_MAIN_MODULE, 0, mod_buffer, length);
            }
        } else {
            printf("%s:%d\n","send new client main_version 22", (int)TSyssize);
            //发送最新的客户端
            kms_sock_send(i_socket, CLIENT_MAIN, CLIENT_MAIN_UPDATE, 0, TSysbuff, TSyssize);
        }
    } else if (pc_ver->c_type == C_UPDOWNFILE) {
        p_up_down_file p_client_file     = (p_up_down_file)pi_data_header->crc32;
        if (p_client_file != NULL) {
            i_socket->io_type  = C_UPDOWNFILE;
            i_socket->extend   = p_client_file;
            event_m_file_up_down(i_socket);
        }
    } else if (pc_ver->c_type == C_SCREEN) {
        pkm_client p_client = (pkm_client)pi_data_header->crc32;
        if (p_client != NULL) {
            i_socket->io_type = C_SCREEN;
            i_socket->extend = p_client;
            p_client->frame_screen.i_socket = i_socket;
            event_m_screen(i_socket, &p_client->frame_screen, (pscreen_ver)pc_ver);
        }
    } else if (pc_ver->c_type == C_CAP_SCREEN) {
        pkm_client p_client = (pkm_client)pi_data_header->crc32;
        if (p_client != NULL) {
            i_socket->io_type = C_CAP_SCREEN;
            i_socket->extend = p_client;
            p_client->frame_cap_screen.i_socket = i_socket;
            kms_sock_send(i_socket, 5, 1, 0, 0, 0);
            //event_m_screen(i_socket, &p_client->frame_screen, (pscreen_ver)pc_ver);
        }
    }
   // return client_main_version(&pio_handle->io_socket,&pio_handle->io_data);
   return SOCK_LOOP_RECV_HEADER;
}

int kmc_main_version(pio_socket i_socket, char *buffer, int length)
{
    printf("recv all\n");
    //发送客户端
    return SOCK_LOOP_RECV_HEADER;
}

int kmc_main_module(pio_socket i_socket, char *buffer, int length)
{
    pio_data_header pi_data_header = (pio_data_header)buffer;
    pmanger_module pm_module = find_mod_dll_obj(pi_data_header->crc32);
    if (pm_module == NULL) {
        kms_sock_send(i_socket, CLIENT_MAIN, CLIENT_MAIN_ERROR_MODULE, pi_data_header->crc32, NULL, 0);
    } else {
        printf("%s %s:%d\n", "send module", inet_ntoa(i_socket->remote_addr.sin_addr), (int)pm_module->modulelength);
        kms_sock_send(i_socket, CLIENT_MAIN, CLIENT_MAIN_RECV_MODULE, pm_module->c_module.crc32, pm_module->moduledata, pm_module->modulelength);
    }
    return SOCK_LOOP_RECV_HEADER;
}

int kmc_main_info(pio_socket i_socket, char *buffer, int length)
{
    pkm_client pclient = (pkm_client)i_socket->extend;
    memcpy(&pclient->c_info, buffer+sizeof(io_data_header),sizeof(client_info));
    printf("AAAAAAA:%s\n", pclient->c_info.lpAdapterDescriptor);
    main_frame_client_insert(i_socket);               //添加到listview
    return SOCK_LOOP_RECV_HEADER;
}

int kmc_main_error(pio_socket i_socket, char *buffer, int length)
{
    pio_data_header pi_data_header = (pio_data_header)buffer;
    printf("module:error:%d %s\n", pi_data_header->error1, inet_ntoa(i_socket->remote_addr.sin_addr));
    return SOCK_LOOP_RECV_HEADER;
}

int kmc_main_module_ok(pio_socket i_socket, char *buffer, int length)
{
    kms_sock_send(i_socket, 0, 4, 0, NULL, 0);
    return SOCK_LOOP_RECV_HEADER;
}
