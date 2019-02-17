/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "m_frame_file.h"
#include "resource.h"
#include "km_socket.h"
#include "m_frame_file_updown.h"
#include "frame_tools.h"

HIMAGELIST  hImageList;

char test[] ="status";
char frame_cls_file[] = "cls_file";

char lpDiskType[][32]={
    "无效路径",
    "无效路经",
    "移动磁盘",
    "本地磁盘",
    "网络磁盘",
    "本地光驱(CD/DVD)",
    "随机存取(RAM)磁盘"
};

FILE_ICO file_ico[] ={
    {"ramdisk",0},
    {"disk",0},
    {"cdrom",0},
    {"directory",0},
    {"",0},
    {".cpp",0},{".h",0},{".c",0},
    {".7z",0},{".iso",0},{".zip",0},{".rar",0},
    {".doc",0},{".ini",0},{".log",0},{".txt",0},{".pdf",0},{".inf",0},
    {".sys",0},{".dll",0},
    {".js",0},{".htm",0},{".html",0},{".xml",0},{".ime",0},
    {".exe",0},{".msi",0},{".com",0},
    {".jpg",0},{".gif",0},{".bmp",0},{".ico",0},
    {".flv",0},{".avi",0},{".mp3",0},{".wmv",0},{".wma",0},{".mpeg",0},
    {".bak",0},
    {".db",0},
    {".dat",0},
    {".vbs",0},{".bat",0},
    {".msc",0},
    {".chm",0},{".hlp",0},
    {".tmp",0},
    {".nls",0},
    {".scf",0}
};

#define FILE_ICO_DIRECTORY  3
#define FILE_ICO_UNDOWN     4

extern      HINSTANCE   hInst;

HIMAGELIST hHotyyImageList4;
HIMAGELIST hHotyyImageList3;
HIMAGELIST hHotyyImageList2;

HMENU   hMenuList;

#define CBES_EX_NOSIZELIMIT 0x00000008

#define file_ico_start  5
#define file_ico_size   sizeof(file_ico)/sizeof(FILE_ICO)


int initialize_frame_file_tools(HWND hWnd, pm_frame_file_control pm_controls)
{
    pm_controls->hTools = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_VISIBLE|WS_CHILD|WS_BORDER| TBSTYLE_WRAPABLE| TBSTYLE_FLAT | CCS_ADJUSTABLE, 0, 0, 0, 0, hWnd, (HMENU)IDR_TOOLBAR1, hInst, NULL);
//CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS
    pm_controls->hComboEx = CreateWindowEx(0, WC_COMBOBOXEX, NULL, WS_VISIBLE|WS_CHILD|CBS_SIMPLE|CBS_DROPDOWN|CBS_DROPDOWNLIST|CBS_SORT|WS_VSCROLL, 100, 4, 300, 200, pm_controls->hTools, (HMENU)IDR_COMBOEX, hInst, NULL);

    SendMessage(pm_controls->hTools, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    TBBUTTON thb[3];
    thb[0].iBitmap      = 0;
    thb[0].fsState      = TBSTATE_ENABLED;
    thb[0].fsStyle      = TBSTYLE_BUTTON;
    thb[0].dwData       = 0;
    thb[0].idCommand    = IDR_COMMAND1;
    thb[0].iString      = 0;//SendMessage(hWndTools, TB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) t1);

    thb[1].iBitmap      = 1;
    thb[1].fsState      = TBSTATE_ENABLED;
    thb[1].fsStyle      = TBSTYLE_BUTTON;
    thb[1].dwData       = 0;
    thb[1].idCommand    = IDR_COMMAND2;
    thb[1].iString      = 0;//SendMessage(hWndTools, TB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) t2);

    thb[2].iBitmap      = 1;
    thb[2].fsState      = TBSTATE_ENABLED;
    thb[2].fsStyle      = TBSTYLE_BUTTON;
    thb[2].dwData       = 0;
    thb[2].idCommand    = IDR_COMMAND3;
    thb[2].iString      = 0;//SendMessage(hWndTools, TB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) t2);

    SendMessage(pm_controls->hTools, TB_ADDBUTTONS, (WPARAM)3, (LPARAM)(LPTBBUTTON)&thb);
    SendMessage(pm_controls->hTools, TB_AUTOSIZE, 0, 0);
    SendMessage(pm_controls->hTools, TB_SETEXTENDEDSTYLE, (WPARAM) 0, (LPARAM) (DWORD)TBSTYLE_EX_DRAWDDARROWS|TBSTYLE_EX_DOUBLEBUFFER|TBSTYLE_EX_HIDECLIPPEDBUTTONS );

    SendMessage(pm_controls->hTools, TB_SETIMAGELIST, 0, (LPARAM)hHotyyImageList4);
    SendMessage(pm_controls->hComboEx, CBEM_SETIMAGELIST, 0, (LPARAM)hHotyyImageList2);

    pm_controls->hStatus = CreateStatusWindow(WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN, test, hWnd, 0);

    return 0;
}

