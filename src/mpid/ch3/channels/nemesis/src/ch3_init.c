/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "mpidi_ch3_impl.h"

/* borrowed this from Myricom */
#define __GM_COMPILE_TIME_ASSERT(a) do {                                \
  char (*__GM_COMPILE_TIME_ASSERT_var)[(a) ? 1 : -1] = 0;               \
  (void) __GM_COMPILE_TIME_ASSERT_var; /* prevent unused var warning */ \
} while (0)


void          *MPIDI_CH3_packet_buffer;
int            MPIDI_CH3I_my_rank;

static int called_pre_init = 0;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Pre_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Pre_init (int *setvals, int *has_parent, int *rank, int *size)
{
    int nem_errno;
    int null_argc = 0;
    char **null_argv = 0;
    
    nem_errno = MPID_nem_init (null_argc, null_argv, rank, size);
    if (nem_errno != 0)
    {
	return MPI_ERR_INTERN;
    }

    MPIDI_CH3I_my_rank = *rank;
    
    *has_parent = 0;
    *setvals = 1;
    
    called_pre_init = 1;

    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t *pg_p, int pg_rank)
{
    int mpi_errno;
    int p;

    __GM_COMPILE_TIME_ASSERT (sizeof(MPIDI_CH3_Pkt_t) == MPID_NEM__MPICH2_HEADER_LEN);

    if (!called_pre_init)
    {
	mpi_errno =  MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**intern", 0 );
	return mpi_errno;
    }
    
    /*
     * Initialize Progress Engine 
     */
    mpi_errno = MPIDI_CH3I_Progress_init();
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code (mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_progress", 0);
	return mpi_errno;
	/* --END ERROR HANDLING-- */
    }    

    /* initialize VCs */
    for (p = 0; p < pg_p->size; p++)
    {
	struct MPIDI_VC *vcp;

	MPIDI_PG_Get_vcr (pg_p, p, &vcp);
	
	vcp->ch.pg_rank = p;
	vcp->ch.recv_active = NULL;
	vcp->state = MPIDI_VC_STATE_ACTIVE;
    }

    return MPI_SUCCESS;
}

/* This function simply tells the CH3 device to use the defaults for the 
   MPI Port functions */
int MPIDI_CH3_PortFnsInit( MPIDI_PortFns *portFns ) 
{
    MPIU_UNREFERENCED_ARG(portFns);
    return 0;
}

/* This function simply tells the CH3 device to use the defaults for the 
   MPI-2 RMA functions */
int MPIDI_CH3_RMAFnsInit( MPIDI_RMAFns *a ) 
{ 
    return 0;
}
