/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef TCPIMPL_H
#define TCPIMPL_H

#include "mm_tcp.h"

typedef struct TCP_PerProcess {
    MPID_Thread_lock_t lock;
                   int listener;
		   int port;
		  char host[100];
               bfd_set readset;
               bfd_set writeset;
} TCP_PerProcess;

extern TCP_PerProcess TCP_Process;

#endif
