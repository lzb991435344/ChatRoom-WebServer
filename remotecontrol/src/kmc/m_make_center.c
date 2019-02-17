/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "m_make_center.h"

extern pshare_main g_share_main;
extern long g_lineport;
extern long g_cmdport;

BOOL __stdcall dlg_proc_make_center(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
    case WM_INITDIALOG:
        {
            char line_port[256] = {0};
            char cmd_port[256] = {0};
            ltoa(g_lineport, line_port, 10);
            ltoa(g_cmdport, cmd_port, 10);
            SetWindowText(GetDlgItem(hWnd, IDC_EDIT_LINE_PORT), line_port);
            SetWindowText(GetDlgItem(hWnd, IDC_EDIT_CMD_PORT), cmd_port);
        }
        break;
    case WM_COMMAND:
        {
            if (LOWORD(wParam) == ID_BT_CANCEL) {
                EndDialog(hWnd, -1);
            } else if (LOWORD(wParam) == ID_BT_OK) {
                char config_path[MAX_PATH];
                sprintf(config_path, "%skmc.ini", g_share_main->km_cur_path);

                char line_port[256] = {0};
                char cmd_port[256] = {0};

                GetWindowText(GetDlgItem(hWnd, IDC_EDIT_LINE_PORT), line_port, MAX_PATH);
                GetWindowText(GetDlgItem(hWnd, IDC_EDIT_CMD_PORT), cmd_port, MAX_PATH);

                WritePrivateProfileString("CONFIG", "line_port", line_port, config_path);
                WritePrivateProfileString("CONFIG", "cmd_port", cmd_port, config_path);
                EndDialog(hWnd, -1);
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

int make_center_initialize(HWND hWnd)
{
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_CENTER_CONFIG), hWnd, dlg_proc_make_center, 0);
    return 0;
}

int make_config_initialize()
{
    char config_path[MAX_PATH];
    sprintf(config_path, "%skmc.ini", g_share_main->km_cur_path);

    g_lineport = GetPrivateProfileInt("CONFIG", "line_port", 2010, config_path);
    g_cmdport = GetPrivateProfileInt("CONFIG", "cmd_port", 100, config_path);
    return 0;
}
