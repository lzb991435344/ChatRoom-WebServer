#include "module_manger.h"
#include "file.h"
#include "crc32.h"

extern unsigned long screen_exe_crc;

pmanger_module initialize_mod_dll_obj(char *dll)
{
    char dll_path[MAX_PATH];
    sprintf(dll_path,"%s%s", g_share_main->km_mod_path, dll);

    pmanger_module p_manger_module = malloc(sizeof(manger_module));
    memset(p_manger_module,0x0,sizeof(manger_module));

    strcpy(p_manger_module->modulename,dll);

    p_manger_module->moduledata = file_read(dll_path,&p_manger_module->modulelength);
    p_manger_module->c_module.crc32 = crc32(p_manger_module->moduledata,p_manger_module->modulelength);

    if (strstr(dll, ".dll") != NULL) {
        p_manger_module->c_module.majorcmd = 0;
    } else {
        if (strcmp(dll, "exe_screen.exe") == 0) {
            screen_exe_crc = p_manger_module->c_module.crc32;
        }
        p_manger_module->c_module.majorcmd = 1;
    }

    printf("%d\n", (int)p_manger_module->modulelength);
    return p_manger_module;
}

pmanger_module find_mod_dll_obj(unsigned long crc32)
{
    pmanger_module p_temp = p_manger_client_module;
    while(p_temp) {
        if (p_temp->c_module.crc32 == crc32)
            return p_temp;
        p_temp = (pmanger_module)p_temp->next;
    }
    return NULL;
}

int module_initialize()
{
    char find_mod_path[MAX_PATH];
    HANDLE hFind;
    WIN32_FIND_DATA fData;

    printf("enum mod list start\n");
    sprintf(find_mod_path, "%s*.*", g_share_main->km_mod_path);

    pmanger_module p_temp = p_manger_client_module;

    if ((hFind = FindFirstFile(find_mod_path,&fData)) == INVALID_HANDLE_VALUE)
        return 0;
    do {
        if ( (strstr(fData.cFileName, ".dll") != NULL) ||
                (strstr(fData.cFileName, ".exe") != NULL) ) {
            printf("mod:%s\n",fData.cFileName);
            if (p_temp == NULL){
                p_manger_client_module = initialize_mod_dll_obj(fData.cFileName);
                p_temp = p_manger_client_module;
            }else{
                p_temp->next = (pmanger_module)initialize_mod_dll_obj(fData.cFileName);
                p_temp = p_temp->next;
            }
        }
    }while (FindNextFile(hFind, &fData));
     printf("enum mod list stop\n");
    return 0;
}

int module_de_initialize()
{
    return 0;
}
