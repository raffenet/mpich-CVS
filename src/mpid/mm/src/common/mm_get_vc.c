/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pmi.h"

/*@
   mm_get_vc - get the virtual connection pointer

   Parameters:
+  MPID_Comm *comm_ptr - communicator
-  int rank - rank

   Notes:
@*/
MPIDI_VC *mm_get_vc(MPID_Comm *comm_ptr, int rank)
{
    int mpi_errno;
    MPIDI_VC *vc_ptr;
    MM_Car *car_ptr;
    MPID_Request *request_ptr;

#ifdef MPICH_DEV_BUILD
    if ((comm_ptr == NULL) || (rank < 0) || (rank >= comm_ptr->remote_size))
	return NULL;
#endif

    if (comm_ptr->vcrt == NULL)
    {
	mpi_errno = MPID_VCRT_Create(comm_ptr->remote_size, &comm_ptr->vcrt);
	if (mpi_errno != MPI_SUCCESS)
	    return NULL;
	mpi_errno = MPID_VCRT_Get_ptr(comm_ptr->vcrt, &comm_ptr->vcr);
	if (mpi_errno != MPI_SUCCESS)
	    return NULL;
    }

    vc_ptr = comm_ptr->vcr[rank];
    if (vc_ptr == NULL)
    {
	comm_ptr->vcr[rank] = vc_ptr = mm_vc_connect_alloc(comm_ptr->mm.pmi_kvsname, rank);

	/* post a read for the first packet on this newly connected vc */
	request_ptr = mm_request_alloc();
	car_ptr = request_ptr->mm.rcar;

	car_ptr->type = MM_HEAD_CAR | MM_READ_CAR;
	car_ptr->src = rank;
	car_ptr->dest = rank;
	car_ptr->vc_ptr = vc_ptr;
	car_ptr->next_ptr = NULL;
	car_ptr->opnext_ptr = NULL;
	car_ptr->qnext_ptr = NULL;
	car_ptr->request_ptr = NULL;
	car_ptr->request_ptr = request_ptr;
	request_ptr->comm = comm_ptr;
	request_ptr->ref_count = 1;
	request_ptr->mm.buf_type = MM_VEC_BUFFER;
	request_ptr->mm.buf.vec.vec[0].MPID_VECTOR_BUF = (void*)&car_ptr->data.pkt;
	request_ptr->mm.buf.vec.vec[0].MPID_VECTOR_LEN = sizeof(MPID_Packet);
	request_ptr->mm.buf.vec.size = 1;
	request_ptr->mm.buf.vec.num_read = 0;
	request_ptr->mm.buf.vec.min_num_written = 0;

	vc_ptr->post_read(vc_ptr, car_ptr);
    }

    return vc_ptr;
}
