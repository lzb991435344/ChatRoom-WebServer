/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "m_frame_screen_event.h"
#include "m_frame_screen.h"
#include "km_socket.h"
#include "lzw.h"

kms_event_func screen_event_funcs[] =
{
    event_m_screen_frame,
    event_m_screen_count
};

int initialize_km_screen_event()
{
    g_share_main->event_funcs[3].count          = sizeof(screen_event_funcs)/sizeof(kms_event_func);
    g_share_main->event_funcs[3].event_funcs    = screen_event_funcs;
    return 0;
}

int event_m_screen(pio_socket i_socket,pm_frame_screen pframe_screen, pscreen_ver pc_ver)
{
    //pscreen_ver pc_ver = i_
    frame_screen_xy(pframe_screen, pc_ver->wXscreen, pc_ver->wYscreen);
    kms_sock_send(i_socket, 3, 1, 0, NULL, 0);
    return 0;
}

int event_m_screen_count(pio_socket i_socket, char *buffer, int lengthdd)
{
    pio_data_header pi_data_header = (pio_data_header)buffer;
    pkm_client pclient = (pkm_client)i_socket->extend;
    pm_frame_screen pm_f_screen = &pclient->frame_screen;
    if (pm_f_screen->psession_info != NULL) {
        free(pm_f_screen->psession_info);
        pm_f_screen->psession_info = NULL;
    }
    pm_f_screen->session_info_count = (pi_data_header->length - sizeof(io_data_header))/sizeof(WTS_SESSION_INFO);
    pm_f_screen->psession_info = malloc(pm_f_screen->session_info_count  * sizeof(WTS_SESSION_INFO));
    memcpy(pm_f_screen->psession_info, buffer + sizeof(io_data_header), pm_f_screen->session_info_count  * sizeof(WTS_SESSION_INFO));

    PostMessage(pm_f_screen->hWnd, WM_R_SCREEN_COUNT, 0, 0);
    return SOCK_LOOP_RECV_HEADER;
}

BOOL SCREEN_SaveBitmap(BYTE * pBuffer, long lBufferSize )
{
    DWORD dwPaletteSize = (1 << 8) *sizeof(RGBQUAD);
    BITMAPFILEHEADER   bmfHdr;
    DWORD dwWritten;

    SYSTEMTIME systemtime;
    char time[32];
    char bmp_path[MAX_PATH];

    GetLocalTime(&systemtime);
    sprintf(time, "%04d_%02d_%02d_%02d_%02d_%02d.bmp", systemtime.wYear, systemtime.wMonth, systemtime.wDay, systemtime.wHour, systemtime.wMinute, systemtime.wSecond);
    sprintf(bmp_path, "%sphoto_screen\\%s", g_share_main->km_cur_path, time);

    HANDLE fh = CreateFile(bmp_path, GENERIC_WRITE,0, NULL, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (fh == INVALID_HANDLE_VALUE)
       return 0;
    bmfHdr.bfType = 0x4D42;  // "BM"
    bmfHdr.bfSize = lBufferSize;
    bmfHdr.bfReserved1 = 0;
    bmfHdr.bfReserved2 = 0;
    bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;

    WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
    WriteFile(fh, (LPSTR)pBuffer, lBufferSize, &dwWritten, NULL);
    CloseHandle(fh);

    return 0;
}

int event_m_screen_frame(pio_socket i_socket, char *buffer, int lengthdd)
{
    int outlen;
    char *data_buffer = buffer + sizeof(io_data_header);
    int length = lengthdd - sizeof(io_data_header);
    pkm_client pclient = (pkm_client)i_socket->extend;
    pm_frame_screen pm_f_screen = &pclient->frame_screen;

    if (pm_f_screen->hStartStop_State == 0) {
        kms_sock_send(i_socket, 3, 2, 0, NULL, 0);
        return SOCK_LOOP_CLOSE;
    }

    if (pm_f_screen->i_length == 0) {
        pm_f_screen->i_buffer = malloc(4096000);
        pm_f_screen->i_length = 4096000;
    }

    outlen = pm_f_screen->i_length;

    lzw_dechode(data_buffer, length, pm_f_screen->i_buffer, &outlen);

    if (pm_f_screen->isImage == 1) {
        pm_f_screen->isImage = 0;
        SCREEN_SaveBitmap(pm_f_screen->i_buffer, outlen);
    }

    /*
    DWORD dwPaletteSize = (1 << 8) *sizeof(RGBQUAD);
    BITMAPFILEHEADER   bmfHdr;
    DWORD dwWritten;
    HANDLE fh = CreateFile("c:\\aabb.bmp", GENERIC_WRITE,0, NULL, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (fh == INVALID_HANDLE_VALUE)
       return 0;
    bmfHdr.bfType = 0x4D42;  // "BM"
    bmfHdr.bfSize = length;
    bmfHdr.bfReserved1 = 0;
    bmfHdr.bfReserved2 = 0;
    bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;

    WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
    WriteFile(fh, (LPSTR)data_buffer, length, &dwWritten, NULL);
    CloseHandle(fh);*/

    //DWORD wColSize = sizeof(RGBQUAD)*((8 <= 8) ? 1<<8 : 0);
    PostMessage(pm_f_screen->hWnd, WM_R_SCREEN, 0, 0);

    kms_sock_send(i_socket, 3, 1, 0, NULL, 0);

    printf("event_m_screen_frame:%d:%d\n", (int)length, outlen);
    return SOCK_LOOP_RECV_HEADER;
}
