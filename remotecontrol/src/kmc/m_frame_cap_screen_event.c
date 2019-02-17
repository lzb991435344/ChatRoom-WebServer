/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "m_frame_cap_screen_event.h"
#include "m_frame_cap_screen.h"
#include "km_socket.h"
#include "lzw.h"

extern pshare_main g_share_main;

kms_event_func cap_screen_event_funcs[] =
{
    event_m_cap_screen_frame,
    event_m_cap_screen_count
};

int initialize_km_cap_screen_event()
{
    g_share_main->event_funcs[4].count          = sizeof(cap_screen_event_funcs)/sizeof(kms_event_func);
    g_share_main->event_funcs[4].event_funcs    = cap_screen_event_funcs;
    return 0;
}

int event_m_cap_screen_count(pio_socket i_socket, char *buffer, int lengthdd)
{
    pio_data_header pi_data_header = (pio_data_header)buffer;
    pkm_client pclient = (pkm_client)i_socket->extend;
    printf("%s\n", buffer + sizeof(io_data_header));

    pm_frame_cap_screen pm_f_cap_screen = &pclient->frame_cap_screen;
    if (pm_f_cap_screen->psession_info != NULL) {
        free(pm_f_cap_screen->psession_info);
        pm_f_cap_screen->psession_info = NULL;
    }

    pm_f_cap_screen->session_info_count = (pi_data_header->length - sizeof(io_data_header));
    pm_f_cap_screen->psession_info = malloc(pm_f_cap_screen->session_info_count  +1);
    memcpy(pm_f_cap_screen->psession_info, buffer + sizeof(io_data_header), pm_f_cap_screen->session_info_count);
    pm_f_cap_screen->psession_info[pm_f_cap_screen->session_info_count] = '\0';

    PostMessage(pm_f_cap_screen->hWnd, WM_R_CAP_SCREEN_COUNT, 0, 0);

    return SOCK_LOOP_RECV_HEADER;
}

BOOL CAM_SaveBitmap(BYTE * pBuffer, long lBufferSize )
{
    SYSTEMTIME systemtime;
    char time[32];
    char bmp_path[MAX_PATH];

    GetLocalTime(&systemtime);
    sprintf(time, "%04d_%02d_%02d_%02d_%02d_%02d.bmp", systemtime.wYear, systemtime.wMonth, systemtime.wDay, systemtime.wHour, systemtime.wMinute, systemtime.wSecond);
    sprintf(bmp_path, "%sphoto_cam\\%s", g_share_main->km_cur_path, time);

    HANDLE hf = CreateFile(bmp_path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    if( hf == INVALID_HANDLE_VALUE )
        return 0;

    BITMAPFILEHEADER bfh;
    memset( &bfh, 0, sizeof( bfh ));
    bfh.bfType = 0x4D42; // 'MB';
    bfh.bfSize = sizeof(bfh) + lBufferSize + sizeof( BITMAPINFOHEADER );
    bfh.bfOffBits = sizeof( BITMAPINFOHEADER ) + sizeof( BITMAPFILEHEADER );
    DWORD dwWritten = 0;
    WriteFile( hf, &bfh, sizeof(bfh), &dwWritten, NULL);
    //写位图格式
    BITMAPINFOHEADER bih;
    memset( &bih, 0, sizeof( bih ));
    bih.biSize = sizeof(bih);
    bih.biWidth = 320;
    bih.biHeight = 240;
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    WriteFile( hf, &bih, sizeof( bih ), &dwWritten, NULL);
    WriteFile( hf, pBuffer, lBufferSize, &dwWritten, NULL);
    CloseHandle( hf );

    /*HDC hdc = GetDC(hWnd);

    StretchDIBits(hdc, 0, 0,
                        iWidth, iHeight,
        0,0,
        iWidth,
        iHeight,
        pBuffer,
        (LPBITMAPINFO)&bih, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(hWnd, hdc);*/
    return 0;
}

int event_m_cap_screen_frame(pio_socket i_socket, char *buffer, int lengthdd)
{
    int outlen;
    char *data_buffer = buffer + sizeof(io_data_header);
    int length = lengthdd - sizeof(io_data_header);
    pkm_client pclient = (pkm_client)i_socket->extend;
    pm_frame_cap_screen pm_f_cap_screen = &pclient->frame_cap_screen;

    if (pm_f_cap_screen->hStartStop_State == 0) {
        kms_sock_send(i_socket, 5, 2, 0, NULL, 0);
        return SOCK_LOOP_CLOSE;
    }

    if (pm_f_cap_screen->i_length == 0) {
        pm_f_cap_screen->i_buffer = malloc(4096000);
        pm_f_cap_screen->i_length = 4096000;
    }

    outlen = pm_f_cap_screen->i_length;

    lzw_dechode(data_buffer, length, pm_f_cap_screen->i_buffer, &outlen);

    if (pm_f_cap_screen->isImage == 1) {
        pm_f_cap_screen->isImage = 0;
        CAM_SaveBitmap(pm_f_cap_screen->i_buffer, outlen);
    }

    //DWORD wColSize = sizeof(RGBQUAD)*((8 <= 8) ? 1<<8 : 0);
    PostMessage(pm_f_cap_screen->hWnd, WM_R_CAP_SCREEN, 0, 0);

    printf("event_m_screen_cap_frame:%d:%d\n", (int)length, outlen);
    return SOCK_LOOP_RECV_HEADER;
}
