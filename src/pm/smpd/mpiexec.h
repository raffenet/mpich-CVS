/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIEXEC_H
#define MPIEXEC_H

#include "smpd.h"
#include "sock.h"
#include <stdio.h>

#define MP_MAX_HOST_LENGTH    100
#define MP_MAX_EXE_LENGTH    1024
#define MP_MAX_ENV_LENGTH    1024
#define MP_MAX_NAME_LENGTH    256
#define MP_MAX_VALUE_LENGTH  1024
#define MP_MAX_FILENAME      1024
#define MP_MAX_STDOUT_LENGTH 1024

typedef struct mp_host_node_t
{
    int id, parent;
    char host[MP_MAX_HOST_LENGTH];
    /*char ip_str[MP_MAX_HOST_LENGTH];*/
    int nproc;
    struct mp_host_node_t *next;
} mp_host_node_t;

typedef struct mp_launch_node_t
{
    char exe[MP_MAX_EXE_LENGTH];
    char *env, env_data[MP_MAX_ENV_LENGTH];
    int host_id;
    long iproc;
    struct mp_launch_node_t *next;
} mp_launch_node_t;

typedef struct mp_env_node
{
    char name[MP_MAX_NAME_LENGTH];
    char value[MP_MAX_VALUE_LENGTH];
    struct mp_env_node *next;
} mp_env_node;

typedef struct mp_map_drive_node
{
    char drive;
    char share[MP_MAX_EXE_LENGTH];
    struct mp_map_drive_node *next;
} mp_map_drive_node;

typedef struct mp_process_t
{
    int do_console;
    char console_host[SMPD_MAX_HOST_LENGTH];
    mp_host_node_t *host_list;
    mp_launch_node_t *launch_list;
    int credentials_prompt;
    int do_multi_color_output;
    int no_mpi;
    int output_exit_codes;
    int local_root;
    int use_iproot;
    int use_process_session;
    int shutdown_console;
    int nproc;
    int verbose;
} mp_process_t;

#ifdef HAVE_WINDOWS_H
extern HANDLE g_hCloseStdinThreadEvent;
extern HANDLE g_hStdinThread;
#endif

extern int g_bUseProcessSession;
extern mp_process_t mp_process;

#if 0
int mp_dbg_printf(char *str, ...);
int mp_err_printf(char *str, ...);
int mp_enter_fn(char *fcname);
int mp_exit_fn(char *fcname);
#else
#define mp_dbg_printf smpd_dbg_printf
#define mp_err_printf smpd_err_printf
#define mp_enter_fn   smpd_enter_fn
#define mp_exit_fn    smpd_exit_fn
#endif
void mp_get_account_and_password(char *account, char *password);
int mp_console(char *host);
int mp_create_command_from_stdin(char *str, smpd_command_t **cmd_pptr);
int mp_connect_tree(mp_host_node_t *node);
int mp_connect_next(int *parent_ptr, int *id_ptr);
int handle_read(smpd_context_t *context, int num_read, int error, smpd_context_t *session_context);
int handle_written(smpd_context_t *context, int num_written, int error);
int mp_parse_command_args(int *argcp, char **argvp[]);
void mp_print_options(void);

#endif
