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
static int MPIDI_CH3I_Connect_to_root_sctp(const char * port_name, 
                                          MPIDI_VC_t ** new_vc);


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

    vc->ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;

    vc->ch.send_init_count = 0;
   
    return 0;
}

/* Select the routine that uses sctp to connect two communicators
   using a socket */
int MPIDI_CH3_Connect_to_root(const char * port_name, 
			      MPIDI_VC_t ** new_vc)
{
    return MPIDI_CH3I_Connect_to_root_sctp( port_name, new_vc );
}

static int MPIDI_CH3I_Connect_to_root_sctp(const char * port_name, 
			      MPIDI_VC_t ** new_vc)
{
    int tmp_fd, no_nagle, port, real_port, mpi_errno = MPI_SUCCESS;
    struct sctp_event_subscribe evnts;
    MPIU_Size_t nb;
    char host_description[MAX_HOST_DESCRIPTION_LEN];
    int port_name_tag;
    int bufsz = 233016;
    MPIDU_Sock_ifaddr_t ifaddr;
    struct sockaddr_in to_address;
    int hasIfaddr = 0;

    MPIDU_Sctp_event_t event2;
    
    union MPIDI_CH3_Pkt conn_acc_pkt, *pkt;
    int iov_cnt = 2;
    MPID_IOV conn_acc_iov[iov_cnt];
    char bizcard[MPI_MAX_PORT_NAME];            
    MPID_IOV* iovp = conn_acc_iov;


    /* prepare a new socket for connect/accept */
    no_nagle = 1;
    port = 0;
    bzero(&evnts, sizeof(evnts));
    evnts.sctp_data_io_event=1;

    if(sctp_open_dgm_socket2(MPICH_SCTP_NUM_STREAMS,
			     0, 5, port, no_nagle,
			     &bufsz, &evnts, &tmp_fd,
                             &real_port) == -1) {
        /* FIXME define error code */
        goto fn_fail;
    }
    
    
    MPIU_DBG_MSG_S(CH3_CONNECT,VERBOSE,"Connect to root with portstring %s",
		   port_name );

    /* obtain the sockaddr_in from the business card */
    mpi_errno = my_MPIDU_Sock_get_conninfo_from_bc( port_name, host_description,
						 sizeof(host_description),
						 &port, &ifaddr, &hasIfaddr );
    if (mpi_errno) {
	MPIU_ERR_POP(mpi_errno);
    }
    giveMeSockAddr(ifaddr.ifaddr, port, &to_address);

    /* handle the port_name_tag */
    mpi_errno = MPIDI_GetTagFromPort(port_name, &port_name_tag);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_port_name_tag");
    }
    MPIU_DBG_MSG_D(CH3_CONNECT,VERBOSE,"port tag %d",port_name_tag);


    /* store port temporarily so bizcard func works. put new tmp port in to pass to
     *  the accept side.
     */
    port = MPIDI_CH3I_listener_port;
    MPIDI_CH3I_listener_port = real_port;
    mpi_errno = MPIDI_CH3I_Get_business_card(bizcard, MPI_MAX_PORT_NAME);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS) {
        /* FIXME define error code */
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    MPIDI_CH3I_listener_port = port; /* restore */
    
    /* get the conn_acc_pkt ready */
    MPIDI_Pkt_init(&conn_acc_pkt, MPIDI_CH3I_PKT_SC_CONN_ACCEPT); 
    conn_acc_pkt.sc_conn_accept.bizcard_len = (int) strlen(bizcard) + 1; 
    conn_acc_pkt.sc_conn_accept.port_name_tag = port_name_tag;
    conn_acc_pkt.sc_conn_accept.ack = 0; /* this is NOT an ACK */

    /* get the iov ready */
    conn_acc_iov[0].MPID_IOV_BUF = (void *) &conn_acc_pkt;
    conn_acc_iov[0].MPID_IOV_LEN = sizeof(conn_acc_pkt);
    conn_acc_iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) bizcard;
    conn_acc_iov[1].MPID_IOV_LEN = conn_acc_pkt.sc_conn_accept.bizcard_len;

    /* write on control stream now. send on the existing onetomany socket. the 
     *  other side won't add this to the hash because of the pkt's type.  we
     *  don't want the onetomany sockets to get caught up in the VC close
     *  protocol when the tmp VC is killed (so we don't want to mix the new
     *  tmp fd with the "standard" ones).
     */ 
    for(;;) {
        
        mpi_errno = MPIDU_Sctp_writev_fd(MPIDI_CH3I_onetomany_fd, &to_address, iovp,
                             iov_cnt, MPICH_SCTP_CTL_STREAM, 0, &nb );
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS) {
            goto fn_fail;
        }
        /* --END ERROR HANDLING-- */

        /* deliberately avoid nb < 0 */
        if(nb > 0 && adjust_iov(&iovp, &iov_cnt, nb)) { /* static in ch3_progress.c */
            /* done sending */
            break;
        }
    }

    /* for dynamic procs, we only progress on one fd... */
    MPIU_Assert(MPIDI_CH3I_dynamic_tmp_vc == NULL);
    MPIDI_CH3I_dynamic_tmp_fd = tmp_fd;

    /* block on tmp_fd until conn_acc_pkt is ACK'd */
    while(MPIDI_CH3I_dynamic_tmp_vc == NULL) {
        
        mpi_errno = MPIDU_Sctp_wait(tmp_fd, MPIDU_SCTP_INFINITE_TIME,
                                    &event2);
        if (mpi_errno != MPI_SUCCESS)
        {
            MPIU_Assert(MPIR_ERR_GET_CLASS(mpi_errno) != MPIDU_SOCK_ERR_TIMEOUT);
            MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**progress_sock_wait");
            goto fn_fail;
        }

        /* inside handle_sctp_event, it changes
         *  the read to an accept and calls itself recursively which ultimately sets
         *  the value of MPIDI_CH3I_dynamic_tmp_vc.
         */
        mpi_errno = MPIDI_CH3I_Progress_handle_sctp_event(&event2);
        if (mpi_errno != MPI_SUCCESS) {
            MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
                                "**ch3|sock|handle_sock_event");
        }
    }
    

    *new_vc = MPIDI_CH3I_dynamic_tmp_vc;


 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;    
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

    sendq_total = 0;    
    for (p = 0; p < pg_size; p++) {
	MPIDI_CH3_VC_Init(&(pg_p->vct[p]));  /* TODO check success */
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
