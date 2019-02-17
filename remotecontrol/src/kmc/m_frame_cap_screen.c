/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "m_frame_cap_screen.h"
#include "km_socket.h"
#include "frame_tools.h"

char frame_cls_cap_screen[] = "cls_cap_screen";
extern  HINSTANCE   hInst;

int cap_screen_tool(HWND hWnd,pm_frame_cap_screen pm_f_cap_screen)
{
    pm_f_cap_screen->hTool = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_VISIBLE|WS_CHILD|WS_BORDER| TBSTYLE_WRAPABLE | CCS_ADJUSTABLE, 0, 0, 0, 0, hWnd, (HMENU)IDR_TOOLBAR4, hInst, NULL);

    pm_f_cap_screen->hCombox = CreateWindowEx(0, WC_COMBOBOX, NULL, WS_VISIBLE|WS_CHILD|CBS_SIMPLE|CBS_DROPDOWN|CBS_DROPDOWNLIST|CBS_SORT|WS_VSCROLL, 0, 2, 280, 100, pm_f_cap_screen->hTool, (HMENU)IDR_COMBOEX, hInst, NULL);

    pm_f_cap_screen->hReCombox = CreateWindowEx(0, WC_BUTTON, "刷新", WS_VISIBLE|WS_CHILD, 284, 1, 60, 22, pm_f_cap_screen->hTool,(HMENU)IDR_COM_RE_COUNT, hInst, NULL);

    pm_f_cap_screen->hStartStop = CreateWindowEx(0, WC_BUTTON, "开始", WS_VISIBLE|WS_CHILD, 348, 1, 60, 22, pm_f_cap_screen->hTool,(HMENU)IDR_COM_STARTSTOP, hInst, NULL);

    pm_f_cap_screen->hImage = CreateWindowEx(0, WC_BUTTON, "拍照", WS_VISIBLE|WS_CHILD, 412, 1, 60, 22, pm_f_cap_screen->hTool,(HMENU)IDR_COM_IMAGE, hInst, NULL);

    SetWindowFont(pm_f_cap_screen->hCombox);
    SetWindowFont(pm_f_cap_screen->hReCombox);
    SetWindowFont(pm_f_cap_screen->hStartStop);
    SetWindowFont(pm_f_cap_screen->hImage);
    return 0;
}

LRESULT __stdcall frame_cap_screen(HWND hWnd,UINT Message, WPARAM wParam, LPARAM lParam)
{
    pio_socket pi_socket;
    pkm_client pclient     = NULL;
    pm_frame_cap_screen pframe_cap_screen;
    switch(Message)
    {
    case WM_CREATE:
        {
            int x = (GetSystemMetrics(SM_CXSCREEN)-500)/2;
            int y = (GetSystemMetrics(SM_CYSCREEN)-400)/2;
            MoveWindow(hWnd, x, y, 500, 400, TRUE);
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                pclient = (pkm_client)pi_socket->extend;
                cap_screen_tool(hWnd, &pclient->frame_cap_screen);
            }
        }
        break;
    case WM_R_CAP_SCREEN_COUNT:
        {
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                pclient = (pkm_client)pi_socket->extend;
                pframe_cap_screen = &pclient->frame_cap_screen;
                int i=0,count;
                count = SendMessage(pframe_cap_screen->hCombox, CB_GETCOUNT, 0L, 0L);
                for (i=0; i<count; i++)
                    SendMessage(pframe_cap_screen->hCombox, CB_DELETESTRING, 0, 0L);

                char* p_session_info = pframe_cap_screen->psession_info;
                char *buff = strtok(p_session_info, "|");

                while(buff != NULL) {
                    SendMessage(pframe_cap_screen->hCombox, CB_ADDSTRING, 0, (DWORD)buff);
                    buff = strtok(NULL, "|");
                }
                SendMessage(pframe_cap_screen->hCombox, CB_SETCURSEL, 0, 0);
            }
        }
        break;
    case WM_R_CAP_SCREEN:
        {
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                pclient = (pkm_client)pi_socket->extend;
                pframe_cap_screen = &pclient->frame_cap_screen;
                RECT rect;
                GetClientRect(hWnd, &rect);
                HDC hdc = GetDC(hWnd);

                BITMAPINFOHEADER bih;
                memset( &bih, 0, sizeof( bih ));
                bih.biSize = sizeof(bih);
                bih.biWidth = 320;
                bih.biHeight = 240;
                bih.biPlanes = 1;
                bih.biBitCount = 24;

                StretchDIBits(hdc, 0, 0,
                                    rect.right, rect.bottom,
                    0,0,
                    320,
                    240,
                    pframe_cap_screen->i_buffer,
                    (LPBITMAPINFO)&bih, DIB_RGB_COLORS, SRCCOPY);
                ReleaseDC(hWnd, hdc);
            }
        }
        break;
    case WM_SIZE:
        {
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                pclient = (pkm_client)pi_socket->extend;
                HDWP hDwp;
                hDwp  = BeginDeferWindowPos(1);
                DeferWindowPos(hDwp, pclient->frame_cap_screen.hTool, NULL, 0, 0, 0, 0, SWP_NOZORDER);
                EndDeferWindowPos(hDwp);
                SendMessage(hWnd, WM_R_CAP_SCREEN, 0, 0);
            }
        }
        break;
    case WM_COMMAND:
        {
            int iWmId    = LOWORD(wParam);
            pi_socket = get_frame_io_socket(hWnd);
            if (!pi_socket)
                return 0;

            pclient     = (pkm_client)pi_socket->extend;
            pframe_cap_screen = &pclient->frame_cap_screen;

            switch(iWmId)
            {
            case IDR_COM_RE_COUNT:
                kms_sock_send(pi_socket, 4, 0, 0, NULL, 0);
                SetFocus(hWnd);
                break;
            case IDR_COM_STARTSTOP:
                if (pframe_cap_screen->hStartStop_State == 0) {
                    int index = SendMessage(pframe_cap_screen->hCombox, CB_GETCURSEL, 0, 0L);
                    if (index == -1)
                        break;
                    pframe_cap_screen->hStartStop_State = 1;

                    SetWindowText(pframe_cap_screen->hStartStop, "停止");

                    kms_sock_send(pi_socket, 4, 1, (unsigned long)pi_socket->extend, (char*)&index, sizeof(int));
                } else {
                    pframe_cap_screen->hStartStop_State = 0;
                    SetWindowText(pframe_cap_screen->hStartStop, "开始");
                }
                SetFocus(hWnd);
                break;
            case IDR_COM_IMAGE:
                pframe_cap_screen->isImage = 1;
                SetFocus(hWnd);
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
                pframe_cap_screen = &pclient->frame_cap_screen;
                pframe_cap_screen->hWnd = NULL;
                pframe_cap_screen->hStartStop_State = 0;
                destroy_frame_cap_screen(pi_socket);
            }
            DestroyWindow(hWnd);
        }
        break;
    }
    return DefWindowProc(hWnd,Message,wParam,lParam);
}