LRESULT __stdcall frame_file(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    pio_socket             pi_socket  = NULL;
    pkm_client             pclient     = NULL;
    pm_frame_file_control  pm_controls = NULL;
    switch(Message)
    {
    case WM_RECV_DRIVER:
        {
            view_combox_disk((pm_frame_file)lParam);
        }
        break;
    case WM_RECV_NET:
        {
            view_list_net((pio_socket)wParam, (char*)lParam);
        }
        break;
    case WM_RECV_DIRECTORY:
        {
            view_list_directory((pio_socket)wParam, (char*)lParam);
        }
        break;
    case WM_CREATE:
        {
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                pclient = (pkm_client)pi_socket->extend;
                pm_controls = &pclient->frame_file.m_controls;

                pm_controls->hListView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL, WS_CHILD|WS_VISIBLE, 210, 30, 200, 100, hWnd, (HMENU)IDR_LISTVIEW, hInst, NULL);

                SendMessage(pm_controls->hListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, 0x20);
                ListView_SetImageList(pm_controls->hListView, hHotyyImageList3, LVSIL_NORMAL);
                ListView_SetImageList(pm_controls->hListView, hHotyyImageList2, LVSIL_SMALL);

                insert_list_column_text(pm_controls->hListView , 0, 255, "名字");
                insert_list_column_text(pm_controls->hListView , 1, 90, "大小");
                insert_list_column_text(pm_controls->hListView , 2, 100, "类型");
                insert_list_column_text(pm_controls->hListView , 3, 140, "修改时间");

                pm_controls->hListFile = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL, WS_CHILD|LVS_REPORT, 210, 30, 200, 100, hWnd, (HMENU)IDR_LISTVIEWFILE, hInst, NULL);
                SendMessage(pm_controls->hListFile, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, 0x20);
                ListView_SetImageList(pm_controls->hListFile, hImageList, LVSIL_SMALL);

                draw_client_list_file_col(pm_controls->hListFile);

                initialize_frame_file_tools(hWnd,pm_controls);

                int x = (GetSystemMetrics(SM_CXSCREEN)-560)/2;
                int y = (GetSystemMetrics(SM_CYSCREEN)-320)/2;
                MoveWindow(hWnd, x, y, 560, 320, TRUE);
            }
        }
        break;
    case WM_SIZE:
        {
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                pclient = (pkm_client)pi_socket->extend;
                pm_controls = &pclient->frame_file.m_controls;

                RECT rect,tool,status;
                GetClientRect(hWnd, &rect);
                GetClientRect(pm_controls->hTools, &tool);
                GetClientRect(pm_controls->hStatus, &status);
                tool.bottom = tool.bottom + 2;

                ListView_SetColumnWidth(pm_controls->hListView, 0, rect.right-90-100-140-20);

                HDWP hDwp;
                hDwp  = BeginDeferWindowPos(4);
                DeferWindowPos(hDwp, pm_controls->hTools, NULL, 0, 0, 0, 0, SWP_NOZORDER);
                DeferWindowPos(hDwp, pm_controls->hStatus, NULL, 0, 0, 0, 0, SWP_NOZORDER);
                DeferWindowPos(hDwp, pm_controls->hListView, NULL, 0, tool.bottom, rect.right, rect.bottom-tool.bottom-status.bottom, SWP_NOZORDER);
                DeferWindowPos(hDwp, pm_controls->hListFile, NULL, 0, tool.bottom, rect.right, rect.bottom-tool.bottom-status.bottom, SWP_NOZORDER);
                EndDeferWindowPos(hDwp);

                draw_client_list_file_col_width(pm_controls->hListFile, rect.right-10);

                MoveWindow(pm_controls->hComboEx, 90, 4, rect.right - 90, 0, TRUE);
            }
        }
        break;
    case WM_NOTIFY:
        {
            switch(LOWORD(wParam))
            {
            case IDR_LISTVIEW:
                frame_file_list(hWnd, Message, wParam, lParam);
                break;
            }
        }
        break;
    case WM_COMMAND:
        {
            int iWmId    = LOWORD(wParam);
            int iWmEvent = HIWORD(wParam);
            switch(iWmId)
            {
            case IDR_COMBOEX:
                if (iWmEvent == CBN_SELCHANGE)
                    frame_combox_event(hWnd, Message, wParam, lParam);
                break;
            case IDR_COMMAND1:
            case IDR_COMMAND2:
            case IDR_COMMAND3:
                frame_menu_event(hWnd, iWmId, wParam, lParam);
                break;
            default:
                frame_list_menu_event(hWnd, iWmId, wParam, lParam);
                break;
            }
        }
        break;
    case WM_DESTROY:
    case WM_CLOSE:
        {
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                pclient = (pkm_client)pi_socket->extend;
                if (pclient->frame_file.f_state != F_CLOSE) {
                    ShowWindow(hWnd,SW_HIDE);
                    return 0;
                }
                DestroyWindow(hWnd);
            }
        }
        break;
    }
    return DefWindowProc(hWnd, Message, wParam, lParam);
}

