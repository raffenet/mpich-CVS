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
   MPI_Unpack - unpack

   Arguments:
+  void *inbuf - input buffer
.  int insize - input buffer size
.  int *position - position
.  void *outbuf - output buffer
.  int outcount - ouput count
.  MPI_Datatype datatype - datatype
-  MPI_Comm comm - communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
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
    int first, last;
    MPID_Comm *comm_ptr = NULL;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_Segment *segp;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_UNPACK);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
    MPID_Datatype_get_ptr( datatype, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_UNPACK);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* TODO: CHECK RETURN VALUES?? */
    /* TODO: SHOULD THIS ALL BE IN A MPID_PACK??? */
    segp = MPID_Segment_alloc();
    MPID_Segment_init(outbuf, outcount, datatype, segp);

    /* NOTE: the use of buffer values and positions in MPI_Pack and in
     * MPID_Segment_pack are quite different.  See code or docs or something.
     */
    first = *position;
    last  = insize;

    MPID_Segment_pack(segp,
		      first,
		      &last,
		      (void *) ((char *) outbuf + first));

    *position = last;

    MPID_Segment_free(segp);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_UNPACK);
    return MPI_SUCCESS;
}












