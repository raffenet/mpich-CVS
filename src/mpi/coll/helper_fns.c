/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* prototype for fn used in MPIR_Localcopy */
/* implementation in src/mpi/datatype/typeutil.c */
extern void MPIR_Datatype_iscontig( MPI_Datatype , int * );


/* These functions are used in the implementation of collective
   operations. They are wrappers around MPID send/recv functions. They do
   sends/receives by setting the context offset to
   MPID_CONTEXT_INTRA_COLL or MPID_CONTEXT_INTER_COLL. */

int MPIC_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm)
{
    int mpi_errno, context_id;
    MPID_Request *request_ptr=NULL;
    MPID_Comm *comm_ptr=NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIC_SEND);

    MPID_MPI_PT2PT_FUNC_ENTER_FRONT(MPID_STATE_MPIC_SEND);

    MPID_Comm_get_ptr( comm, comm_ptr );
    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Send(buf, count, datatype, dest, tag, comm_ptr,
                          context_id, &request_ptr); 
    if (mpi_errno != MPI_SUCCESS)
    {
	MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPIC_SEND);
	return mpi_errno;
    }
    if (request_ptr) {
        MPIR_Wait(request_ptr);
        MPID_Request_release(request_ptr);
    }
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPIC_SEND);
    return mpi_errno;
}

int MPIC_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
	     MPI_Comm comm, MPI_Status *status)
{
    int mpi_errno, context_id;
    MPID_Request *request_ptr=NULL;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIC_RECV);

    MPID_MPI_PT2PT_FUNC_ENTER_BACK(MPID_STATE_MPIC_RECV);

    MPID_Comm_get_ptr( comm, comm_ptr );
    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Recv(buf, count, datatype, source, tag, comm_ptr,
                          context_id, status, &request_ptr); 
    if (mpi_errno != MPI_SUCCESS)
    {
	MPID_MPI_PT2PT_FUNC_EXIT_BACK(MPID_STATE_MPIC_RECV);
	return mpi_errno;
    }
    if (request_ptr) {
        MPIR_Wait(request_ptr);
        if (status != MPI_STATUS_IGNORE)
            *status = request_ptr->status;
        mpi_errno = request_ptr->status.MPI_ERROR;
        MPID_Request_release(request_ptr);
    }
    MPID_MPI_PT2PT_FUNC_EXIT_BACK(MPID_STATE_MPIC_RECV);
    return mpi_errno;
}

int MPIC_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  int dest, int sendtag, void *recvbuf, int recvcount,
                  MPI_Datatype recvtype, int source, int recvtag,
                  MPI_Comm comm, MPI_Status *status) 
{
    MPID_Request *recv_req_ptr=NULL, *send_req_ptr=NULL;
    int mpi_errno, context_id;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIC_SENDRECV);

    MPID_MPI_PT2PT_FUNC_ENTER_BOTH(MPID_STATE_MPIC_SENDRECV);

    MPID_Comm_get_ptr( comm, comm_ptr );
    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Irecv(recvbuf, recvcount, recvtype, source, recvtag,
                           comm_ptr, context_id, &recv_req_ptr);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPID_MPI_PT2PT_FUNC_EXIT_BOTH(MPID_STATE_MPIC_SENDRECV);
	return mpi_errno;
    }
    mpi_errno = MPID_Isend(sendbuf, sendcount, sendtype, dest, sendtag, 
                           comm_ptr, context_id, &send_req_ptr); 
    if (mpi_errno != MPI_SUCCESS)
    {
	MPID_MPI_PT2PT_FUNC_EXIT_BOTH(MPID_STATE_MPIC_SENDRECV);
	return mpi_errno;
    }

    MPIR_Wait(send_req_ptr); 
    MPID_Request_release(send_req_ptr);

    MPIR_Wait(recv_req_ptr);
    if (status != MPI_STATUS_IGNORE)
        *status = recv_req_ptr->status;
    mpi_errno = recv_req_ptr->status.MPI_ERROR;
    MPID_Request_release(recv_req_ptr);

    MPID_MPI_PT2PT_FUNC_EXIT_BOTH(MPID_STATE_MPIC_SENDRECV);
    return mpi_errno;
}


int MPIR_Localcopy(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype)
{
    int sendtype_iscontig, recvtype_iscontig, sendsize;
    int rank, mpi_errno = MPI_SUCCESS;

    MPIR_Datatype_iscontig(sendtype, &sendtype_iscontig);
    MPIR_Datatype_iscontig(recvtype, &recvtype_iscontig);

    if (sendtype_iscontig && recvtype_iscontig)
    {
        MPID_Datatype_get_size_macro(sendtype, sendsize);
        memcpy(recvbuf, sendbuf, sendcount*sendsize);
    }
    else {
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        mpi_errno = MPIC_Sendrecv ( sendbuf, sendcount, sendtype,
                                    rank, MPIR_LOCALCOPY_TAG, 
                                    recvbuf, recvcount, recvtype,
                                    rank, MPIR_LOCALCOPY_TAG,
                                    MPI_COMM_WORLD, MPI_STATUS_IGNORE );
    }
    return mpi_errno;
}


int MPIC_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno, context_id;
    MPID_Request *request_ptr=NULL;
    MPID_Comm *comm_ptr=NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIC_ISEND);

    MPID_MPI_PT2PT_FUNC_ENTER_FRONT(MPID_STATE_MPIC_ISEND);

    MPID_Comm_get_ptr( comm, comm_ptr );
    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Isend(buf, count, datatype, dest, tag, comm_ptr,
                          context_id, &request_ptr); 
    if (mpi_errno != MPI_SUCCESS)
    {
	MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPIC_ISEND);
	return mpi_errno;
    }

    *request = request_ptr->handle;

    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPIC_ISEND);
    return mpi_errno;
}


int MPIC_Irecv(void *buf, int count, MPI_Datatype datatype, int
               source, int tag, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno, context_id;
    MPID_Request *request_ptr=NULL;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIC_IRECV);

    MPID_MPI_PT2PT_FUNC_ENTER_BACK(MPID_STATE_MPIC_IRECV);

    MPID_Comm_get_ptr( comm, comm_ptr );
    context_id = (comm_ptr->comm_kind == MPID_INTRACOMM) ?
        MPID_CONTEXT_INTRA_COLL : MPID_CONTEXT_INTER_COLL;

    mpi_errno = MPID_Irecv(buf, count, datatype, source, tag, comm_ptr,
                          context_id, &request_ptr); 
    if (mpi_errno != MPI_SUCCESS)
    {
	MPID_MPI_PT2PT_FUNC_EXIT_BACK(MPID_STATE_MPIC_IRECV);
	return mpi_errno;
    }

    *request = request_ptr->handle;

    MPID_MPI_PT2PT_FUNC_EXIT_BACK(MPID_STATE_MPIC_IRECV);
    return mpi_errno;
}
