#ifndef EVENT_VER_CLIENT_H_INCLUDED
#define EVENT_VER_CLIENT_H_INCLUDED

#include "km_head.h"
#include "..\share\event.h"

int initialize_km_main_event();

int kmc_main_version_h(pio_socket i_socket, char *buffer, int length);
int kmc_main_version(pio_socket i_socket, char *buffer, int length);
int kmc_main_module(pio_socket i_socket, char *buffer, int length);
int kmc_main_info(pio_socket i_socket, char *buffer, int length);
int kmc_main_error(pio_socket i_socket, char *buffer, int length);
int kmc_main_module_ok(pio_socket i_socket, char *buffer, int length);

#endif // EVENT_VER_CLIENT_H_INCLUDED