p_up_down_file insert_client_file(pio_socket i_socket, enum file_op_type f_op_type, char *serverpath, char *clientpath)
{
    p_up_down_file pclient_file = (p_up_down_file)malloc(sizeof(up_down_file));
    if (pclient_file == NULL)
        return NULL;

    memset(pclient_file, 0x0, sizeof(up_down_file));

    pclient_file->i_socket = i_socket;
    pclient_file->f_op_type = f_op_type;
    pclient_file->nFileMax = 1;
    pclient_file->nFileCur = 0;
    strcpy(pclient_file->server_directory,serverpath);
    strcpy(pclient_file->client_directory,clientpath);

    return pclient_file;
}

int frame_list_menu_event(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    pio_socket             pi_socket  = NULL;
    pkm_client             pclient     = NULL;
    pm_frame_file_control  pm_controls = NULL;

    int     nCurIndex, nCurCount, nListIndex;
    LONG    lStyle;

    char lpPath[1024]={0};
    char lpName[MAX_PATH]={0};
    char lpRemotePath[1024]={0};
    char lpRemoteName[MAX_PATH]={0};
    char *find = NULL;

    p_up_down_file pclient_file = NULL;

    pi_socket = get_frame_io_socket(hWnd);
    if (!pi_socket)
        return 0;

    pclient     = (pkm_client)pi_socket->extend;
    pm_controls = &pclient->frame_file.m_controls;

    nCurIndex = SendMessage(pm_controls->hComboEx, CB_GETCURSEL, 0, 0);
    nCurCount = SendMessage(pm_controls->hComboEx, CB_GETCOUNT, 0, 0);
    nListIndex= SendMessage(pm_controls->hListView, LVM_GETSELECTIONMARK, 0, 0);

    get_combox_text(pm_controls->hComboEx, nCurIndex, lpRemotePath);
    ListView_GetItemText(pm_controls->hListView, nListIndex, 0, lpRemoteName, MAX_PATH);
   // ListView_GetItemText(pm_controls->hListView,nListIndex,2,lpRemoteName,MAX_PATH);

    switch(Message)
    {
    case IDM_UP_FILE:
        if (open_file_path(lpPath, lpName, hWnd)) {
            strcat(lpRemotePath, lpName);
            //MessageBox(0, lpPath, lpRemotePath, 0);
            pclient_file = insert_client_file(pi_socket, F_UPFILE, lpPath, lpRemotePath);
            if (pclient_file != NULL) {
                kms_sock_send(pi_socket, CLIENT_FILE, CLIENT_UP_DOWN_FILE, (unsigned long)pclient_file, NULL, 0);
            }
        }
        break;
    case IDM_DOWN_FILE:
        strcpy(lpPath,lpRemoteName);
        if (save_file_path(lpPath, hWnd)) {
            strcat(lpRemotePath, lpRemoteName);
            //MessageBox(NULL, lpPath, lpRemotePath, 0);
            pclient_file = insert_client_file(pi_socket, F_DOWNFILE, lpPath, lpRemotePath);
            if (pclient_file != NULL) {
                kms_sock_send(pi_socket, CLIENT_FILE, CLIENT_UP_DOWN_FILE, (unsigned long)pclient_file, NULL, 0);
            }
        }
        break;
    case IDM_UP_DIRECTORY:
        if (open_file_directory(lpPath, "选择要上传的路径", hWnd)) {
            find = strrchr(lpPath, '\\');
            if (find == NULL)
                return 0;
            if (lpPath[strlen(lpPath)-1] != '\\')
                lpPath[strlen(lpPath)] = '\\';
            find++;
            strcat(lpRemotePath,find);
            //MessageBox(NULL, lpPath, lpRemotePath, 0);
            pclient_file = insert_client_file(pi_socket, F_UPDIRECTORY, lpPath, lpRemotePath);
            if (pclient_file != NULL){
                kms_sock_send(pi_socket, CLIENT_FILE, CLIENT_UP_DOWN_FILE, (unsigned long)pclient_file, NULL, 0);
            }
        }
        break;
    case IDM_DOWN_DIRECTORY:
        if (open_file_directory(lpPath, "选择要保存的路径", hWnd)) {
            strcat(lpRemotePath, lpRemoteName);
            strcat(lpRemotePath, "\\");
            if (lpPath[strlen(lpPath)] != '\\')
                strcat(lpPath, "\\");
            strcat(lpPath, lpRemoteName);
            strcat(lpPath, "\\");
            //MessageBox(0, lpPath, lpRemotePath, 0);
            pclient_file = insert_client_file(pi_socket, F_DOWNDIRECTORY, lpPath, lpRemotePath);
            if (pclient_file != NULL){
                kms_sock_send(pi_socket, CLIENT_FILE, CLIENT_UP_DOWN_FILE, (unsigned long)pclient_file, NULL, 0);
            }
        }
        break;
    case IDM_DELETE_FILE:
        strcat(lpRemotePath, lpRemoteName);
        kms_sock_send(pi_socket, CLIENT_FILE, CLIENT_DELETE_FILE, 0, lpRemotePath, strlen(lpRemotePath));
        ListView_DeleteItem(pm_controls->hListView, nListIndex);
        break;
    case IDM_DELETE_DIRECTORY:
        strcat(lpRemotePath, lpRemoteName);
        strcat(lpRemotePath, "\\");
        kms_sock_send(pi_socket, CLIENT_FILE, CLIENT_DELETE_DIRECTORY, 0, lpRemotePath, strlen(lpRemotePath));
        ListView_DeleteItem(pm_controls->hListView, nListIndex);
        break;
    case IDM_EXECFILE:
        strcat(lpRemotePath, lpRemoteName);
        kms_sock_send(pi_socket, CLIENT_FILE, CLIENT_REMOTE_EXEC, 0, lpRemotePath, strlen(lpRemotePath));
        break;
    case IDM_VIEW_MAX_ICON:
        lStyle=GetWindowLong(pm_controls->hListView, GWL_STYLE);
        lStyle   &=   ~LVS_TYPEMASK;
        lStyle   |=   LVS_ICON ;
        SetWindowLong(pm_controls->hListView, GWL_STYLE, lStyle);
        break;
    case IDM_VIEW_MIN_ICON:
        lStyle=GetWindowLong(pm_controls->hListView, GWL_STYLE);
        lStyle   &=   ~LVS_TYPEMASK;
        lStyle   |=   LVS_SMALLICON ;
        SetWindowLong(pm_controls->hListView, GWL_STYLE, lStyle);
        break;
    case IDM_VIEW_LIST:
        lStyle=GetWindowLong(pm_controls->hListView, GWL_STYLE);
        lStyle   &=   ~LVS_TYPEMASK;
        lStyle   |=   LVS_LIST;
        SetWindowLong(pm_controls->hListView, GWL_STYLE, lStyle);
        break;
    case IDM_VIEW_X_LIST:
        lStyle=GetWindowLong(pm_controls->hListView, GWL_STYLE);
        lStyle   &=   ~LVS_TYPEMASK;
        lStyle   |=   LVS_REPORT;
        SetWindowLong(pm_controls->hListView, GWL_STYLE, lStyle);
        break;
    }
    return 0;
}

