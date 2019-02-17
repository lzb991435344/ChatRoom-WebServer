/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "frame_tools.h"

int open_file_path(char *path, char *file, HWND hWnd)
{
	OPENFILENAME ofn;
	memset(&ofn,0x0,sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = "All Files(*.*)\0*.*\0\0";
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = path;
	ofn.nMaxFile = 1024;
	ofn.lpstrFileTitle = file;
	ofn.nMaxFileTitle = 128;
	ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_EXPLORER;
	if (GetOpenFileName(&ofn))
		return 1;
	return 0;
}

int save_file_path(char *path, HWND hWnd)
{
	OPENFILENAME ofn;
	memset(&ofn,0x0,sizeof(ofn));
	char szFile[512]="aa";
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = "All Files(*.*)\0*.*\0\0";
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = path;
	ofn.nMaxFile = 1024;
	ofn.lpstrFileTitle = szFile;
	ofn.nMaxFileTitle = sizeof(szFile);
	ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_EXPLORER;
	if (GetSaveFileName(&ofn))
		return 1;
	return 0;
}

int open_file_directory(char *path, char *title, HWND hWnd)
{
    BROWSEINFO bi;
    ZeroMemory(&bi, sizeof(BROWSEINFO));
    bi.hwndOwner=hWnd;
    bi.lpszTitle=title;
    bi.ulFlags=BIF_RETURNONLYFSDIRS;     //属性你可自己选择
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != NULL){
        SHGetPathFromIDList(pidl, path);
        return 1;
    }
    return 0;
}

int get_combox_text(HWND m_hWnd, int nIndex, char* lpszItem)
{
    COMBOBOXEXITEM cbex = { 0 };
    cbex.mask = CBEIF_TEXT;
    cbex.iItem = nIndex;
    cbex.pszText = lpszItem;
    cbex.cchTextMax = 256;

    return SendMessage(m_hWnd, CBEM_GETITEM, 0, (LPARAM)&cbex);
}

int insert_combo_item(HWND hWnd, int iItem, int iIndent, int iImage, char *lpStr)
{
    COMBOBOXEXITEM ComboItem;
    memset(&ComboItem, 0x0, sizeof(ComboItem));
    ComboItem.mask = CBEIF_DI_SETITEM | CBEIF_IMAGE | CBEIF_INDENT | CBEIF_LPARAM | CBEIF_OVERLAY | CBEIF_SELECTEDIMAGE | CBEIF_TEXT;
    ComboItem.cchTextMax = 256;
    ComboItem.pszText = lpStr;
    ComboItem.iItem = iItem;  // or any value -1, 0, 1, 2
    ComboItem.iImage = iImage;
    ComboItem.iIndent = iIndent;
    ComboItem.iSelectedImage = iImage;
    return (int)SendMessage(hWnd, CBEM_INSERTITEM, 0, (LPARAM)&ComboItem);
}

int set_combo_item(HWND hWnd, int iItem, int iIndent, int iImage, char *lpStr)
{
    COMBOBOXEXITEM ComboItem;
    memset(&ComboItem,0x0,sizeof(ComboItem));
    ComboItem.mask = CBEIF_DI_SETITEM | CBEIF_IMAGE | CBEIF_INDENT | CBEIF_LPARAM | CBEIF_OVERLAY | CBEIF_SELECTEDIMAGE | CBEIF_TEXT;
    ComboItem.cchTextMax = 256;
    ComboItem.pszText = lpStr;
    ComboItem.iItem = iItem;  // or any value -1, 0, 1, 2
    ComboItem.iImage = iImage;
    ComboItem.iIndent = iIndent;
    ComboItem.iSelectedImage = iImage;
    return (int)SendMessage(hWnd, CBEM_SETITEM, 0, (LPARAM)&ComboItem);
}

int insert_list_item(HWND hWndList, char *str, int iItem, int image)
{
    LVITEM lvi;
    lvi.mask        = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM|LVIF_STATE;
    lvi.state       = 0;
    lvi.stateMask   = 0;
    lvi.iSubItem    = 0;
    lvi.pszText     = str;
    lvi.iImage      = image;
    lvi.iItem       = iItem;
    return ListView_InsertItem(hWndList, &lvi);
}

int insert_list_column_text(HWND hList, int col, int width, char *pszText)
{
    LV_COLUMN lvcol;
    lvcol.mask  = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvcol.fmt   = LVCFMT_LEFT;
    lvcol.cx    = 70;
    lvcol.pszText = pszText;
    ListView_InsertColumn(hList, col, &lvcol);
    ListView_SetColumnWidth(hList, col, width);
    return 1;
}

pio_socket get_frame_io_socket(HWND hWnd)
{
    pio_socket pi_socket = NULL;
    char buff[64];
    char *iostr;
    if (GetWindowText(hWnd, buff, 64)) {
        iostr = strstr(buff, "@");
        if (iostr != NULL) {
            iostr++;
            pi_socket = (pio_socket)atol(iostr);
        }
    }
    return pi_socket;
}

void SetWindowFont(HWND hWnd)
{
	HFONT hFont=CreateFont(13,0,0,0,0,0,0,0,GB2312_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"宋体'");
	SendMessage(hWnd,WM_SETFONT,(WPARAM)hFont,0);
}
