request_ptr = &global_request[*request];

if (request_ptr->complete) {
    if (status) *status = request_ptr->status;
    err = request_ptr->status.MPI_ERROR;
    /* Is there other rundown code, such as: */
    if (request_ptr->buf.datatype) {
	MPID_Datatype_incr( request_ptr->buf.datatype, -1 );
    }
    if (request_ptr->kind & MPID_REQ_REQULAR_MASK) {
	MPID_Request_free( request_ptr );
	*request = MPI_REQUEST_NULL;  
    }
    return err;
}
while (1) {
#ifdef SINGLE_THREADED_DEVICE
    MPID_Device_wait( BLOCKING );
#else
    pthread_lock( &request_ptr->mutex );
#endif
    if (request_ptr->complete) {
#ifdef SINGLE_THREADED_DEVICE
	pthread_unlock( &request_ptr->mutex );
#endif
	/* as above */
    }
#ifndef SINGLE_THREADED_DEVICE
    else {
	request_ptr->thread_id = local->my_thread_id;  
	pthread_cond_wait( &request_ptr->mutex, &request_ptr->cond );
	/* Saving the thread_id of the waiting thread allows the 
	   communication agent to use pthread_cond_signal instead
	   of pthread_cond_broadcast */
    }
#endif
}