int frame_combox_event(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    pio_socket             pi_socket  = NULL;
    pkm_client             pclient     = NULL;
    pm_frame_file_control  pm_controls = NULL;

    int nIndex=0;
    int nCount=0;
    char drive[12]={0};

    pi_socket = get_frame_io_socket(hWnd);
    if (pi_socket) {
        pclient = (pkm_client)pi_socket->extend;
        pm_controls = &pclient->frame_file.m_controls;

        nIndex = SendMessage(pm_controls->hComboEx, CB_GETCURSEL, 0, 0);
        nCount = SendMessage(pm_controls->hComboEx, CB_GETCOUNT, 0, 0);
        if (nIndex == -1)
            return 0;
        if (pclient->frame_file.m_data.comboxcur == nIndex) {
            return 0;
        }
        if (nIndex < pclient->frame_file.m_data.comboxnum) {
            pclient->frame_file.m_data.comboxcur = nIndex;
            ListView_DeleteAllItems(pm_controls->hListView);
            if (nIndex == 0) {
                view_combox_disk(&pclient->frame_file);
            //}else if((nIndex+1) == pio_handle->io_socket.pclient->frame_file.m_data.comboxnum){
            //    send_socket(&pio_handle->io_socket,&pio_handle->io_data,CLIENT_FILE,CLIENT_GET_NET,0);
            } else {
                nIndex--;
                sprintf(drive, "%s\\", ((PDRIVE)pclient->frame_file.m_data.drive+nIndex)->name);
                kms_sock_send(pi_socket, CLIENT_FILE, CLIENT_GET_DIRECTORY, 0, drive, strlen(drive));
                insert_foo_item(pclient, pm_controls, drive);
            }
        }
        SetFocus(hWnd);
    }
    return 0;
}

