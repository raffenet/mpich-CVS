/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

//FIXME-Darius Just added the following dummy stuff for getting it to compile.
// copied from tcp_module_finalize.c

#include "newtcp_module_impl.h"
#include <sched.h>

//#define TRACE 
#define NEM_TCP_BUF_SIZE    MPID_NEM_OPT_HEAD_LEN
#define NEM_TCP_MASTER_RANK 0

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_newtcp_module_finalize ()
{
    int mpi_errno = MPI_SUCCESS;    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_ckpt_shutdown
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_newtcp_module_ckpt_shutdown ()
{
    int mpi_errno = MPI_SUCCESS;
    int ret;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

