/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "socketimpl.h"

int socket_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_POST_READ);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_POST_READ);

    MPIU_dbg_printf("socket_post_read\n");
    socket_car_enqueue(vc_ptr, car_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_POST_READ);
    return MPI_SUCCESS;
}

int socket_post_read_pkt(MPIDI_VC *vc_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_POST_READ_PKT);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_POST_READ_PKT);

    MPIU_dbg_printf("*** socket_post_read_pkt - sock %d, num_bytes %d ***\n", sock_getid(vc_ptr->data.socket.sock), sizeof(MPID_Packet));
#ifdef MPICH_DEV_BUILD
    if (!vc_ptr->data.socket.state & SOCKET_CONNECTED)
    {
	err_printf("Error: socket_post_read_pkt cannot change to reading_header state until the vc is connected.\n");
    }
#endif
    /* possibly insert state to indicate a header is being read? */
    SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_READING_HEADER);
    SOCKET_CLR_BIT(vc_ptr->data.socket.state, SOCKET_READING_DATA);
    MPIU_dbg_printf("socket state: 0x%x\n", vc_ptr->data.socket.state);
    sock_post_read(vc_ptr->data.socket.sock, &vc_ptr->pkt_car.msg_header.pkt, sizeof(MPID_Packet), NULL);

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_POST_READ_PKT);
    return MPI_SUCCESS;
}

int socket_handle_read_ack(MPIDI_VC *vc_ptr, int num_read)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_READ_ACK);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_READ_ACK);

    MPIU_dbg_printf("socket_handle_read_ack(%d) - %d bytes\n", sock_getid(vc_ptr->data.socket.sock), num_read);

    SOCKET_CLR_BIT(vc_ptr->data.socket.connect_state, SOCKET_READING_ACK);
    SOCKET_CLR_BIT(vc_ptr->data.socket.state, SOCKET_CONNECTING);

    switch (vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_in)
    {
    case SOCKET_ACCEPT_CONNECTION:
	SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_CONNECTED);
	MPIU_dbg_printf("socket_handle_read_ack - %d = accept\n", sock_getid(vc_ptr->data.socket.sock));
	socket_post_read_pkt(vc_ptr);
	socket_write_aggressive(vc_ptr);
	break;
    case SOCKET_REJECT_CONNECTION:
	MPIU_dbg_printf("socket_handle_read_ack - %d = reject\n", sock_getid(vc_ptr->data.socket.sock));
	break;
    default:
	MPIU_dbg_printf("socket_handle_read_ack - %d = unknown ack #%d\n", sock_getid(vc_ptr->data.socket.sock), (int)vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_in);
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_ACK);
    return MPI_SUCCESS;
}