LRESULT __stdcall frame_file_list(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    #define lpnm ((LPNMHDR)lParam)

    pio_socket             pi_socket  = NULL;
    pkm_client             pclient     = NULL;
    pm_frame_file_control  pm_controls = NULL;
    LV_DISPINFO *lpDis = (LV_DISPINFO*)lParam;

    char    path[1024] = {0};
    char    size[128]  = {0};
    char    drive[12]={0};
    int     nCurCombox  = 0;
    int     nPathLength = 0;
    switch(lpnm->code)
    {
    case NM_DBLCLK:
        {
            if(lpDis->item.mask == -1)
                return -1;
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                pclient     = (pkm_client)pi_socket->extend;
                pm_controls = &pclient->frame_file.m_controls;

                nCurCombox      = SendMessage(pm_controls->hComboEx, CB_GETCURSEL, 0, 0);

                if (nCurCombox < pclient->frame_file.m_data.comboxnum) {
                    sprintf(drive,"%s\\", ((PDRIVE)pclient->frame_file.m_data.drive+lpDis->item.mask)->name);
                    kms_sock_send(pi_socket, CLIENT_FILE, CLIENT_GET_DIRECTORY, 0, drive, strlen(drive));

                    insert_foo_item(pclient, pm_controls, drive);
                    ListView_DeleteAllItems(pm_controls->hListView);
                }
                else if (get_combox_text(pm_controls->hComboEx, nCurCombox, path) > 0 ) {
                    nPathLength = strlen(path);

                    ListView_GetItemText(lpnm->hwndFrom, lpDis->item.mask, 1, size, 128);
                    ListView_GetItemText(lpnm->hwndFrom, lpDis->item.mask, 0, &path[nPathLength], 512);

                    strcat(path,"\\");
                    if (strcmp(size,"") == 0) {
                        kms_sock_send(pi_socket, CLIENT_FILE, CLIENT_GET_DIRECTORY, 0, path, strlen(path));
                        insert_foo_item(pclient, pm_controls, path);
                        ListView_DeleteAllItems(pm_controls->hListView);
                    }
                }

            }
        }
        break;
    case NM_RCLICK:
        {
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                pclient         = (pkm_client)pi_socket->extend;
                pm_controls     = &pclient->frame_file.m_controls;
                nCurCombox      = SendMessage(pm_controls->hComboEx, CB_GETCURSEL, 0, 0);


                EnableMenuItem(hMenuList, IDM_UP_FILE, FALSE);
                EnableMenuItem(hMenuList, IDM_UP_DIRECTORY, FALSE);

                EnableMenuItem(hMenuList, IDM_DOWN_FILE, FALSE);
                EnableMenuItem(hMenuList, IDM_DOWN_DIRECTORY, FALSE);

                EnableMenuItem(hMenuList, IDM_DELETE_FILE, FALSE);
                EnableMenuItem(hMenuList, IDM_DELETE_DIRECTORY, FALSE);

                EnableMenuItem(hMenuList, IDM_EXECFILE, FALSE);

                if (nCurCombox == 0) {
                    EnableMenuItem(hMenuList, IDM_UP_FILE, TRUE);
                    EnableMenuItem(hMenuList, IDM_UP_DIRECTORY, TRUE);

                    EnableMenuItem(hMenuList,IDM_DOWN_FILE, TRUE);
                    EnableMenuItem(hMenuList,IDM_DOWN_DIRECTORY, TRUE);

                    EnableMenuItem(hMenuList,IDM_DELETE_FILE, TRUE);
                    EnableMenuItem(hMenuList,IDM_DELETE_DIRECTORY, TRUE);

                    EnableMenuItem(hMenuList,IDM_EXECFILE, TRUE);
                }
                else if (lpDis->item.mask == -1) {
                    EnableMenuItem(hMenuList, IDM_DOWN_FILE, TRUE);
                    EnableMenuItem(hMenuList, IDM_DOWN_DIRECTORY, TRUE);

                    EnableMenuItem(hMenuList, IDM_DELETE_FILE, TRUE);
                    EnableMenuItem(hMenuList, IDM_EXECFILE, TRUE);
                }


                ListView_GetItemText(pm_controls->hListView, lpDis->item.mask, 1, path, MAX_PATH);

                if (strcmp(path,"") == 0) {
                    EnableMenuItem(hMenuList, IDM_DOWN_FILE, TRUE);
                    EnableMenuItem(hMenuList, IDM_EXECFILE, TRUE);
                    EnableMenuItem(hMenuList, IDM_DELETE_FILE, TRUE);
                } else {
                    EnableMenuItem(hMenuList, IDM_DOWN_DIRECTORY, TRUE);
                    EnableMenuItem(hMenuList, IDM_DELETE_DIRECTORY, TRUE);
                }
                POINT point;
                GetCursorPos(&point);
                TrackPopupMenu(hMenuList, TPM_RIGHTBUTTON, point.x, point.y, 0, hWnd, NULL);
            }
        }
        break;
    }
    return 0;
}

