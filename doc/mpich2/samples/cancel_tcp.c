/* Code executed on sender side */
/* Indicate that we are attempting to cancel this request.  Note that the
   request to cancel may fail */
request_ptr->state = MPID_CANCEL_IN_PROGRESS;
packet             = ?
packet->sender_id  = request_ptr->self;
vector[0].iov_base = packet;
vector[0].iov_len  = sizeof(MPID_Hid_cancel_t);
MPID_Rhcv_tcp( rank, comm_ptr, MPID_Hid_cancel, vector, 1, NULL );

/* Additional packet cases added to communication agent */
...
case MPID_Hid_cancel:
/* Try to find matching request by sender id in recv queue (meaning that it
   is unmatched) */
request_ptr = MPID_Request_recv_remove_by_id( packet->sender_id, source );
if (!request_ptr) {
  /* cancel fails */
  newpacket = ?
  newpacket->kind      = MPID_Hid_cancel_ack;
  newpacket->sender_id = packet->sender_id;
  newpacket->success   = 0;
} else {
  /* cancel succeeds */
  newpacket = ?
  newpacket->kind      = MPID_Hid_cancel_ack;
  newpacket->sender_id = packet->sender_id;
  newpacket->success   = 1;
}
vector[0].iov_base = newpacket;
vector[0].iov_len  = sizeof(MPID_Hid_cancel_ack_t);
MPID_Rhcv_tcp( source_rank, comm_ptr, MPID_Hid_cancel_ack, vector, 1, NULL );
break;
...
