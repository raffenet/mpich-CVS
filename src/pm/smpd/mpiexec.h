#ifndef MPIEXEC_H
#define MPIEXEC_H

#include "sock.h"
#include <stdio.h>

#define MP_MAX_HOST_LENGTH  100
#define MP_MAX_EXE_LENGTH  1024

typedef struct mp_host_node_t
{
    char host[MP_MAX_HOST_LENGTH];
    char ip_str[MP_MAX_HOST_LENGTH];
    int id;
    struct mp_host_node_t *next;
} mp_host_node_t;

typedef struct mp_launch_node_t
{
    char exe[MP_MAX_EXE_LENGTH];
    int host_id;
    long nproc;
    struct mp_launch_node_t *next;
} mp_launch_node_t;

#ifdef HAVE_WINDOWS_H
extern HANDLE g_hCloseStdinThreadEvent;
extern HANDLE g_hStdinThread;
#endif

extern int g_bUseProcessSession;

int mp_dbg_printf(char *str, ...);
int mp_err_printf(char *str, ...);
int mp_enter_fn(char *fcname);
int mp_exit_fn(char *fcname);
int mp_parse_command_args(int *argc, char **argv[]);
void mp_get_account_and_password(char *account, char *password);
int mp_console(char *host);

#endif