int frame_menu_event(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    pio_socket             pi_socket  = NULL;
    pkm_client             pclient     = NULL;
    pm_frame_file_control  pm_controls = NULL;
    int     nIndex=0;
    char    path[1024] = {0};
    char    *find = NULL;
    pi_socket = get_frame_io_socket(hWnd);
    if (pi_socket) {
        pclient     = (pkm_client)pi_socket->extend;
        pm_controls = &pclient->frame_file.m_controls;
        if (Message == IDR_COMMAND3) {
            nIndex = SendMessage(pm_controls->hComboEx, CB_GETCURSEL, 0, 0);
            if (nIndex == 0)
                return 0;
            ListView_DeleteAllItems(pm_controls->hListView);
            if(get_combox_text(pm_controls->hComboEx, nIndex, path)){
                path[strlen(path)-1] = '\0';
                find = strrchr(path,'\\');
                if (find == NULL){
                    view_combox_disk(&pclient->frame_file);
                }else{
                    find++;
                    *find = '\0';

                    kms_sock_send(pi_socket, CLIENT_FILE, CLIENT_GET_DIRECTORY, 0, path, strlen(path));
                    insert_foo_item(pclient, pm_controls, path);
                }
            }
        }else if(Message == IDR_COMMAND1) {
            ShowWindow(pm_controls->hListView, SW_SHOW);
            ShowWindow(pm_controls->hListFile, SW_HIDE);
        }else if(Message == IDR_COMMAND2) {
            ShowWindow(pm_controls->hListView, SW_HIDE);
            ShowWindow(pm_controls->hListFile, SW_SHOW);
        }
    }
    return 0;
}

int insert_foo_item(pkm_client pclient, pm_frame_file_control pm_controls, char *pstrItem)
{
    int nCount = SendMessage(pm_controls->hComboEx, CB_GETCOUNT, 0, 0);
    if (nCount == pclient->frame_file.m_data.comboxnum) {
        insert_combo_item(pm_controls->hComboEx, -1, 0, 2, pstrItem);
        SendMessage(pm_controls->hComboEx, CB_SETCURSEL, nCount, 0);
        pclient->frame_file.m_data.comboxcur = nCount;
    } else {
        set_combo_item(pm_controls->hComboEx, nCount-1, 0, 2, pstrItem);
        SendMessage(pm_controls->hComboEx, CB_SETCURSEL, nCount-1, 0);
        pclient->frame_file.m_data.comboxcur = nCount-1;
    }
    return 0;
}

