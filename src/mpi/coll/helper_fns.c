/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* These functions are used in the implementation of collective
   operations. They are wrappers around MPID send/recv functions. They do
   sends/receives by setting the context offset to
   MPID_INTRA_CONTEXT_COLL. */

int MPIC_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm)
{
    int mpi_errno;
    MPID_Request *request_ptr=NULL;
    MPID_Comm *comm_ptr=NULL;

    MPID_Comm_get_ptr( comm, comm_ptr );
    mpi_errno = MPID_Send(buf, count, datatype, dest, tag, comm_ptr,
                          MPID_CONTEXT_INTRA_COLL, &request_ptr); 
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    if (request_ptr) {
        MPIR_Wait(request_ptr);
        MPID_Request_release(request_ptr);
    }
    return mpi_errno;
}

int MPIC_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
	     MPI_Comm comm, MPI_Status *status)
{
    int mpi_errno;
    MPID_Request *request_ptr=NULL;
    MPID_Comm *comm_ptr = NULL;

    MPID_Comm_get_ptr( comm, comm_ptr );
    mpi_errno = MPID_Recv(buf, count, datatype, source, tag, comm_ptr,
                          MPID_CONTEXT_INTRA_COLL, status, &request_ptr); 
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    if (request_ptr) {
        MPIR_Wait(request_ptr);
        if (status != NULL)
            *status = request_ptr->status;
        mpi_errno = request_ptr->status.MPI_ERROR;
        MPID_Request_release(request_ptr);
    }
    return mpi_errno;
}

int MPIC_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  int dest, int sendtag, void *recvbuf, int recvcount,
                  MPI_Datatype recvtype, int source, int recvtag,
                  MPI_Comm comm, MPI_Status *status) 
{
    MPID_Request *recv_req_ptr=NULL, *send_req_ptr=NULL;
    MPI_Request recv_req;
    int mpi_errno;
    MPID_Comm *comm_ptr = NULL;

    MPID_Comm_get_ptr( comm, comm_ptr );

    mpi_errno = MPID_Irecv(recvbuf, recvcount, recvtype, source, recvtag,
                           comm_ptr, MPID_CONTEXT_INTRA_COLL, &recv_req_ptr);
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;
    mpi_errno = MPID_Isend(sendbuf, sendcount, sendtype, dest, sendtag, 
                           comm_ptr, MPID_CONTEXT_INTRA_COLL, &send_req_ptr); 
    if (mpi_errno != MPI_SUCCESS) return mpi_errno;

    MPIR_Wait(send_req_ptr); 
    MPID_Request_release(send_req_ptr);

    MPIR_Wait(recv_req_ptr);
    if (status != NULL)
        *status = recv_req_ptr->status;
    mpi_errno = recv_req_ptr->status.MPI_ERROR;
    MPID_Request_release(recv_req_ptr);

    return mpi_errno;
}
