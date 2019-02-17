/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "m_make_file.h"
#include "file.h"

extern long g_lineport;

BOOL __stdcall dlg_proc_make_file(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
    case WM_INITDIALOG:
        {
            char line_port[256] = {0};
            ltoa(g_lineport, line_port, 10);

            SetWindowText(GetDlgItem(hWnd, IDC_EDIT_PORT), line_port);
            SetWindowText(GetDlgItem(hWnd, IDC_EDIT_SERVER), "KMS");
            SetWindowText(GetDlgItem(hWnd, IDC_EDIT_SERVER_NAME), "Ke Min Server");
        }
        break;
    case WM_COMMAND:
        {
            if (LOWORD(wParam) == ID_BT_CANCEL) {
                EndDialog(hWnd, -1);
            } else if (LOWORD(wParam) == ID_BT_MAKE_FILE) {
                char addr[MAX_PATH] = {0};
                char port[32] = {0};
                char server[256] = {0};
                char server_name[256] = {0};
                GetWindowText(GetDlgItem(hWnd, IDC_EDIT_ADDR), addr, MAX_PATH);
                GetWindowText(GetDlgItem(hWnd, IDC_EDIT_PORT), port, MAX_PATH);
                GetWindowText(GetDlgItem(hWnd, IDC_EDIT_SERVER), server, MAX_PATH);
                GetWindowText(GetDlgItem(hWnd, IDC_EDIT_SERVER_NAME), server_name, MAX_PATH);
                if ((strlen(addr) > 1) && (strlen(port) > 1) && (strlen(server) > 1) && (strlen(server_name) > 1)) {
                    make_file(addr, port, server, server_name);
                } else {
                    MessageBox(hWnd, "请输入正确的配置,然后再生成客户端.", KM_NAME, MB_ICONQUESTION);
                }
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

int make_file_initialize(HWND hWnd)
{
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_MAKE_FILE), hWnd, dlg_proc_make_file, 0);
    return 0;
}

int make_file(char *addr, char *port, char *server, char *server_name)
{
    char info[1024] = {0};
    char path[MAX_PATH] = {0};
    unsigned long length;
    sprintf(path, "%skms.exe", g_share_main->km_cur_path);

    char *data = file_read(path, &length);
    if (data == NULL) {
        sprintf(info, "读取文件错误:%s", path);
        MessageBox(NULL, info, KM_NAME, MB_ICONQUESTION);
        return -1;
    }

    memcpy(&data[0x2e20], addr, strlen(addr)+1);
    memcpy(&data[0x3020], port, strlen(port)+1);
    memcpy(&data[0x2e60], server, strlen(server)+1);
    memcpy(&data[0x2ea0], server_name, strlen(server_name)+1);

    if (file_write(path, data, length)) {
        sprintf(info, "生成文件完毕:%s", path);
    } else {
        sprintf(info, "写入文件错误:%s", path);
    }
    MessageBox(NULL, info, KM_NAME, MB_ICONQUESTION);
    return 0;
}
