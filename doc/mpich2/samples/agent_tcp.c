/* Waiting on incoming messages */

/* Blocking (in GetNextPacket) has three values:
   BLOCKING: Wait until a packet is available
   NONBLOCKING: Test for any communication and return null if nothing 
   available
   EXPECTING: Data is expected ``soon''.  This can be implemented as 
   NONBLOCKING, but may use a short wait (e.g., the round-trip time) 
   instead. 
 */
enum { BLOCKING=0, NOTBLOCKING=1, EXPECTING=2 } MPID_Blocking_t;

#ifdef AGENT_IN_THREAD
/* If the communication agent runs in a separate thread, run it
   forever */
while (1) 
#elif defined(MULTITHREADED)
/* this is the polling and multithreaded process case */
if (MPID_THREAD_LEVEL == MPID_THREAD_MULTIPLE) pthread_lock( agent_mutex );
#endif
{
    MPID_Hid_general_packet_t *packet;
    /* returns the next successfully read complete packet */
    /* This isn't quite correct - we don't want to require reading all
       of a MPID_Hid_data packet, and maybe not all of an MPID_Hid_eager
       packet.  We assume that this returns a pointer to a 
       complete packet header, upto but not necessarily including any
       data portion.  ? How do we know how much of any data portion 
       is valid? */
    packet = GetNextPacket( blocking, &source );
    if (!packet) break;  /* or pthread_unlock/return in the polling case */
    switch (packet->type) {
        case MPID_Hid_eager:
        MPID_Hid_eager_t *lpacket = (MPID_Hid_eager_t *)packet;
        comm_ptr = &global_comm[lpacket->context_id];
        request_ptr = MPID_Request_recv_FOA( lpacket->tag, l
                                             packet->sender_rank, 
                                             comm_ptr, &found );
        if (found) {
            /* Matching receive exists */
            /* Check for truncation */
            if (lpacket->msg_bytes > request_ptr->msg_bytes) {
		request_ptr->status.MPI_ERROR = 
		    MPID_Err_create_code( MPI_ERR_TRUNCATE, 0, 0, 
					  lpacket->msg_bytes, 
					  request_ptr->msg_bytes );
	    }
            MPID_Segment_unpack_init( source, &request_ptr->stream );
            MPID_Stream_irecv_tcp( source, lpacket->msg_bytes, 
				   &request_ptr->stream, 
				   NULL, &request_ptr->complete );
	    /* In the truncate error case, do we add a discard to the 
	       input stream (see the MSG_READY case)?  Also, we 
	       need to decrement the refcounts on the datatype and comm_ptr */
        }
        else {
	    # This is an eager message with no matching receive.
            if (lpacket->mode == MPID_MSG_READY) {
                /* indicate error */
		struct iovec vector[1];
		MPID_Hid_control_t *tpacket = 
		    (MPID_Hid_control_t *)&request->packet;
		vector[0].iov_base = tpacket;
		vector[0].iov_len  = sizeof(MPID_Hid_control);
		tpacket->kind = MPID_Hid_control;
		tpacket->len  = sizeof(MPID_Hid_control_t);
		tpacket->control_type = MPID_Control_error;
		tpacket->nargs        = 5;
		tpacket->iparm[0]     = MPIi_ERR_OTHER_RSEND;
	        tpacket->iparm[1]     = lpacket->tag;
		tpacket->iparm[2]     = lpacket->source_rank;
		tpacket->iparm[3]     = lpacket->dest_rank;
		tpacket->iparm[4]     = lpacket->context_id;
		MPID_Rhcv_tcp( lpacket->sender_rank, comm_ptr, 
			       MPID_Hid_control, vector, 1, NULL );
		/* Discard bytes */
		MPID_Stream_discard_tcp( source, lpacket->msg_bytes );
		
            } else {
		/* Copy byte data */
		void *ptr = MPID_EagerAlloc( lpacket->sender_rank, comm, 
					     lpacket->msg_bytes );
		/* EagerAlloc updates flow control */

		request_ptr->status.count  = lpacket->msg_bytes;
		/* Copy data to a temporary buffer.  Note that the data may
		   not have arrived yet.  This is similar to the found case
		   above */
		request_ptr->stream.datatype = MPI_BYTE;
		request_ptr->eager.temp      = ptr;
		MPID_Segment_unpack_init( source, &request_ptr->segment );
		MPID_Stream_irecv_tcp( source, lpacket->msg_bytes, 
				       &request_ptr->stream, 
				       MPID_Eager_complete_func, &request_ptr );
	    }
        }
        break;

        case MPID_Hid_request_to_send:
        MPID_Hid_request_to_send_t *lpacket = 
	    (MPID_Hid_request_to_send_t *)packet;
        comm_ptr = &global_comm[lpacket->context_id];
        request_ptr = MPID_Request_recv_FOA( lpacket->tag, 
                                             lpacket->sender_rank, 
                                             comm_ptr, &found );
        if (found) {
             /* Same as Irecv case for found and not eager */
        }
        else {
             request_ptr->rndv.sender_id = lpacket->request_id;
             request_ptr->msg_bytes      = lpacket->msg_bytes;
             MPID_MemWrite_ordered( request_ptr->busy, 0 );
        }
        break;

        case MPID_Hid_ok_to_send:
        MPID_Hid_ok_to_send_t *lpacket = (MPID_Hid_ok_to_send *)packet;

        request_ptr = &global_request[lpacket->request_id];
        if (request_ptr->buf.datatype->is_contig) {
            struct iovec vector[2];
	    MPID_Hid_data *tpacket;
            vector[0].iov_base = tpacket = &request_ptr->packet; 
            vector[0].iov_len  = sizeof(MPID_Hid_data_t);
	    tpacket->recv_id = lpacket->recv_id;
	    tpacket->kind    = MPID_Hid_data;
	    tpacket->len     = sizeof(MPID_Hid_data_t);
	    tpacket->msg_bytes = request_ptr->msg_bytes;
            vector[1].iov_base = request_ptr->buf.buffer;
            vector[1].iov_len  = request_ptr->msg_bytes;
            MPID_Rhcv_tcp( request_ptr->recv_rank, request_ptr->comm_ptr, 
                           MPID_Hid_data, vector, 2, 
                           &request_ptr->complete );
        } else {
            /* Start a send stream.  */
	    MPID_Segment_init_pack( &request_ptr->segment );
            MPID_Stream_isend_tcp( source, 
                                   &request_ptr->segment, &request_ptr->stream,
                                   NULL, &request_ptr->complete );
        }
        break;

        case MPID_Hid_data:
        /* Expected data */
        MPID_Hid_data_t *lpacket = (MPID_Hid_data_t *)packet;
        request_ptr = &global_request[lpacket->request_id];

        /* If the destination is contiguous, we can read directly into it.
           Otherwise, we need an intermediate buffer to unpack from */
        /* The data may not be available yet.  Define a stream to read it */
        MPID_Stream_irecv_tcp( source, &lpacket->data, lpacket->msg_bytes, 
			       &request->segment, &request->stream, 
			       NULL, &request->complete );
        break;

        case MPID_Hid_control:
	    /* not done yet */
        break;    
	...
    }
#if defined(MULTITHREADED)
if (MPID_THREAD_LEVEL == MPID_THREAD_MULTIPLE) pthread_unlock( agent_mutex );
#endif
}


/* This function is called by MPID_Stream_irecv_tcp when an eager stream
   is completely received */
static void MPID_Eager_complete_func( void *ptr )
{
    MPID_Request *req = (MPID_Request *)ptr;
    
    request_ptr->eager.ptr = request_ptr->eager.temp;
    MPID_MemWrite_ordered( request_ptr->busy, 0 );
    if (request_ptr->thread_id) {
	/* Signal a thread that is waiting for this request to 
	   complete */ 
	pthread_cont_signal( request_ptr->thread_id, 
			     request_ptr->cond ); 
    }
}
