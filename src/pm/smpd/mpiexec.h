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

void mp_get_account_and_password(char *account, char *password);
int mp_console(char *host);
int mp_create_command_from_stdin(char *str, smpd_command_t **cmd_pptr);
int mp_connect_tree(smpd_host_node_t *node);
int mp_connect_next(int *parent_ptr, int *id_ptr);
int handle_read(smpd_context_t *context, int num_read, int error, smpd_context_t *session_context);
int handle_written(smpd_context_t *context, int num_written, int error);
int mp_parse_command_args(int *argcp, char **argvp[]);
void mp_print_options(void);

#endif
