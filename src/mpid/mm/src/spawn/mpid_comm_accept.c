/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Comm_accept

/*@
   MPID_Comm_accept - communicator accept

   Arguments:
+  char *port_name - port name
.  MPI_Info info - info
.  int root - root
.  MPI_Comm comm - communicator
-  MPI_Comm *newcomm - new communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Comm_accept(char *port_name, MPID_Info *info_ptr, int root, MPID_Comm *comm_ptr, MPID_Comm **newcomm)
{
    static const char FCNAME[] = "MPID_Comm_accept";
    int conn;
//    char value[10];
//    int same_domain;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_COMM_ACCEPT);

    if (comm_ptr->rank == root)
    {
	conn = MM_Accept(info_ptr, port_name);
//	PMPI_Info_get(info, MPICH_PMI_SAME_DOMAIN_KEY, 10, value, &same_domain);

	/* Transfer stuff */

	MM_Close(conn);

	/* Bcast resulting intercommunicator stuff to the rest of this communicator */
    }
    else
    {
	/* Bcast resulting intercommunicator stuff */
    }

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_COMM_ACCEPT);
    return MPI_SUCCESS;
}
