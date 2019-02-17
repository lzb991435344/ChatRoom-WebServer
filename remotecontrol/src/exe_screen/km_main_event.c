/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "km_main_event.h"
#include "lzw.h"

kms_event_func event_funcs[] = {
    NULL,
    kms_main_screen_frame,
    kms_main_screen_frame_exit,
    kms_main_screen_mouse_click,
    kms_main_screen_mouse_right,
    kms_main_screen_char,
    kms_main_screen_count,
    kms_main_screen_mouse_click_up,
    kms_main_screen_mouse_right_up
};

extern HANDLE        hdib;
extern unsigned long crc32;
extern screen_ver s_ver;

int
km_main_connect(int ret, DWORD dwTrans, pio_socket i_socket, LPOVERLAPPED lpoverlapped)
{
    if (ret == 0) {
        return SOCK_LOOP_CLOSE;
    }

    g_share_main->event_funcs[3].event_funcs = event_funcs;
    g_share_main->event_funcs[3].count = 7;

    i_socket->f_call_work = (void*)km_main_parse;

    i_socket->i_length = g_share_main->km_length;
    i_socket->i_buffer = g_share_main->km_buffer;

    kms_sock_send(i_socket, 0, 0, crc32, (char*)&s_ver, sizeof(screen_ver));

    return SOCK_LOOP_RECV_HEADER;
}

int
km_main_parse(int ret, DWORD dwTrans, pio_socket i_socket, LPOVERLAPPED lpoverlapped)
{
    pio_data_header pi_data_header;

    if ( (ret == 0) || (dwTrans == 0) ) {
        goto _exit_err;
    }
    i_socket->r_count += dwTrans;
    if (i_socket->r_count < sizeof(io_data_header)) {
        i_socket->r_buf.buf = i_socket->i_buffer + i_socket->r_count;
        i_socket->r_buf.len = sizeof(io_data_header) - i_socket->r_count;
        return SOCK_LOOP_RECV_NEXT;
    }
    pi_data_header = (pio_data_header)i_socket->i_buffer;
    if (i_socket->r_count < pi_data_header->length) {
        i_socket->r_buf.buf = i_socket->i_buffer + i_socket->r_count;
        i_socket->r_buf.len = pi_data_header->length - i_socket->r_count;
        return SOCK_LOOP_RECV_NEXT;
    }
    i_socket->i_buffer[i_socket->r_count] = '\0';
    //printf("%d:%d %d:%d\n", ret, (int)dwTrans, pi_data_header->major_cmd, pi_data_header->minor_cmd);
    return g_share_main->event_funcs[pi_data_header->major_cmd].event_funcs[pi_data_header->minor_cmd]
                                (i_socket, i_socket->i_buffer, i_socket->r_count);
_exit_err:
    return SOCK_LOOP_CLOSE;
}

int
kms_main_close(pio_socket i_socket, char *buffer, int length)
{
    //printf("%s\n","kms_main_close");
    closesocket(i_socket->socket);
    return SOCK_LOOP_RE_COUNT;
}

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
	GetDIBits(hdc, hbitmap, 0, bitmap.bmHeight, lpBits, (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);
	DeleteDC(hdc);

	int outlength=i_socket->i_length;

    lzw_enchode((unsigned char*)lpbi, dwSize, i_socket->i_buffer, &outlength);

	//printf("MakeDib:%d:%d\n", kms_sock_send(i_socket, 3, 0, 0, i_socket->i_buffer, outlength), outlength);
    kms_sock_send(i_socket, 3, 0, 0, i_socket->i_buffer, outlength);

    GlobalUnlock(hdib);
    //GlobalFree(hdib);
	return hdib ;
}

int
kms_main_screen_frame(pio_socket i_socket, char *buffer, int length)
{
    //printf("%s\n", "kms_main_screen_frame");
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
    ReleaseDC(hwnd, hsrc);

    MakeDib(i_socket, hbmp, 8);

    return SOCK_LOOP_RECV_HEADER;
}

int kms_main_screen_frame_exit(pio_socket i_socket, char *buffer, int length)
{
    //printf("%s\n","kms_main_screen_frame_exit");
    return SOCK_LOOP_CLOSE;
}

int screen_socket_close(pio_socket i_socket)
{
    GlobalFree(hdib);
    //printf("%s\n","screen_socket_close");
    return 0;
}

int
kms_main_screen_mouse_click(pio_socket i_socket, char *buffer, int length)
{
    //printf("%s\n", "kms_main_screen_mouse_click");
    POINT *ptCursor = (POINT*)(buffer + sizeof(io_data_header));
    SetCursorPos(ptCursor->x, ptCursor->y);
    mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
    mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
    return SOCK_LOOP_RECV_HEADER;
}

int
kms_main_screen_mouse_right(pio_socket i_socket, char *buffer, int length)
{
    //printf("%s\n", "kms_main_screen_mouse_right");
    POINT *ptCursor = (POINT*)(buffer + sizeof(io_data_header));
    SetCursorPos(ptCursor->x, ptCursor->y);
    mouse_event(MOUSEEVENTF_RIGHTDOWN,0,0,0,0);
    mouse_event(MOUSEEVENTF_RIGHTUP,0,0,0,0);
    return SOCK_LOOP_RECV_HEADER;
}

int
kms_main_screen_char(pio_socket i_socket, char *buffer, int length)
{
    pio_data_header pi_data_header = (pio_data_header)buffer;

    keybd_event((byte)pi_data_header->crc32, MapVirtualKey((byte)pi_data_header->crc32, 0), 0, 0);
	keybd_event((byte)pi_data_header->crc32, MapVirtualKey((byte)pi_data_header->crc32, 0), KEYEVENTF_KEYUP, 0);

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
            OutputDebugString(sessionInfo[i].pWinStationName);
        }
        k_sock_send(i_socket, 3, 1, 0, (char*)sessionInfo, sizeof(WTS_SESSION_INFO)*sessionInfoCount);
        k_WTSFreeMemory(sessionInfo);
    }
    return SOCK_LOOP_RECV_HEADER;
}

int kms_main_screen_mouse_click_up(pio_socket i_socket, char *buffer, int length)
{
    //printf("%s\n", "kms_main_screen_mouse_click_up");
    POINT *ptCursor = (POINT*)(buffer + sizeof(io_data_header));
    SetCursorPos(ptCursor->x, ptCursor->y);
    mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
    return SOCK_LOOP_RECV_HEADER;
}

int kms_main_screen_mouse_right_up(pio_socket i_socket, char *buffer, int length)
{
    //printf("%s\n", "kms_main_screen_mouse_right_up");
    POINT *ptCursor = (POINT*)(buffer + sizeof(io_data_header));
    SetCursorPos(ptCursor->x, ptCursor->y);
    mouse_event(MOUSEEVENTF_RIGHTDOWN,0,0,0,0);
    return SOCK_LOOP_RECV_HEADER;
}
