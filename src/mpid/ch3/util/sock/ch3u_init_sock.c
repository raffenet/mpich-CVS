/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

/*  MPIDI_CH3U_Init_sock - does socket specific channel initialization
 *     publish_bc_p - if non-NULL, will be a pointer to the original position 
 *                    of the bc_val and should
 *                    do KVS Put/Commit/Barrier on business card before 
 *                    returning
 *     bc_key_p     - business card key buffer pointer.  
 *     bc_val_p     - business card value buffer pointer, updated to the next
 *                    available location or freed if published.
 *     val_max_sz_p - ptr to maximum value buffer size reduced by the number 
 *                    of characters written
 *                               
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Init_sock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Init_sock(int has_parent, MPIDI_PG_t *pg_p, int pg_rank,
			 char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int pg_size;
    int p;

    /*
     * Initialize the VCs associated with this process group (and thus 
     * MPI_COMM_WORLD)
     */

    /* FIXME: Get the size from the process group */
    pmi_errno = PMI_Get_size(&pg_size);
    if (pmi_errno != 0) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_get_size",
			     "**pmi_get_size %d", pmi_errno);
    }

    /* FIXME: This should probably be the same as MPIDI_VC_InitSock.  If
       not, why not? */
    for (p = 0; p < pg_size; p++)
    {
	pg_p->vct[p].ch.sendq_head = NULL;
	pg_p->vct[p].ch.sendq_tail = NULL;
	pg_p->vct[p].ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
	pg_p->vct[p].ch.sock = MPIDU_SOCK_INVALID_SOCK;
	pg_p->vct[p].ch.conn = NULL;
    }    

    mpi_errno = MPIDI_CH3U_Get_business_card_sock(bc_val_p, val_max_sz_p);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**init_buscard");
    }

 fn_exit:
    
    return mpi_errno;
    
 fn_fail:
    /* FIXME: This doesn't belong here, since the pg is not created in 
       this routine */
    /* --BEGIN ERROR HANDLING-- */
    if (pg_p != NULL) 
    {
	MPIDI_PG_Destroy(pg_p);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* This routine initializes Sock-specific elements of the VC */
int MPIDI_VC_InitSock( MPIDI_VC_t *vc ) 
{
    vc->ch.sock               = MPIDU_SOCK_INVALID_SOCK;
    vc->ch.conn               = NULL;
    return 0;
}
