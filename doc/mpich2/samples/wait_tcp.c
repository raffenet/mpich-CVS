while (1) {
#ifdef SINGLE_THREADED_DEVICE
    if (request_ptr->complete) {
         if (status) *status = *request_ptr->status;
         MPID_Request_free( request_ptr );
         *request = MPI_REQUEST_NULL;
    }
    else {
        MPID_Device_wait( BLOCKING );
        }
#else
    if (request_ptr->complete) {
	/* as above, no lock needed */
    } 
    else {
	pthread_lock( &request_ptr->mutex );
	if (request_ptr->complete) {
	    pthread_unlock( &request_ptr->mutex );
	    /* as above */
	}
	else {
	    request_ptr->thread_id = local->my_thread_id;  
	    pthread_cond_wait( &request_ptr->mutex, &request_ptr->cond );
	    /* Saving the thread_id of the waiting thread allows the 
	       communication agent to use pthread_cond_signal instead
	       of pthread_cond_broadcast */
	}
    }
#endif
} 
