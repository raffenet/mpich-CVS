/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Pack */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Pack = PMPI_Pack
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Pack  MPI_Pack
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Pack as PMPI_Pack
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Pack PMPI_Pack

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Pack

/*@
   MPI_Pack - short description

   Arguments:
+  void *inbuf - input buffer
.  int incount - input count
.  MPI_Datatype datatype - datatype
.  void *outbuf - output buffer
.  int outcount - output count
.  int *position - position
-  MPI_Comm comm - communicator

   Notes (from the specifications):

   The input value of position is the first location in the output buffer to be
   used for packing.  position is incremented by the size of the packed message,
   and the output value of position is the first location in the output buffer
   following the locations occupied by the packed message.  The comm argument is
   the communicator that will be subsequently used for sending the packed
   message.


.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Pack(void *inbuf,
	     int incount,
	     MPI_Datatype datatype,
	     void *outbuf, 
	     int outcount,
	     int *position,
	     MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Pack";
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint first, last;
    MPID_Comm *comm_ptr = NULL;
    MPID_Segment *segp;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_PACK);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_PACK);

    MPID_Comm_get_ptr(comm, comm_ptr);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COUNT(incount,mpi_errno);
	    MPIR_ERRTEST_COUNT(outcount,mpi_errno);
	    /* NOTE: inbuf could be null (MPI_BOTTOM) */
	    MPIR_ERRTEST_ARGNULL(outbuf, "output buffer", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(position, "position", mpi_errno);
            /* Validate comm_ptr */
	    /* If comm_ptr is not valid, it will be reset to null */
            MPID_Comm_valid_ptr(comm_ptr, mpi_errno);

	    MPIR_ERRTEST_DATATYPE_NULL(datatype, "datatype", mpi_errno);
	    if (mpi_errno == MPI_SUCCESS) {
		if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
		    MPID_Datatype *datatype_ptr = NULL;

		    MPID_Datatype_get_ptr(datatype, datatype_ptr);
		    MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		    MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
		}
	    }
	    if (mpi_errno != MPI_SUCCESS) {
		MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PACK);
		return MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

#ifdef HAVE_ERROR_CHECKING /* IMPLEMENTATION-SPECIFIC ERROR CHECKS */
    {
	int tmp_sz;

	MPID_BEGIN_ERROR_CHECKS;
	/* Verify that there is space in the buffer to pack the type */
	MPID_Datatype_get_size_macro(datatype, tmp_sz);

	if (tmp_sz * incount > outcount - *position) {
	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG, "**arg", 0);
	    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PACK);
	    return MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
	}
	MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */
    
    /* TODO: CHECK RETURN VALUES?? */
    /* TODO: SHOULD THIS ALL BE IN A MPID_PACK??? */
    segp = MPID_Segment_alloc();
    if (segp == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment");
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_pack", "**mpi_pack %p %d %D %p %d %p %C", inbuf, incount, datatype, outbuf, outcount, position, comm);
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PACK);
	return MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    }
    mpi_errno = MPID_Segment_init(inbuf, incount, datatype, segp);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_pack", "**mpi_pack %p %d %D %p %d %p %C", inbuf, incount, datatype, outbuf, outcount, position, comm);
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PACK);
	return MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    }

    /* NOTE: the use of buffer values and positions in MPI_Pack and in
     * MPID_Segment_pack are quite different.  See code or docs or something.
     */
    first = 0;
    last  = SEGMENT_IGNORE_LAST;

    MPID_Segment_pack(segp,
		      first,
		      &last,
		      (void *) ((char *) outbuf + *position));

    *position += (int) last;

    MPID_Segment_free(segp);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PACK);
    return MPI_SUCCESS;
}
