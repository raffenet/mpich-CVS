/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIEXEC_H
#define MPIEXEC_H

#include "smpd.h"
#include "mpidu_sock.h"
#include <stdio.h>

int mp_parse_command_args(int *argcp, char **argvp[]);
void mp_print_options(void);

#endif
