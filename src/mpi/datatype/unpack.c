/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Unpack */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Unpack = PMPI_Unpack
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Unpack  MPI_Unpack
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Unpack as PMPI_Unpack
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Unpack PMPI_Unpack

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Unpack

/*@
    MPI_Unpack - Unpack a datatype into contiguous memory

Input Parameters:
+ inbuf - input buffer start (choice) 
. insize - size of input buffer, in bytes (integer) 
. position - current position in bytes (integer) 
. outcount - number of items to be unpacked (integer) 
. datatype - datatype of each output data item (handle) 
- comm - communicator for packed message (handle) 

Output Parameter:
. outbuf - output buffer start (choice) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_ARG

.seealso: MPI_Pack, MPI_Pack_size
@*/
int MPI_Unpack(void *inbuf,
	       int insize,
	       int *position,
	       void *outbuf,
	       int outcount,
	       MPI_Datatype datatype,
	       MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Unpack";
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint first, last;
    MPID_Comm *comm_ptr = NULL;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_Segment *segp;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_UNPACK);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_UNPACK);

    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
    MPID_Datatype_get_ptr( datatype, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_ARGNULL(inbuf, "input buffer", mpi_errno);
	    /* Note: outbuf could be MPI_BOTTOM; don't test for NULL */
	    MPIR_ERRTEST_COUNT(insize, mpi_errno);
	    MPIR_ERRTEST_COUNT(outcount, mpi_errno);
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
		MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
		MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
	    }
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_UNPACK);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* TODO: CHECK RETURN VALUES?? */
    /* TODO: SHOULD THIS ALL BE IN A MPID_UNPACK??? */
    segp = MPID_Segment_alloc();
    MPID_Segment_init(outbuf, outcount, datatype, segp);

    /* NOTE: the use of buffer values and positions in MPI_Unpack and in
     * MPID_Segment_unpack are quite different.  See code or docs or something.
     */
    first = 0;
    last  = SEGMENT_IGNORE_LAST;

    MPID_Segment_unpack(segp,
			first,
			&last,
			(void *) ((char *) inbuf + *position));

    *position += (int) last;

    MPID_Segment_free(segp);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_UNPACK);
    return MPI_SUCCESS;
}












