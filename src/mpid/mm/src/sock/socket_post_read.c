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
    socket_car_enqueue_read(vc_ptr, car_ptr);

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_POST_READ);
    return MPI_SUCCESS;
}

int socket_post_read_pkt(MPIDI_VC *vc_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_POST_READ_PKT);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_POST_READ_PKT);

    MPIU_dbg_printf("socket_post_read_pkt(%d), %d bytes\n", sock_getid(vc_ptr->data.socket.sock), sizeof(MPID_Packet));

#ifdef MPICH_DEV_BUILD
    if (!(vc_ptr->data.socket.state & SOCKET_CONNECTED))
    {
	err_printf("Error: socket_post_read_pkt cannot change to reading_header state until the vc is connected.\n");
    }
#endif
    /* possibly insert state to indicate a header is being read? */
    SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_READING_HEADER);
    SOCKET_CLR_BIT(vc_ptr->data.socket.state, SOCKET_READING_DATA);
    /*MPIU_dbg_printf("socket state: 0x%x\n", vc_ptr->data.socket.state);*/
    sock_post_read(vc_ptr->data.socket.sock, &vc_ptr->pkt_car.msg_header.pkt, sizeof(MPID_Packet), NULL);

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_POST_READ_PKT);
    return MPI_SUCCESS;
}

int socket_handle_read_ack(MPIDI_VC *vc_ptr, int num_read)
{
    MPIDI_VC *temp_vc_ptr;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_READ_ACK);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_READ_ACK);

    /*MPIU_dbg_printf("socket_handle_read_ack(%d) - %d byte\n", sock_getid(vc_ptr->data.socket.sock), num_read);*/

    SOCKET_CLR_BIT(vc_ptr->data.socket.connect_state, SOCKET_READING_ACK);
    /*SOCKET_CLR_BIT(vc_ptr->data.socket.state, SOCKET_CONNECTING);*/

    switch (vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_in)
    {
    case SOCKET_ACCEPT_CONNECTION:
	SOCKET_CLR_BIT(vc_ptr->data.socket.state, SOCKET_CONNECTING);
	/*SOCKET_CLR_BIT(vc_ptr->data.socket.state, SOCKET_ACCEPTING);*/
	SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_CONNECTED);
	/*SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_READ_CONNECTED);*/
	MPIU_dbg_printf("socket_handle_read_ack(%d) = accept\n", sock_getid(vc_ptr->data.socket.sock));
	socket_post_read_pkt(vc_ptr);
	socket_write_aggressive(vc_ptr);
	break;
    case SOCKET_REJECT_CONNECTION:
	/*MPIU_dbg_printf("socket_handle_read_ack(%d) = reject\n", sock_getid(vc_ptr->data.socket.sock));*/
	if (vc_ptr->data.socket.freeme)
	{
	    MPIU_dbg_printf("socket_handle_read_ack(%d) = reject, freeme\n", sock_getid(vc_ptr->data.socket.sock));
	    /* close the socket in VC */
	    sock_set_user_ptr(vc_ptr->data.socket.sock, NULL);
	    MPIU_dbg_printf("sock_post_close(%d)\n", sock_getid(vc_ptr->data.socket.sock));
	    sock_post_close(vc_ptr->data.socket.sock);
	    /* copy the temporary VC into the real VC */
	    temp_vc_ptr = vc_ptr->data.socket.connect_vc_ptr;
	    vc_ptr->data.socket.sock = temp_vc_ptr->data.socket.sock;
	    vc_ptr->data.socket.connect_vc_ptr = NULL;
	    sock_set_user_ptr(vc_ptr->data.socket.sock, vc_ptr);
	    SOCKET_CLR_BIT(vc_ptr->data.socket.state, SOCKET_CONNECTING);
	    SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_CONNECTED);
	    /* post a read of the first header packet */
	    socket_post_read_pkt(vc_ptr);
	    socket_write_aggressive(vc_ptr);
	    /* free the temporary VC */
	    mm_vc_free(temp_vc_ptr);
	}
	else
	{
	    MPIU_dbg_printf("socket_handle_read_ack(%d) = reject, !freeme\n", sock_getid(vc_ptr->data.socket.sock));
	    vc_ptr->data.socket.freeme = TRUE;
	}
	break;
    default:
	MPIU_dbg_printf("socket_handle_read_ack(%d) = unknown ack #%d\n", 
	    sock_getid(vc_ptr->data.socket.sock), (int)vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_in);
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ_ACK);
    return MPI_SUCCESS;
}

/*
void swap_vc_data(MPIDI_VC *v1, MPIDI_VC *v2)
{
    MPIDI_VC v;
    v.data = v1->data;
    v1->data = v2->data;
    v2->data = v.data;
}
*/

