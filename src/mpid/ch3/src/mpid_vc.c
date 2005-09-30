/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"


/*
 * MPIDI_VCRT - virtual connection reference table
 *
 * handle - this element is not used, but exists so that we may use the MPIU_Object routines for reference counting
 *
 * ref_count - number of references to this table
 *
 * vcr_table - array of virtual connection references
 */
typedef struct MPIDI_VCRT
{
    int handle;
    volatile int ref_count;
    int size;
    MPIDI_VC_t * vcr_table[1];
}
MPIDI_VCRT_t;


#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCRT_Create(int size, MPID_VCRT *vcrt_ptr)
{
    MPIDI_VCRT_t * vcrt;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_CREATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_CREATE);
    
    vcrt = MPIU_Malloc(sizeof(MPIDI_VCRT_t) + (size - 1) * sizeof(MPIDI_VC_t *));
    if (vcrt != NULL)
    {
	MPIU_Object_set_ref(vcrt, 1);
	vcrt->size = size;
	*vcrt_ptr = vcrt;
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
    }
    /* --END ERROR HANDLING-- */
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_CREATE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Add_ref
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCRT_Add_ref(MPID_VCRT vcrt)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_ADD_REF);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_ADD_REF);
    MPIU_Object_add_ref(vcrt);
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_ADD_REF);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Release
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCRT_Release(MPID_VCRT vcrt)
{
    int count;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_RELEASE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_RELEASE);
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_Object_release_ref(vcrt, &count);
    if (count == 0)
    {
	int i, inuse;

	for (i = 0; i < vcrt->size; i++)
	{
	    MPIDI_VC_t * const vc = vcrt->vcr_table[i];
	    
	    MPIU_Object_release_ref(vc, &count);
	    if (count == 0)
	    {
		/* If the VC is myself then skip the close message */
		if (vc->pg == MPIDI_Process.my_pg && vc->pg_rank == MPIDI_Process.my_pg_rank)
		{
                    MPIDI_PG_Release_ref(vc->pg, &inuse);
                    if (inuse == 0)
                    {
                        MPIDI_PG_Destroy(vc->pg);
                    }
		    continue;
		}
		
		if (vc->state != MPIDI_VC_STATE_INACTIVE)
		{
		    MPIDI_CH3_Pkt_t upkt;
		    MPIDI_CH3_Pkt_close_t * close_pkt = &upkt.close;
		    MPID_Request * sreq;

		    /*
		    if (vc->state == MPIDI_VC_STATE_LOCAL_CLOSE || vc->state == MPIDI_VC_STATE_CLOSE_ACKED)
		    {
			printf("[%s%d]Assertion failed\n", MPIU_DBG_parent_str, MPIR_Process.comm_world->rank);
			MPIU_DBG_PrintVC(vc);
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_RELEASE);
			return mpi_errno;
		    }
		    */
		    MPIU_Assert(vc->state != MPIDI_VC_STATE_LOCAL_CLOSE && vc->state != MPIDI_VC_STATE_CLOSE_ACKED);
		    
		    MPIDI_Pkt_init(close_pkt, MPIDI_CH3_PKT_CLOSE);
		    close_pkt->ack = (vc->state == MPIDI_VC_STATE_ACTIVE) ? FALSE : TRUE;
		    
		    /* MT: this is not thread safe */
		    MPIDI_Outstanding_close_ops += 1;
		    MPIDI_DBG_PRINTF((30, FCNAME, "sending close(%s) to %d, ops = %d", close_pkt->ack ? "TRUE" : "FALSE",
				      i, MPIDI_Outstanding_close_ops));

		    /*
		     * A close packet acknowledging this close request could be received during iStartMsg, therefore the state
		     * must be changed before the close packet is sent.
		     */
		    if (vc->state == MPIDI_VC_STATE_ACTIVE)
		    {
			MPIU_DBG_PrintVCState2(vc, MPIDI_VC_STATE_LOCAL_CLOSE);
			vc->state = MPIDI_VC_STATE_LOCAL_CLOSE;
		    }
		    else /* if (vc->state == MPIDI_VC_STATE_REMOTE_CLOSE) */
		    {
			MPIU_DBG_PrintVCState2(vc, MPIDI_VC_STATE_CLOSE_ACKED);
			vc->state = MPIDI_VC_STATE_CLOSE_ACKED;
		    }
		    
		    mpi_errno = MPIDI_CH3_iStartMsg(vc, close_pkt, sizeof(*close_pkt), &sreq);
		    /* --BEGIN ERROR HANDLING-- */
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**ch3|send_close_ack", 0);
			continue;
		    }
		    /* --END ERROR HANDLING-- */
		    
		    if (sreq != NULL)
		    {
			MPID_Request_release(sreq);
		    }
		}
		else
		{
                    MPIDI_PG_Release_ref(vc->pg, &inuse);
                    if (inuse == 0)
                    {
                        MPIDI_PG_Destroy(vc->pg);
                    }

		    MPIDI_DBG_PRINTF((30, FCNAME, "not sending a close to %d, vc in state %s", i,
				      MPIDI_VC_Get_state_description(vc->state)));
		}
	    }
	}

	MPIU_Free(vcrt);
    }
    
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_RELEASE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Get_ptr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCRT_Get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_GET_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_GET_PTR);
    *vc_pptr = vcrt->vcr_table;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_GET_PTR);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_VCR_Dup
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCR_Dup(MPID_VCR orig_vcr, MPID_VCR * new_vcr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCR_DUP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCR_DUP);
    MPIU_Object_add_ref(orig_vcr);
    *new_vcr = orig_vcr;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCR_DUP);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_VCR_Get_lpid
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCR_Get_lpid(MPID_VCR vcr, int * lpid_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCR_GET_LPID);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCR_GET_LPID);
    *lpid_ptr = vcr->lpid;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCR_GET_LPID);
    return MPI_SUCCESS;
}

