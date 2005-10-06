/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

/* FIXME: What does this function do?  Who calls it?  Can we assume that
   it is called only dynamic process operations (specifically spawn) 
   are supported?  Do we need the concept of a port? For example,
   could a channel that supported only shared memory call this (it doesn't
   look like it right now, so this could go into util/sock, perhaps?
   
   It might make more sense to have this function provided as a function 
   pointer as part of the channel init setup, particularly since this
   function appears to access channel-specific storage (MPIDI_CH3_Process) */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Get_parent_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Get_parent_port(char ** parent_port)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef MPIDI_CH3_IMPLEMENTS_GET_PARENT_PORT
    char val[MPIDI_MAX_KVS_VALUE_LEN];

    if (MPIDI_Process.parent_port_name == NULL)
    {
	mpi_errno = MPIDI_KVS_Get(MPIDI_Process.my_pg->ch.kvs_name, "PARENT_ROOT_PORT_NAME", val);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_POP(mpi_errno);
	}

	MPIDI_Process.parent_port_name = MPIU_Strdup(val);
	if (MPIDI_Process.parent_port_name == NULL) {
	    MPIU_ERR_POP(mpi_errno);
	}
    }

    *parent_port = MPIDI_Process.parent_port_name;
#endif

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
