/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

int tcp_cq_test()
{
    int nready = 0;
    struct timeval tv;
    MPIDI_VC *vc_iter;
    bfd_set readset, writeset;
    
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    readset = TCP_Process.readset;
    writeset = TCP_Process.writeset;

    nready = bselect(TCP_Process.max_bfd, &readset, &writeset, NULL, &tv);

    if (nready == 0)
	return MPI_SUCCESS;

    vc_iter = TCP_Process.read_list;
    while (vc_iter)
    {
	if (BFD_ISSET(vc_iter->data.tcp.bfd, &readset))
	{
	    /* read data */
	    nready--;
	}
	if (nready == 0)
	    return MPI_SUCCESS;
	vc_iter = vc_iter->read_next_ptr;
    }

    vc_iter = TCP_Process.write_list;
    while (vc_iter)
    {
	if (BFD_ISSET(vc_iter->data.tcp.bfd, &writeset))
	{
	    /* write data */
	    nready--;
	}
	if (nready == 0)
	    return MPI_SUCCESS;
	vc_iter = vc_iter->write_next_ptr;
    }

    if (nready == 0)
	return MPI_SUCCESS;

    if (BFD_ISSET(TCP_Process.listener, &readset))
    {
	nready--;
	/* accept new connection */
	/* add the new connection to the read set
	TCP_Process.max_bfd = BFD_MAX(bfd, TCP_Process.max_bfd);
	BFD_SET(bfd, &TCP_Process.readset);
	When the vc is established, add it to the active read list.
	vc_ptr->read_next_ptr = TCP_Process.read_list;
	TCP_Process.read_list = vc_ptr;
	*/
    }

    if (nready)
    {
	err_printf("Error: %d sockets still signalled after traversing read_list, write_list and listener.");
	/* return some error */
    }

    return MPI_SUCCESS;
}
