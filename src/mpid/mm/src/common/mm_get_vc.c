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
	/* allocate and connect a virtual connection */
	comm_ptr->vcr[rank] = vc_ptr = mm_vc_connect_alloc(comm_ptr, rank);
	/* post a read on the newly connected vc for a packet */
	mm_post_read_pkt(vc_ptr);
    }

    return vc_ptr;
}

/*@
   mm_vc_get - get the virtual connection pointer

   Parameters:
+  int rank - rank

   Notes:
@*/
MPIDI_VC *mm_vc_get(int rank)
{
    int mpi_errno;
    MPIDI_VC *vc_ptr;
    MPID_Comm *comm_ptr;

    comm_ptr = MPIR_Process.comm_world;

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
	comm_ptr->vcr[rank] = vc_ptr = mm_vc_alloc(MM_UNBOUND_METHOD);
    }

    return vc_ptr;
}
