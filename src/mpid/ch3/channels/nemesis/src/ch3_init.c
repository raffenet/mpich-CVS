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


void *MPIDI_CH3_packet_buffer;
int MPIDI_CH3I_my_rank;

#undef USE_PRE_INIT
#ifdef USE_PRE_INIT
static int called_pre_init = 0;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Pre_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Pre_init (int *setvals, int *has_parent, int *rank, int *size)
{
    int mpi_errno = MPI_SUCCESS;
    int appnum;
    int pmi_errno;
    int nem_errno;
    int null_argc = 0;
    char **null_argv = 0;
    
    nem_errno = MPID_nem_init (null_argc, null_argv, rank, size);
    if (nem_errno != 0)
    {
	return MPI_ERR_INTERN;
    }

    pmi_errno = PMI_Get_appnum (&appnum);
    if (pmi_errno != PMI_SUCCESS)
    {
	MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**pmi_get_appnum", "**pmi_get_appnum %d", pmi_errno);
    }

    /* Note that if pmi is not availble, the value of MPI_APPNUM is 
       not set */
    if (appnum != -1)
    {
	MPIR_Process.attrs.appnum = appnum;
    }


    MPIDI_CH3I_my_rank = *rank;
    
    *has_parent = 0;
    *setvals = 1;
    
    called_pre_init = 1;

 fn_fail:
    return mpi_errno;
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t *pg_p, int pg_rank)
{
    int i;
    int nem_errno;
    int mpi_errno;
    
    __GM_COMPILE_TIME_ASSERT (sizeof(MPIDI_CH3_Pkt_t) == MPID_NEM__MPICH2_HEADER_LEN);

#if USE_PRE_INIT
    if (!called_pre_init)
    {
	mpi_errno =  MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**intern", 0 );
	return mpi_errno;
    }
#endif
    
    nem_errno = MPID_nem_init (pg_rank, pg_p);
    if (nem_errno != 0)
    {
	return MPI_ERR_INTERN;
    }
    
    MPIDI_CH3I_my_rank = pg_rank;
    
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

    for (i = 0; i < pg_p->size; i++)
    {
	MPIDI_VC_t *vc;
	MPIDI_PG_Get_vcr (pg_p, i, &vc);
	MPIDI_CH3_VC_Init (vc);
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

/* Perform the channel-specific vc initialization */
int MPIDI_CH3_VC_Init( MPIDI_VC_t *vc )
{
    int ret = 0;
    
    vc->ch.recv_active = NULL;
    vc->state = MPIDI_VC_STATE_ACTIVE;

    return MPID_nem_vc_init (vc);
}

int MPIDI_CH3_Connect_to_root (const char *port_name, MPIDI_VC_t **new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    MPIDI_VC_t * vc;
    MPIU_CHKPMEM_DECL (1);
    
    MPIU_CHKPMEM_MALLOC (vc, MPIDI_VC_t *, sizeof(MPIDI_VC_t), mpi_errno, "vc");
    /* FIXME - where does this vc get freed? */

    *new_vc = vc;

    MPIDI_VC_Init (vc, NULL, 0);
    MPIDI_CH3_VC_Init (vc);
    
    ret = MPID_nem_connect_to_root (port_name, vc->lpid);
    if (ret != 0)
    {
	mpi_errno =  MPIR_Err_create_code (MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**intern", 0);
        goto fn_fail;
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}