int socket_handle_read_connect_pkt(MPIDI_VC *temp_vc_ptr, int num_read)
{
    /*int error;*/
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

    MPID_Thread_lock(vc_ptr->lock);
    
    /*
    if ((error = sock_set_user_ptr(sock, vc_ptr)) != SOCK_SUCCESS)
    {
	socket_print_sock_error(error, "socket_handle_accept: sock_set_user_ptr failed.");
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_ACCEPT);
	return -1;
    }
    */

    /* Copy this setting from the temporary vc to the real vc */
    /*SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_ACCEPTING);*/
    SOCKET_CLR_BIT(temp_vc_ptr->data.socket.connect_state, SOCKET_READING_CONNECT_PKT);

    if (vc_ptr->data.socket.state & SOCKET_CONNECTING || vc_ptr->data.socket.freeme)
    {
	/* Head to head connections made.
	   Keep the connection from the higher rank and close the lower rank socket.
	 */
	vc_ptr->data.socket.connect_vc_ptr = temp_vc_ptr;
	temp_vc_ptr->data.socket.connect_vc_ptr = vc_ptr;

	if (remote_rank > MPIR_Process.comm_world->rank)
	{
	    /*
	    swap_vc_data(vc_ptr, temp_vc_ptr);
	    sock_set_user_ptr(vc_ptr->data.socket.sock, vc_ptr);
	    sock_set_user_ptr(temp_vc_ptr->data.socket.sock, temp_vc_ptr);
	    */
	    /* close the socket */
	    /*SOCKET_CLR_BIT(vc_ptr->data.socket.state, SOCKET_ACCEPTING);
	    SOCKET_CLR_BIT(vc_ptr->data.socket.state, SOCKET_CONNECTING);*/
	    /*
	    MPIU_dbg_printf("sock_post_close(%d) - closing connect socket, rank %d < remote_rank %d\n", 
		sock_getid(vc_ptr->data.socket.sock), MPIR_Process.comm_world->rank, remote_rank);
	    sock_post_close(vc_ptr->data.socket.sock);
	    */

	    /* make the temp VC the real VC */
	    /* This won't work because there already are aliases to vc_ptr stored in the request structures */
	    /**(vc_ptr->vc_pptr) = temp_vc_ptr;*/

	    /* send an accept ack */
	    /*
	    SOCKET_SET_BIT(vc_ptr->data.socket.connect_state, SOCKET_WRITING_ACK);
	    MPIU_dbg_printf("sock_post_write(%d:accept ack)\n", sock_getid(sock));
	    vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_out = SOCKET_ACCEPT_CONNECTION;
	    sock_post_write(sock, (void*)&vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_out, 1, NULL);
	    */
	    temp_vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_out = SOCKET_ACCEPT_CONNECTION;
	    MPIU_dbg_printf("sock_post_write(%d:accept ack)\n", sock_getid(sock));
	}
	else
	{
	    /*sock_set_user_ptr(sock, NULL);*/
	    /* send a reject acknowledgement */
	    /*SOCKET_CLR_BIT(vc_ptr->data.socket.state, SOCKET_ACCEPTING);*/
	    /*
	    SOCKET_SET_BIT(temp_vc_ptr->data.socket.connect_state, SOCKET_WRITING_ACK);
	    MPIU_dbg_printf("sock_post_write(%d:reject ack)\n", sock_getid(sock));
	    temp_vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_out = SOCKET_REJECT_CONNECTION;
	    sock_post_write(sock, (void*)&temp_vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_out, 1, NULL);
	    */
	    /* close the new socket and keep the old */
	    /*
	    MPIU_dbg_printf("sock_post_close(%d) - closing accept socket, rank %d > remote_rank %d\n", 
		sock_getid(sock), MPIR_Process.comm_world->rank, remote_rank);
	    sock_post_close(sock);
	    */
	    temp_vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_out = SOCKET_REJECT_CONNECTION;
	    MPIU_dbg_printf("sock_post_write(%d:reject ack)\n", sock_getid(sock));
	}
	SOCKET_SET_BIT(temp_vc_ptr->data.socket.connect_state, SOCKET_WRITING_ACK);
	sock_post_write(sock, (void*)&temp_vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_out, 1, NULL);
    }
    else
    {
	MPIU_dbg_printf("accepted non-headtohead connection, posting accept ack.\n");
	/* free the temporary VC */
	mm_vc_free(temp_vc_ptr);
	/* move the sock from the temp VC to the real VC */
	vc_ptr->data.socket.sock = sock;
	vc_ptr->data.socket.connect_vc_ptr = NULL;
	sock_set_user_ptr(sock, vc_ptr);
	/* post an accept acknowledgement */
	SOCKET_SET_BIT(vc_ptr->data.socket.connect_state, SOCKET_WRITING_ACK);
	MPIU_dbg_printf("sock_post_write(%d:accept ack)\n", sock_getid(sock));
	vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_out = SOCKET_ACCEPT_CONNECTION;
	sock_post_write(sock, (void*)&vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_out, 1, NULL);
    }
    
    MPID_Thread_unlock(vc_ptr->lock);

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_ACCEPT);
    return MPI_SUCCESS;
}

int socket_handle_read(MPIDI_VC *vc_ptr, int num_bytes)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_READ);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_READ);

    if (vc_ptr == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ);
	return MPI_SUCCESS;
    }

    if (!(vc_ptr->data.socket.state & SOCKET_CONNECTED))
    {
    /*if (vc_ptr->data.socket.state & SOCKET_CONNECT_MASK)*/
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
	    err_printf("socket_handle_read: unexpectedly read %d bytes while in connecting state.\n", num_bytes);
	}

	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_READ);
	return MPI_SUCCESS;
    }
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
