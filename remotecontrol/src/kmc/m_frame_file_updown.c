/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "m_frame_file_updown.h"

CLIENT_LIST_FILE_COL client_list_file_col[] =
{
    {COL_K_SHOW ,"  总数",60,     insert_file_list_file_max_num},
    {COL_K_SHOW ,"已传数",60,     insert_file_list_file_cur_num},
    {COL_K_SHOW ,"当前大小",60,   insert_file_list_file_max_size},
    {COL_K_SHOW ,"已传大小",60,   insert_file_list_file_cur_size},
    {COL_K_SHOW ,"当前远程文件",140,insert_file_list_file_remote_path},
    {COL_K_SHOW ,"当前本地文件",140,insert_file_list_file_local_path}
};

#define client_list_file_col_num sizeof(client_list_file_col)/sizeof(CLIENT_LIST_FILE_COL)

int update_client_list_file_item(p_up_down_file pclient_file)
{
    int i,col;
    pio_socket pi_socket = pclient_file->i_socket;

    pkm_client pclient = pi_socket->extend;
    pm_frame_file_control  pm_controls = &pclient->frame_file.m_controls;

    for ( i=0,col=0; i<client_list_file_col_num ; i++){
        if (client_list_file_col[i].col_state > 0){
            client_list_file_col[i].insert_file_list_text(pm_controls->hListFile, pclient_file, col, pclient_file->index);
            col++;
        }
    }

    return 0;
}

int  insert_client_list_file_item(pio_socket i_socket)
{
    p_up_down_file pclient_file = i_socket->extend;
    pio_socket pi_socket = pclient_file->i_socket;

    pkm_client pclient = pi_socket->extend;
    pm_frame_file_control  pm_controls = &pclient->frame_file.m_controls;

    pclient_file->index = client_insert_list_file_ip(pm_controls->hListFile, pi_socket, 0, 0);
    update_client_list_file_item(pclient_file);
    return 0;
}

int client_insert_list_file_ip(HWND hWndListFile, pio_socket i_socket, int col, int row)
{
    p_up_down_file pclient_file = i_socket->extend;
    char str[32];
    sprintf(str,"0");

    LVITEM lvi;
    lvi.mask        = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM | LVIF_STATE;
    lvi.state       = LVIS_SELECTED;
    lvi.stateMask   = 0;
    lvi.iSubItem    = 0;
    lvi.pszText     = str;
    lvi.iImage      = 0;
    lvi.iItem       = ListView_GetItemCount(hWndListFile);

    if (pclient_file->f_op_type == F_UPFILE || pclient_file->f_op_type == F_UPDIRECTORY)
        lvi.iImage      = 6;
    else if(pclient_file->f_op_type == F_DOWNFILE || pclient_file->f_op_type == F_DOWNDIRECTORY)
        lvi.iImage      = 5;
    return ListView_InsertItem(hWndListFile, &lvi);
}

int insert_file_list_file_max_num(HWND hWndListFile, p_up_down_file pclient_file, int col, int row)
{
    char str[32];
    sprintf(str, "%d", (int)pclient_file->nFileMax);
    ListView_SetItemText(hWndListFile, row, col, str);
    return 0;
}

int insert_file_list_file_cur_num(HWND hWndListFile, p_up_down_file pclient_file, int col, int row)
{
    char str[32];
    sprintf(str, "%d", (int)pclient_file->nFileCur);
    ListView_SetItemText(hWndListFile, row, col, str);
    return 0;
}

int insert_file_list_file_max_size(HWND hWndListFile, p_up_down_file pclient_file, int col, int row)
{
    char str[32];
    sprintf(str, "%d", (int)pclient_file->nMaxSize);
    ListView_SetItemText(hWndListFile, row, col, str);
    return 0;
}

int insert_file_list_file_cur_size(HWND hWndListFile, p_up_down_file pclient_file, int col, int row)
{
    char str[32];
    sprintf(str, "%d",(int)pclient_file->nCurSize);
    ListView_SetItemText(hWndListFile, row, col, str);
    return 0;
}

int insert_file_list_file_remote_path(HWND hWndListFile, p_up_down_file pclient_file, int col, int row)
{
    ListView_SetItemText(hWndListFile, row, col, pclient_file->client_directory);
    return 0;
}

int insert_file_list_file_local_path(HWND hWndListFile, p_up_down_file pclient_file, int col, int row)
{
    ListView_SetItemText(hWndListFile, row, col, pclient_file->server_directory);
    return 0;
}

int  draw_client_list_file_col(HWND hWndListFile)
{
    LV_COLUMN lvcol;
    int i,col;

    for ( i=0; i<client_list_file_col_num; i++){
        ListView_DeleteColumn(hWndListFile, 0);
    }

    lvcol.mask  = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvcol.fmt   = LVCFMT_LEFT;

    for ( i=0,col=0; i<client_list_file_col_num; i++){
        if (client_list_file_col[i].col_state > 0){
            lvcol.pszText = client_list_file_col[i].col_name;
            ListView_InsertColumn(hWndListFile, col, &lvcol);
            ListView_SetColumnWidth(hWndListFile, col, client_list_file_col[i].width);
            col++;
        }
    }
    return 0;
}

int  draw_client_list_file_col_width(HWND hWndListFile, int right)
{
    int i,col,width=0,temp=0;

    for ( i=0, col=0; i<client_list_file_col_num; i++){
        if (client_list_file_col[i].col_state > 0){
            temp = client_list_file_col[i].width;
            width = width+temp;
            col++;
        }
    }

    if (width < right){
         width = width - temp;
         width = right - width -10;
         ListView_SetColumnWidth(hWndListFile, col-1, width);
    }
    return 0;
}
