#include "main_frame_log.h"
#include "frame_tools.h"

extern HWND hWndLogList;

int  main_frame_draw_log_col()
{
    insert_list_column_text(hWndLogList, 0, 100, "时间");
    insert_list_column_text(hWndLogList, 1, 100, "说明");
    return 0;
}

int  main_frame_draw_log_col_width(int right)
{
    ListView_SetColumnWidth(hWndLogList, 0, 100);
    ListView_SetColumnWidth(hWndLogList, 1, right-100);
    return 0;
}

int _log(char *line,...)
{
    int i = 0;
    LVITEM lvi;
    SYSTEMTIME systemtime;
    char message[4096]={0};
    char time[32];

    va_list vl;
    va_start(vl, line);
    vsprintf(message+strlen(message), line, vl);
    va_end(vl);

    GetLocalTime(&systemtime);
    sprintf(time, "%d点%d分%d秒", systemtime.wHour, systemtime.wMinute, systemtime.wSecond);

    lvi.mask        = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM | LVIF_STATE;
    lvi.state       = LVIS_SELECTED;
    lvi.stateMask   = 0;
    lvi.iSubItem    = 0;
    lvi.pszText     = time;
    lvi.iImage      = 0;
    lvi.iItem       = ListView_GetItemCount(hWndLogList);
    lvi.iImage      = 1;

    i = ListView_InsertItem(hWndLogList, &lvi);

    ListView_SetItemText(hWndLogList, i, 1, message);
    return 0;
}