int initialize_frame_file()
{
    WNDCLASSEX wcx;
    int i=0;
    SHFILEINFO shfi;
    char name[32];

    wcx.cbSize          = sizeof(WNDCLASSEX);
    wcx.style           = 0;//CS_HREDRAW | CS_VREDRAW;
    wcx.cbClsExtra      = 0;
    wcx.cbWndExtra      = 0;
    wcx.hInstance       = hInst;
    wcx.hIcon           = LoadIcon(NULL,IDI_APPLICATION);
    wcx.hIconSm         = NULL;
    wcx.hCursor         = LoadCursor(NULL,IDC_ARROW);
    wcx.hbrBackground   = (HBRUSH)(COLOR_BACKGROUND);
    wcx.lpszMenuName    = NULL;
    wcx.lpszClassName   = frame_cls_file;
    wcx.lpfnWndProc     = frame_file;

    if (!RegisterClassEx(&wcx))
        return -1;

    hHotyyImageList2= ImageList_Create(16, 16, ILC_COLOR32 |ILC_MASK , 1, 0);
    hHotyyImageList3= ImageList_Create(32, 32, ILC_COLOR32 |ILC_MASK , 1, 0);
    hHotyyImageList4= ImageList_Create(22, 22, ILC_COLOR32 |ILC_MASK , 1, 0);

    ImageList_AddIcon(hHotyyImageList4, LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLIENT_LIST_ICON1)));
    ImageList_AddIcon(hHotyyImageList4, LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLIENT_LIST_ICON2)));

    ImageList_AddIcon(hHotyyImageList2, LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLIENT_LIST_ICON1)));
    ImageList_AddIcon(hHotyyImageList2, LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLIENT_LIST_ICON2)));

    ImageList_AddIcon(hHotyyImageList3, LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLIENT_LIST_ICON1)));
    ImageList_AddIcon(hHotyyImageList3, LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLIENT_LIST_ICON2)));

    file_ico[0].image = ImageList_AddIcon(hHotyyImageList2, LoadIcon(hInst, MAKEINTRESOURCE(IDI_DISK)));
    file_ico[1].image = ImageList_AddIcon(hHotyyImageList2, LoadIcon(hInst, MAKEINTRESOURCE(IDI_DISK)));
    file_ico[2].image = ImageList_AddIcon(hHotyyImageList2, LoadIcon(hInst, MAKEINTRESOURCE(IDI_CDROM)));
    file_ico[3].image = ImageList_AddIcon(hHotyyImageList2, LoadIcon(hInst, MAKEINTRESOURCE(IDI_UNKNOWN)));

    file_ico[0].image = ImageList_AddIcon(hHotyyImageList3, LoadIcon(hInst, MAKEINTRESOURCE(IDI_DISK)));
    file_ico[1].image = ImageList_AddIcon(hHotyyImageList3, LoadIcon(hInst, MAKEINTRESOURCE(IDI_DISK)));
    file_ico[2].image = ImageList_AddIcon(hHotyyImageList3, LoadIcon(hInst, MAKEINTRESOURCE(IDI_CDROM)));
    file_ico[3].image = ImageList_AddIcon(hHotyyImageList3, LoadIcon(hInst, MAKEINTRESOURCE(IDI_UNKNOWN)));

    for ( i=file_ico_start-1; i<file_ico_size; i++ ) {
        memset(name, 0x0, 32);
        memset(&shfi, 0, sizeof(shfi));
        sprintf(name, "foo%s", file_ico[i].name);
        SHGetFileInfo(name, FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(shfi), SHGFI_ICON|SHGFI_USEFILEATTRIBUTES);
        file_ico[i].image = ImageList_AddIcon(hHotyyImageList2, shfi.hIcon);
        file_ico[i].image = ImageList_AddIcon(hHotyyImageList3, shfi.hIcon);
    }

    hMenuList = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU2));
    hMenuList = GetSubMenu(hMenuList, 0);
    return 1;
}

int create_frame_file(HWND hWndRoot, pio_socket i_socket)
{
    char title[64];
    pkm_client pclient = (pkm_client)i_socket->extend;
    pm_frame_file_control pframe_file = (pm_frame_file_control)&pclient->frame_file.m_controls;

    if (pframe_file->hWnd == NULL) {
        sprintf(title,"%s@%d", inet_ntoa(i_socket->remote_addr.sin_addr), (int)i_socket);
        pframe_file->hWnd = CreateWindowEx(0, frame_cls_file, title, WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN, 0, 0, 500, 400, hWndRoot, NULL, hInst, (LPVOID)i_socket);
    }
    if (pclient->frame_file.m_data.drive == NULL) {
        kms_sock_send(i_socket, CLIENT_FILE, CLIENT_GET_DRIVE, 0, NULL, 0);
    }

    UpdateWindow(pframe_file->hWnd);
    ShowWindow(pframe_file->hWnd, SW_SHOW);
    return 0;
}

int destroy_frame_file(pio_socket i_socket)
{
    pkm_client pclient = (pkm_client)i_socket->extend;
    pclient->frame_file.f_state = F_CLOSE;
    SendMessage(pclient->frame_file.m_controls.hWnd, WM_CLOSE, 0, 0);

    if (pclient->frame_file.m_data.drive != NULL)
        free(pclient->frame_file.m_data.drive);
    memset(&pclient->frame_file, 0x0, sizeof(m_frame_file));
    return 0;
}



