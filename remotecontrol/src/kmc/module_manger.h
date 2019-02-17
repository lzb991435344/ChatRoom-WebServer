#ifndef CLIENT_MODULE_MANGER_H_INCLUDED
#define CLIENT_MODULE_MANGER_H_INCLUDED

#include "km_head.h"

pmanger_module initialize_mod_dll_obj(char *dll);
pmanger_module find_mod_dll_obj(unsigned long crc32);

int module_initialize();
int module_de_initialize();

#endif // CLIENT_MODULE_MANGER_H_INCLUDED
