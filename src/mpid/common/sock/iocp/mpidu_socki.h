/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <winsock2.h>
#include <windows.h>

#include "mpiimpl.h"

#define inline __inline

typedef HANDLE MPIDU_SOCK_NATIVE_FD;
typedef HANDLE MPIDU_Sock_set_t;
typedef struct sock_state_t * MPIDU_Sock_t;
typedef size_t MPIDU_Sock_size_t;