int socket_handle_read_connect_pkt(MPIDI_VC *temp_vc_ptr, int num_read)
{
    int error;
    MPIDI_VC *vc_ptr;
    sock_t sock;
    int remote_rank;

    MPIU_dbg_printf("socket_handle_read_connect_pkt(%d) - %d bytes of %d\n", sock_getid(temp_vc_ptr->data.socket.sock), num_read, sizeof(MPID_Connect_pkt));

    /* resolve the context to a virtual connection */
    vc_ptr = mm_vc_from_context(
	temp_vc_ptr->pkt_car.msg_header.pkt.u.connect.context, 
	temp_vc_ptr->pkt_car.msg_header.pkt.u.connect.rank);

    remote_rank = temp_vc_ptr->pkt_car.msg_header.pkt.u.connect.rank;
    sock = temp_vc_ptr->data.socket.sock;

    mm_vc_free(temp_vc_ptr);
    temp_vc_ptr = NULL;

    MPID_Thread_lock(vc_ptr->lock);
    
    if ((error = sock_set_user_ptr(sock, vc_ptr)) != SOCK_SUCCESS)
    {
	socket_print_sock_error(error, "socket_handle_accept: sock_set_user_ptr failed.");
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_ACCEPT);
	return -1;
    }

    /* Copy this setting from the temporary vc to the real vc */
    SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_ACCEPTING);
    /* This is unnecessary because the flag was set in the temporary vc and not this one.
     * The clr_bit shouldn't do anything but it is here for completeness. */
    SOCKET_CLR_BIT(vc_ptr->data.socket.connect_state, SOCKET_READING_CONNECT_PKT);

    if (vc_ptr->data.socket.state & SOCKET_CONNECTING)
    {
	/* Head to head connections made.
	   Keep the connection from the higher rank and close the lower rank socket.
	 */
	if (remote_rank > MPIR_Process.comm_world->rank)
	{
	    /* close the socket */
	    MPIU_dbg_printf("closing old socket: %d\n", sock_getid(vc_ptr->data.socket.sock));
	    sock_post_close(vc_ptr->data.socket.sock);
	    /* save the new connection */
	    vc_ptr->data.socket.sock = sock;
	    /* send an accept ack */
	    SOCKET_SET_BIT(vc_ptr->data.socket.connect_state, SOCKET_WRITING_ACK);
	    MPIU_dbg_printf("sock_post_write(%d:accept ack)\n", sock_getid(sock));
	    vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_out = SOCKET_ACCEPT_CONNECTION;
	    sock_post_write(sock, (void*)&vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_out, 1, NULL);
	}
	else
	{
	    /* send a reject acknowledgement */
	    SOCKET_SET_BIT(vc_ptr->data.socket.connect_state, SOCKET_WRITING_ACK);
	    MPIU_dbg_printf("sock_post_write(%d:reject ack)\n", sock_getid(sock));
	    vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_out = SOCKET_REJECT_CONNECTION;
	    sock_post_write(sock, (void*)&vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_out, 1, NULL);
	    /* close the new socket and keep the old */
	    sock_post_close(sock);
	}
    }
    else
    {
	SOCKET_SET_BIT(vc_ptr->data.socket.connect_state, SOCKET_WRITING_ACK);
	MPIU_dbg_printf("sock_post_write(%d:accept ack)\n", sock_getid(sock));
	vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_out = SOCKET_ACCEPT_CONNECTION;
	sock_post_write(sock, (void*)&vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_out, 1, NULL);
	vc_ptr->data.socket.sock = sock;
    }
    
    MPID_Thread_unlock(vc_ptr->lock);

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_ACCEPT);
    return MPI_SUCCESS;
}

int socket_handle_read(MPIDI_VC *vc_ptr, int num_bytes)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_READ);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_READ);

    if (vc_ptr->data.socket.state & SOCKET_CONNECT_MASK)
    {
	MPIU_dbg_printf("socket_handle_read(%d) - %d bytes\n", sock_getid(vc_ptr->data.socket.sock), num_bytes);

	if (vc_ptr->data.socket.connect_state & SOCKET_READING_ACK)
	{
	    socket_handle_read_ack(vc_ptr, num_bytes);
	    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ);
	    return MPI_SUCCESS;
	}
	else if (vc_ptr->data.socket.connect_state & SOCKET_READING_CONNECT_PKT)
	{
	    socket_handle_read_connect_pkt(vc_ptr, num_bytes);
	    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ);
	    return MPI_SUCCESS;
	}
	else
	{
	    err_printf("socket_read_connecting: unknown ack char #%d received in read function.\n", (int)vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_in);
	}

	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ);
	return MPI_SUCCESS;
    }

    if (vc_ptr->data.socket.state & SOCKET_READING_HEADER)
    {
	MPIU_dbg_printf("socket_handle_read(%d) received header - %d bytes\n", sock_getid(vc_ptr->data.socket.sock), num_bytes);
	mm_cq_enqueue(&vc_ptr->pkt_car);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ);
	return MPI_SUCCESS;
    }

    if (vc_ptr->data.socket.state & SOCKET_READING_DATA)
    {
	MPIU_dbg_printf("socket_handle_read(%d) received data - %d bytes\n", sock_getid(vc_ptr->data.socket.sock), num_bytes);
	socket_handle_read_data(vc_ptr, num_bytes);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ);
	return MPI_SUCCESS;
    }

    dbg_printf("socket_handle_read: sock %d read finished with a unknown socket state %d\n", sock_getid(vc_ptr->data.socket.sock), vc_ptr->data.socket.state);
    exit(-1);

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ);
    return -1;
}
