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
    if ((comm_ptr == NULL) || (rank < 0) || (rank >= comm_ptr->size))
	return NULL;
#endif

    if (comm_ptr->vcrt == NULL)
    {
	mpi_errno = MPID_VCRT_Create(comm_ptr->size, &comm_ptr->vcrt);
	if (mpi_errno != MPI_SUCCESS)
	    return NULL;
	mpi_errno = MPID_VCRT_Get_ptr(comm_ptr->vcrt, &comm_ptr->vcr);
	if (mpi_errno != MPI_SUCCESS)
	    return NULL;
    }

    vc_ptr = comm_ptr->vcr[rank];
    if (vc_ptr == NULL)
    {
	comm_ptr->vcr[rank] = vc_ptr = mm_vc_alloc(MM_CONNECTOR_METHOD);
	vc_ptr->rank = rank;
	vc_ptr->pmi_kvsname = comm_ptr->mm.pmi_kvsname;
    }

    return vc_ptr;
}

/*@
   *mm_get_packer_vc - get the packer virtual connection

   Notes:
@*/
MPIDI_VC *mm_get_packer_vc()
{
    return mm_vc_alloc(MM_PACKER_METHOD);
}

/*@
   *mm_get_unpacker_vc - get the unpacker virtual connection

   Notes:
@*/
MPIDI_VC *mm_get_unpacker_vc()
{
    return mm_vc_alloc(MM_UNPACKER_METHOD);
}
