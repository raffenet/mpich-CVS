err = MPI_SUCCESS;
/* Error checking */
...
request_ptr = MPID_Request_recv_FOA( tag, rank, comm_ptr, &found );
/* FOA ensures that the request is not busy when it is returned if found, and
   is busy if not found.  FOA is really ``Find and remove or allocate and 
   insert'' */
if (found) {
    if (request_ptr->eager.ptr) {
        /* This is the eager case; all data is available */
        /* This requires FOA to wait while receiving data (busy).  The
           assumption is that eager messages will be short relatively
           short */
        /* Check for Truncation */
        msg_size = request_ptr->status.count;
        if (count * datatype->size < msg_size ) {
            err = MPID_Err_create_code( MPI_ERR_TRUNCATE, 0, 0, 
					msg_size, count*datatype->size );
            request_ptr->status.count = count * datatype->size;
        }
        if (datatype->is_contig) 
            memcpy( buffer, request_ptr->eager.ptr, 
		    request_ptr->status.count );
        else {
            int location = 0;
            MPID_Unpack( buffer, count, datatype, 
			 request_ptr->eager.ptr, &location, 
			 request_ptr->status.count, request_ptr->msg_format );
        }
        /* release eager.ptr (includes flow control) */
        MPID_EagerFree( rank, comm_ptr, request_ptr->eager.ptr, msg_size );
        MPID_MemWrite_ordered( request_ptr->complete, 1 );
    }
    else {
         MPID_Hid_ok_to_send_t *packet;
         struct iovec vector[1];

	 /* First, set up to unpack the data when it starts to arrive */
	 request_ptr->buf.ptr      = buffer;
	 request_ptr->buf.count    = count;
	 request_ptr->buf.datatype = datatype;
	 MPID_Datatype_incr(datatype,1);
	 MPID_Comm_incr(comm_ptr,1);

	 /* Now, send the request to deliver the data */
         packet = (MPID_Hid_ok_to_send_t *)&request_ptr.packet;
         vector[0].iov_base = &packet;
         vector[0].iov_len  = sizeof(MPID_Hid_ok_to_send_t);
         packet->recv_request_id = request_ptr->self;
         packet->send_request_id = request_ptr->rndv.sender_id;
         packet->kind            = MPID_Hid_ok_to_send;
         packet->len             = sizeof(MPID_Hid_ok_to_send_t);
         MPID_Rhcv_tcp( request_ptr->status.MPI_SOURCE, comm_ptr,
                        MPID_Hid_ok_to_send, vector, 1, NULL );
    }
}
else {
    /* Unmatched.  Attach datatype/buffer and wait for communication agent */
    /* The tag, communicator, and source rank are already saved (by FOA) */ 
    request_ptr->buf.ptr      = buffer;
    request_ptr->buf.count    = count;
    request_ptr->buf.datatype = datatype;
    request_ptr->buf.comm_ptr = comm_ptr;
    MPID_Datatype_incr(datatype,1);
    MPID_Comm_incr(comm_ptr,1);
    MPID_MemWrite_ordered(request_ptr->busy,0);
}
*request = request_ptr->self;
return err;