int view_combox_disk(pm_frame_file pm_frame_file)
{
    char bufstatus[128]={0};
    char buff[128];
    char list[128];
    char drivesize1[64];
    char drivesize2[64];
    int i,nIndex;
    PDRIVE pDrive = (PDRIVE)pm_frame_file->m_data.drive;

    for ( i=0; i<pm_frame_file->m_data.comboxnum+1; i++ ) {
        SendMessage(pm_frame_file->m_controls.hComboEx, CBEM_DELETEITEM, 0, 0);
    }
    pm_frame_file->m_data.comboxnum = 0;

    insert_combo_item(pm_frame_file->m_controls.hComboEx, -1, 0, 1, "我的电脑");

    for ( i=0; i<pm_frame_file->m_data.drivenum; i++ ) {
        sprintf(list, "(%s)%s",((PDRIVE)pDrive+i)->name, lpDiskType[(int)((PDRIVE)pDrive+i)->type]);
        sprintf(drivesize1,"%-7.2f GB", ((PDRIVE)pDrive+i)->nTotal);
        sprintf(drivesize2,"%-7.2f GB", ((PDRIVE)pDrive+i)->nFree);
        sprintf(buff,"%s 可用空间:(%0.2fGB) 总空间:(%0.2fGB)", list, ((PDRIVE)pDrive+i)->nFree, ((PDRIVE)pDrive+i)->nTotal);

        insert_combo_item(pm_frame_file->m_controls.hComboEx, -1, 1, 2, buff);

        nIndex = insert_list_item(pm_frame_file->m_controls.hListView, list, i, 2);
        ListView_SetItemText(pm_frame_file->m_controls.hListView, nIndex, 1, drivesize1);
        ListView_SetItemText(pm_frame_file->m_controls.hListView, nIndex, 3, drivesize2);

        pm_frame_file->m_data.comboxnum ++;
    }

    pm_frame_file->m_data.comboxnum ++;
    SendMessage(pm_frame_file->m_controls.hComboEx, CB_SETCURSEL, 0, 0L);

    sprintf(bufstatus, "共有%d个对象", i);
    SetWindowText(pm_frame_file->m_controls.hStatus, bufstatus);
    return 0;
}

int view_list_net(pio_socket i_socket, char *buffer)
{
    pkm_client pclient = (pkm_client)i_socket->extend;
    PNET pnet = (PNET)(buffer+sizeof(io_data_header));
    while (pnet->next){
        insert_list_item(pclient->frame_file.m_controls.hListView, pnet->name,0,0);
        pnet = (PNET)(((char*)pnet) + pnet->next);
    }
    return 0;
}

int view_list_directory(pio_socket i_socket, char *buffer)
{
    SYSTEMTIME st;
    char bufstatus[256]={0};
    char filetime[64]={0};
    char filesize[64]={0};
    int iDirectory   = 0;
    int iCount       = 0;
    int nIndex  = 0;

    pkm_client pclient = (pkm_client)i_socket->extend;
    PDIRECTORY pDirectory = (PDIRECTORY)(buffer+sizeof(io_data_header));;

    while (pDirectory->next) {

        FileTimeToSystemTime(&pDirectory->lastwritetime, &st);
        sprintf(filetime, "%4d-%02d-%02d %02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);

        if (pDirectory->attributes & FILE_ATTRIBUTE_DIRECTORY){
            nIndex = insert_list_item(pclient->frame_file.m_controls.hListView, pDirectory->filename, iDirectory,file_ico[FILE_ICO_DIRECTORY].image);
            iDirectory ++ ;
        }else{
            if (pDirectory->filesize.QuadPart > 1024*1024*1024){
                sprintf(filesize, "%-7.2f GB", (float)pDirectory->filesize.QuadPart/1024/1024/1024);
            }else if (pDirectory->filesize.QuadPart > 1024*1024){
                sprintf(filesize, "%-7.2f MB", (float)pDirectory->filesize.QuadPart/1024/1024);
            }else if (pDirectory->filesize.QuadPart > 1024){
                sprintf(filesize, "%-7.0f KB", (float)pDirectory->filesize.QuadPart/1024);
            }else{
                sprintf(filesize, "%-7.0f Byte", (float)pDirectory->filesize.QuadPart);
            }

            nIndex = insert_list_item(pclient->frame_file.m_controls.hListView, pDirectory->filename, iCount, get_list_file_img(pDirectory->filename));
            ListView_SetItemText(pclient->frame_file.m_controls.hListView, nIndex, 1, filesize);
        }

        ListView_SetItemText(pclient->frame_file.m_controls.hListView, nIndex, 3, filetime);
        iCount ++ ;
        pDirectory = (PDIRECTORY)(((char*)pDirectory) + pDirectory->next);
    }
    sprintf(bufstatus, "共有%d个对象,包含%d个文件夹,%d个文件.", iCount, iDirectory, iCount-iDirectory);
    SetWindowText(pclient->frame_file.m_controls.hStatus, bufstatus);
    return 0;
}

int get_list_file_img(char *name)
{
    int i=0;
    char *find = strrchr(name,'.');
    if (find != NULL){
        for ( i=file_ico_start; i<file_ico_size; i++ ){
            if (stricmp(find,file_ico[i].name) == 0)
                return file_ico[i].image;
        }
    }
    return file_ico[FILE_ICO_UNDOWN].image;
}
