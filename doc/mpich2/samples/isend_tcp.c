err = MPI_SUCCESS;
/* Error checking */
...
request_ptr = MPID_Request_send_FOA( tag, rank, comm_ptr, &found );
/* found always false for now */
/* not found */
if (count * datatype->size > eager_limit ||
    !MPID_Flow_limit( count * datatype->size , rank, comm_ptr )) {
    MPID_Hid_request_to_send_t *packet;
    struct iovec vector[1];
    /* rendezvous */

    /* save data fields */
    request_ptr->buf.ptr      = buffer;
    request_ptr->buf.count    = count;
    request_ptr->buf.datatype = datatype;
    MPID_Datatype_incr( datatype, 1 );
    MPID_Datatype_incr( comm_ptr, 1 );

    packet = (MPID_Hid_request_to_send_t *)&request_ptr->packet;
    packet->kind               = MPID_Hid_request_to_send;
    packet->len                = sizeof(MPID_Hid_request_to_send_t);
    packet->sender_id          = request_ptr->self;
    packet->msg_size           = count * datatype->size;
    packet->tag                = tag;
    packet->sender_rank        = sender_rank;
    packet->dest_rank          = rank;
    packet->context_id         = comm_ptr->context_id;
    vector[0].iov_base = packet;
    vector[0].iov_len  = sizeof(MPID_Hid_request_to_send_t);
    MPID_Rhcv_tcp( rank, comm_ptr, MPID_Hid_request_to_send, vector, 1, 
                   NULL );  /* No completion flag needed here */
}
else {
    /* Note that the flow limit test takes length, rank, and communicator.
       This ensures that the test is thread-safe 
       (it is really test and reserve on success) */
    MPID_Hid_eager_t *packet;
    struct iovec vector[2];

    packet = (MPID_Hid_eager_t *)&request_ptr->packet; 
    packet->sender_rank = sender_rank;
    packet->dest_rank   = rank;
    packet->context_id  = comm_ptr->context_id;
    packet->tag         = tag;
    packet->msg_size    = count * datatype->size;
    packet->sender_id   = request_ptr->self;

    vector[0].iov_base = packet;
    vector[0].iov_len  = sizeof(MPID_Hid_eager_t);
    if (datatype->is_contig) {
        vector[1].iov_base = buffer;
	vector[1].iov_len  = count * datatype->size;
	MPID_Rhcv_tcp( rank, comm_ptr, MPID_Hid_eager, vector, 2, 
		       &request_ptr->complete );
        }
    else if (count * datatype->size <= short_limit) {
	vector[1].iov_base = MPID_SendAlloc( count * datatype->size );
	vector[1].iov_len  = count * datatype->size;
	request_ptr->buf.ptr = vector[1].iov_base;
        MPID_Pack( buffer, count, datatype, request_ptr->buf.ptr
                   count * datatype->size, msg_format );
	MPID_Rhcv_tcp( rank, comm_ptr, MPID_Hid_eager, vector, 2, 
		       NULL );
        request_ptr->complete = 1;
    } else {
	/* This is the complicated case.  We want to send a stream of 
	   data */
	MPID_Stream_send_init( buffer, count, datatype, &request_ptr->stream );
	MPID_Stream_isend_tcp( rank, comm_ptr, MPID_Hid_eager, vector, 
			       &request_ptr->stream, &request_ptr->complete );
    }
}
*request = request_ptr->self;
return err;
