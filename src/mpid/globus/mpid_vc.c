/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

/*
 * mpig_vcrt object definition
 */
typedef struct mpig_vcrt
{
    volatile int ref_count;

    /* number of entries in the table */
    int size;

    /* array of virtual connection references (pointers to VCs) */
    mpig_vc_t * vcr_table[1];
}
mpig_vcrt_t;


/*
 * MPID_VCRT_Create()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Create
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_VCRT_Create(int size, MPID_VCRT *vcrt_ptr)
{
    mpig_vcrt_t * vcrt;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_VCRT_CREATE);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCRT_CREATE);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    vcrt = MPIU_Malloc(sizeof(mpig_vcrt_t) + (size - 1) * sizeof(mpig_vc_t *));
    if (vcrt != NULL)
    {
	vcrt->ref_count = 1;
	vcrt->size = size;
	*vcrt_ptr = vcrt;
    }
    /* --BEGIN ERROR HANDLING-- */
    else
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
    }
    /* --END ERROR HANDLING-- */
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCRT_CREATE);
    return mpi_errno;
}


/*
 * MPID_VCRT_Add_ref()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Add_ref
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_VCRT_Add_ref(MPID_VCRT vcrt)
{
    MPIG_STATE_DECL(MPID_STATE_MPID_VCRT_ADD_REF);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCRT_ADD_REF);
    vcrt->ref_count += 1;
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCRT_ADD_REF);
    return MPI_SUCCESS;
}


/*
 * MPID_VCRT_Release()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Release
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_VCRT_Release(MPID_VCRT vcrt)
{
#if defined(XXX)
    int count;
#endif
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_VCRT_RELEASE);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCRT_RELEASE);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);

#if defined(XXX)    
    count = --vcrt->ref_count;
    if (count == 0)
    {
	int i, inuse;

	for (i = 0; i < vcrt->size; i++)
	{
	    mpig_vc_t * const vc = vcrt->vcr_table[i];
	    
	    mpig_vc_release_ref(vc, &count);
	    if (count == 0)
	    {
		/* If the VC is myself then skip the close message */
		if (vc->pg == mpig_process.my_pg && vc->pg_rank == mpig_process.my_pg_rank)
		{
                    mpig_pg_release_ref(vc->pg, &inuse);
                    if (inuse == 0)
                    {
                        mpig_pg_destroy(vc->pg);
                    }
		    continue;
		}
		
		if (vc->state != MPIG_VC_STATE_INACTIVE)
		{
		    MPIDI_CH3_Pkt_t upkt;
		    MPIDI_CH3_Pkt_close_t * close_pkt = &upkt.close;
		    MPID_Request * sreq;
			
		    MPIU_Assert(vc->state != MPIG_VC_STATE_LOCAL_CLOSE && vc->state != MPIG_VC_STATE_CLOSE_ACKED);
		    
		    MPIDI_Pkt_init(close_pkt, MPIDI_CH3_PKT_CLOSE);
		    close_pkt->ack = (vc->state == MPIG_VC_STATE_ACTIVE) ? FALSE : TRUE;
		    
		    /* MT: this is not thread safe */
		    MPIDI_Outstanding_close_ops += 1;
		    MPIG_DBG_PRINTF((30, FCNAME, "sending close(%s) to %d, ops = %d", close_pkt->ack ? "TRUE" : "FALSE",
				      i, MPIDI_Outstanding_close_ops));

		    /*
		     * A close packet acknowledging this close request could be received during iStartMsg, therefore the state
		     * must be changed before the close packet is sent.
		     */
		    if (vc->state == MPIG_VC_STATE_ACTIVE)
		    { 
			vc->state = MPIG_VC_STATE_LOCAL_CLOSE;
		    }
		    else /* if (vc->state == MPIG_VC_STATE_REMOTE_CLOSE) */
		    {
			vc->state = MPIG_VC_STATE_CLOSE_ACKED;
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
                    mpig_pg_release_ref(vc->pg, &inuse);
                    if (inuse == 0)
                    {
                        mpig_pg_destroy(vc->pg);
                    }

		    MPIG_DBG_PRINTF((30, FCNAME, "not sending a close to %d, vc in state %s", i,
				      mpig_vc_get_state_description(vc->state)));
		}
	    }
	}

	MPIU_Free(vcrt);
    }
#endif

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCRT_RELEASE);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}


/*
 * MPID_VCRT_Get_ptr()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Get_ptr
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_VCRT_Get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr)
{
    MPIG_STATE_DECL(MPID_STATE_MPID_VCRT_GET_PTR);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCRT_GET_PTR);
    *vc_pptr = vcrt->vcr_table;
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCRT_GET_PTR);
    return MPI_SUCCESS;
}


/*
 * MPID_VCR_Dup()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCR_Dup
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_VCR_Dup(MPID_VCR orig_vcr, MPID_VCR * new_vcr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_VCR_DUP);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCR_DUP);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    /* XXX: MT: this really need to be done atomically, but should the lock be here or in the calling routine? probably here
     * since the MPICH layer can call this routine. */
    {
	if (orig_vcr->ref_count == 0)
	{
	    mpig_pg_add_ref(orig_vcr->pg);
	}
	mpig_vc_add_ref(orig_vcr);
    }
    *new_vcr = orig_vcr;
    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCR_DUP);
    return mpi_errno;
}


/*
 * MPID_VCR_Get_lpid()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCR_Get_lpid
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_VCR_Get_lpid(MPID_VCR vcr, int * lpid_ptr)
{
    MPIG_STATE_DECL(MPID_STATE_MPID_VCR_GET_LPID);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCR_GET_LPID);
    *lpid_ptr = vcr->lpid;
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCR_GET_LPID);
    return MPI_SUCCESS;
}
