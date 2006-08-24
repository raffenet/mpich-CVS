/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

// myct: we need the NUM_STREAM def
#include "mpidi_ch3_pre.h"

static int MPIDI_CH3U_Init_sctp(int has_parent, MPIDI_PG_t *pg_p, int pg_rank,
                         char **publish_bc_p, char **bc_key_p, 
				char **bc_val_p, int *val_max_sz_p);


/*
 *  MPIDI_CH3_Init  - makes socket specific initializations.  Most of this 
 *                    functionality is in the MPIDI_CH3U_Init_sock upcall 
 *                    because the same tasks need to be done for the ssh 
 *                    (sock + shm) channel.  
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t * pg_p, int pg_rank )
{
    int mpi_errno = MPI_SUCCESS;
    char *publish_bc_orig = NULL;
    char *bc_key = NULL;
    char *bc_val = NULL;
    int val_max_remaining, key_max_sz;
    MPIDI_STATE_DECL(MPID_STATE_MPID_CH3_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CH3_INIT);

    mpi_errno = MPIDI_CH3I_Acceptq_init();
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIDI_CH3I_Progress_init(MPIDI_PG_Get_size(pg_p));
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Initialize the business card */
    mpi_errno = my_MPIDI_CH3I_BCInit( pg_rank, &publish_bc_orig, &bc_key, &bc_val,
				   &val_max_remaining );
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* Currently, this "upcall" is a static within this file but later this could
     *  go with all the others in the ch3/util directory.
     */
    /* initialize aspects specific to sctp  */
    mpi_errno = MPIDI_CH3U_Init_sctp(has_parent, pg_p, pg_rank,
				     &publish_bc_orig, &bc_key, &bc_val, 
				     &val_max_remaining);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CH3_INIT);
    return mpi_errno;
 fn_fail:
    if (bc_key != NULL) {
        MPIU_Free(bc_key);
    }
    if (publish_bc_orig != NULL) {
        MPIU_Free(publish_bc_orig);
    }           
    goto fn_exit;
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
int MPIDI_CH3_VC_Init( MPIDI_VC_t *vc ) {

    int i = 0;
    MPIDI_CH3I_SCTP_Stream_t* str_ptr = NULL;

    for(i = 0; i < MPICH_SCTP_NUM_REQS_ACTIVE_TO_INIT; i++) {
	str_ptr = &(vc->ch.stream_table[i]);
	STREAM_INIT(str_ptr);
    }

    vc->ch.fd = MPIDI_CH3I_onetomany_fd;
    vc->ch.pkt = NULL;
    vc->ch.pg_id = NULL;

    sendq_total = 0;

    vc->ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;

    vc->ch.send_init_count = 0;
   
    return 0;
}

/* Select the routine that uses sockets to connect two communicators
   using a socket */
int MPIDI_CH3_Connect_to_root(const char * port_name, 
			      MPIDI_VC_t ** new_vc)
{
    return MPI_SUCCESS;
}

/* This "upcall" is (temporarily?) a static here, and may be in ch3/util later. */
static int MPIDI_CH3U_Init_sctp(int has_parent, MPIDI_PG_t *pg_p, int pg_rank,
                         char **publish_bc_p, char **bc_key_p, 
			 char **bc_val_p, int *val_max_sz_p) {
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int pg_size;
    int p, i;
    MPIDI_CH3I_SCTP_Stream_t* str_ptr;

    /*
     * Initialize the VCs associated with this process group (and thus MPI_COMM_WORLD)
     */
    pmi_errno = PMI_Get_size(&pg_size);
    if (pmi_errno != 0) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_get_size",
			     "**pmi_get_size %d", pmi_errno);
    }

    /* FIXME: This should probably be the same as MPIDI_VC_InitSock.  If
       not, why not? */
    sendq_total = 0;    
    for (p = 0; p < pg_size; p++) {
	MPIDI_CH3_VC_Init(&(pg_p->vct[p]));
    }    

    /* This function actually will work for SCTP if we use the MPIDI_CH3I_listener_port */
    mpi_errno = my_MPIDI_CH3U_Get_business_card_sock(bc_val_p, val_max_sz_p);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**init_buscard");
    }

    MPIDI_PG_GetConnKVSname(&pg_p->ch.kvs_name);

    /* might still have something to add (e.g. ssm channel) so don't publish */
    if (publish_bc_p != NULL)
    {
	// myct: PMI success is 0
	pmi_errno = PMI_KVS_Put(pg_p->ch.kvs_name, *bc_key_p, *publish_bc_p);
	if (pmi_errno != 0) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_kvs_put",
				 "**pmi_kvs_put %d", pmi_errno);
	}
	pmi_errno = PMI_KVS_Commit(pg_p->ch.kvs_name);

	if (pmi_errno != 0) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_kvs_commit",
				 "**pmi_kvs_commit %d", pmi_errno);
	}

	pmi_errno = PMI_Barrier();
	if (pmi_errno != 0) {
	    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_barrier",
				 "**pmi_barrier %d", pmi_errno);
	}
    }

 fn_exit:
    
    return mpi_errno;
    
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (pg_p != NULL)
    {
	/* MPIDI_CH3I_PG_Destroy(), which is called by MPIDI_PG_Destroy(), frees pg->ch.kvs_name */
	MPIDI_PG_Destroy(pg_p);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

int MPIDI_CH3_PG_Init( MPIDI_PG_t *pg )
{
    return MPI_SUCCESS;
}

const char * MPIDI_CH3_VC_GetStateString( int state )
{

    return "urmom";
}
