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


int MPIR_Localcopy(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype)
{
    int sendtype_iscontig, recvtype_iscontig, sendsize;
    int rank, mpi_errno = MPI_SUCCESS;
    MPI_Status status;

    if (HANDLE_GET_KIND(sendtype) == HANDLE_KIND_BUILTIN)
        sendtype_iscontig = 1;
    else {
        sendtype_iscontig = 0;
        /* CHANGE THIS TO CHECK THE is_contig FIELD OF THE DATATYPE */
    }
    if (HANDLE_GET_KIND(recvtype) == HANDLE_KIND_BUILTIN)
        recvtype_iscontig = 1;
    else {
        recvtype_iscontig = 0;
        /* CHANGE THIS TO CHECK THE is_contig FIELD OF THE DATATYPE */
    }

    if (sendtype_iscontig && recvtype_iscontig)
    {
        MPID_Datatype_get_size_macro(sendtype, sendsize);
        memcpy(recvbuf, sendbuf, sendcount*sendsize);
    }
    else {
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        mpi_errno = MPIC_Sendrecv ( sendbuf, sendcount, sendtype,
                                    rank, MPIR_LOCALCOPY_TAG, 
                                    recvbuf, recvcount, recvtype,
                                    rank, MPIR_LOCALCOPY_TAG,
                                    MPI_COMM_WORLD, &status );
    }
    return mpi_errno;
}


/*
int MPIR_Init_op_table()
{

    MPIR_Op_table = (MPI_User_function **)
        MPIU_Malloc(MPIR_PREDEF_OP_COUNT * sizeof(MPI_User_function *));

    MPIR_Op_table[(int)MPI_MAX] = MPIR_MAX;
    MPIR_Op_table[(int)MPI_MIN] = MPIR_MIN;
    MPIR_Op_table[(int)MPI_SUM] = MPIR_SUM;
    MPIR_Op_table[(int)MPI_PROD] = MPIR_PROD;
    MPIR_Op_table[(int)MPI_LAND] = MPIR_LAND;
    MPIR_Op_table[(int)MPI_BAND] = MPIR_BAND;
    MPIR_Op_table[(int)MPI_LOR] = MPIR_LOR;
    MPIR_Op_table[(int)MPI_BOR] = MPIR_BOR;
    MPIR_Op_table[(int)MPI_LXOR] = MPIR_LXOR;
    MPIR_Op_table[(int)MPI_BXOR] = MPIR_BXOR;
    MPIR_Op_table[(int)MPI_MINLOC] = MPIR_MINLOC;
    MPIR_Op_table[(int)MPI_MAXLOC] = MPIR_MAXLOC;
}
*/