int initialize_frame_cap_screen()
{
    WNDCLASSEX wcx;
    wcx.cbSize          = sizeof(WNDCLASSEX);
    wcx.style           = 0;//CS_HREDRAW | CS_VREDRAW;
    wcx.cbClsExtra      = 0;
    wcx.cbWndExtra      = 0;
    wcx.hInstance       = hInst;
    wcx.hIcon           = LoadIcon(NULL,IDI_APPLICATION);
    wcx.hIconSm         = NULL;
    wcx.hCursor         = LoadCursor(NULL,IDC_ARROW);
    wcx.hbrBackground   = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcx.lpszMenuName    = NULL;
    wcx.lpszClassName   = frame_cls_cap_screen;
    wcx.lpfnWndProc     = frame_cap_screen;

    if (!RegisterClassEx(&wcx))
        return -1;
    return 0;
}

int create_frame_cap_screen(HWND hWndRoot, pio_socket i_socket)
{
    char title[64];
    pkm_client pclient = (pkm_client)i_socket->extend;

    pm_frame_cap_screen pframe_cap_screen = (pm_frame_cap_screen)&pclient->frame_cap_screen;

    if (pframe_cap_screen->hWnd == NULL) {
        pframe_cap_screen->hStartStop_State = 0;
        sprintf(title,"%s@%d", inet_ntoa(i_socket->remote_addr.sin_addr), (int)i_socket);
        pframe_cap_screen->hWnd = CreateWindowEx(0, frame_cls_cap_screen, title, WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN, 0, 0, 500, 400, hWndRoot, NULL, hInst, (LPVOID)i_socket);
    }

    UpdateWindow(pframe_cap_screen->hWnd);
    ShowWindow(pframe_cap_screen->hWnd, SW_SHOW);
    return 0;
}

int free_frame_cap_screen(pio_socket i_socket)
{
    pkm_client pclient = (pkm_client)i_socket->extend;

    if (pclient->frame_cap_screen.hStartStop_State != 0) {
        SendMessage(pclient->frame_cap_screen.hWnd, WM_COMMAND, IDR_COM_STARTSTOP, 0);
    }

    //destroy_frame_cap_screen(i_socket);
    return 0;
}

int destroy_frame_cap_screen(pio_socket i_socket)
{
    pkm_client pclient = (pkm_client)i_socket->extend;
    SendMessage(pclient->frame_cap_screen.hWnd, WM_CLOSE, 0, 0);
    if (pclient->frame_cap_screen.i_length > 0) {
        pclient->frame_cap_screen.i_length = 0;
        free(pclient->frame_cap_screen.i_buffer);
    }
    if (pclient->frame_cap_screen.psession_info != NULL) {
        free(pclient->frame_cap_screen.psession_info);
        pclient->frame_cap_screen.psession_info = NULL;
    }
    return 0;
}
