#ifndef MPIEXEC_H
#define MPIEXEC_H

#include "sock.h"
#include <stdio.h>

int mp_dbg_printf(char *str, ...);
int mp_err_printf(char *str, ...);
int mp_parse_command_args(int *argc, char **argv[]);
void mp_get_account_and_password(char *account, char *password);
int mp_console(char *host);

#endif
