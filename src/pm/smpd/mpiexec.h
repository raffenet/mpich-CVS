#ifndef MPIEXEC_H
#define MPIEXEC_H

#include "sock.h"
#include <stdio.h>

int mp_err_printf(char *str, ...);
int mp_parse_command_args(int argc, char *argv[]);
void mp_get_account_and_password(char *account, char *password);
int mp_connect_to_smpd(char *host, char *session_type, sock_set_t *set_ptr, sock_t *sock_ptr);
int mp_close_connection(sock_set_t set, sock_t sock);

#endif