/* 
 * The following routines convert to/from the global pids, which are 
 * represented as pairs of ints (process group id, rank in that process group)
 */

int MPID_GPID_GetAllInComm( MPID_Comm *comm_ptr, int local_size, 
			    int local_gpids[], int *singlePG )
{
    int i;
    int *gpid = local_gpids;
    int lastPGID = -1, pgid;
    MPID_VCR vc;
    
    *singlePG = 1;
    for (i=0; i<comm_ptr->local_size; i++) {
	vc = comm_ptr->vcr[i];

	/* Get the process group id as an int */
	MPIDI_PG_IdToNum( vc->pg, &pgid );

	*gpid++ = pgid;
	if (lastPGID != pgid) { 
	    if (lastPGID != -1)
		*singlePG = 0;
	    lastPGID = pgid;
	}
	*gpid++ = vc->pg_rank;
	if (vc->pg_rank != vc->lpid) {
	    return 1;
/*	    printf( "Unexpected results %d != %d\n",
	    vc->pg_rank, vc->lpid ); */
	}
    }
    
    return 0;
}

/* 
 * The following is a very simple code for looping through 
 * the GPIDs
 */
int MPID_GPID_ToLpidArray( int size, int gpid[], int lpid[] )
{
    int i;
    int pgid;
    MPIDI_PG_t *pg = 0;

    for (i=0; i<size; i++) {
	MPIDI_PG_Iterate_reset();
	do {
	    MPIDI_PG_Get_next( &pg );
	    if (!pg) {
		/* Internal error.  This gpid is unknown on this process */
		lpid[i] = -1;
		return MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					     "MPID_GPID_ToLpidArray", __LINE__,
					     MPI_ERR_INTERN, "**unknowngpid",
					     "**unknowngpid %d %d", 
					     gpid[0], gpid[1] );
	    }
	    MPIDI_PG_IdToNum( pg, &pgid );
	    if (pgid == gpid[0]) {
		/* found the process group.  gpid[1] is the rank in 
		   this process group */
		lpid[i] = pg->vct[gpid[1]].lpid;
		/* printf( "lpid[%d] = %d for gpid = (%d)%d\n", i, lpid[i], 
		   gpid[0], gpid[1] ); */
		break;
	    }
	} while (1);
	gpid += 2;
    }
    return 0;
}

int MPID_VCR_CommFromLpids( MPID_Comm *newcomm_ptr, 
			    int size, const int lpids[] )
{
    MPID_Comm *commworld_ptr;
    int i;

    commworld_ptr = MPIR_Process.comm_world;
    /* Setup the communicator's vc table: remote group */
    MPID_VCRT_Create( size, &newcomm_ptr->vcrt );
    MPID_VCRT_Get_ptr( newcomm_ptr->vcrt, &newcomm_ptr->vcr );
    for (i=0; i<size; i++) {
	MPIDI_VC_t *vc = 0;

	/* For rank i in the new communicator, find the corresponding
	   rank in the comm world (FIXME FOR MPI2) */
	/* printf( "[%d] Remote rank %d has lpid %d\n", 
	   MPIR_Process.comm_world->rank, i, lpids[i] ); */
	if (lpids[i] < commworld_ptr->remote_size) {
	    vc = commworld_ptr->vcr[lpids[i]];
	}
	else {
	    /* We must find the corresponding vcr for a given lpid */	
	    /* For now, this means iterating through the process groups */
	    MPIDI_PG_t *pg = 0;
	    int j;

	    MPIDI_PG_Iterate_reset();
	    do {
		MPIDI_PG_Get_next( &pg );
		if (!pg) {
		    return MPIR_Err_create_code( MPI_SUCCESS, 
				     MPIR_ERR_RECOVERABLE,
				     "MPID_VCR_CommFromLpids", __LINE__,
				     MPI_ERR_INTERN, "**intern", 0 );
		}
		for (j=0; j<pg->size; j++) {
		    if (pg->vct[j].lpid == lpids[i]) {
			vc = &pg->vct[j];
			/* printf( "found vc %x for lpid = %d in another pg\n", 
			   (int)vc, lpids[i] ); */
			break;
		    }
		}
	    } while (!vc);
	}

	/* printf( "about to dup vc %x for lpid = %d in another pg\n", 
	   (int)vc, lpids[i] ); */
	MPID_VCR_Dup( vc, &newcomm_ptr->vcr[i] );
    }
    return 0;
}
