/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Pack_external */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Pack_external = PMPI_Pack_external
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Pack_external  MPI_Pack_external
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Pack_external as PMPI_Pack_external
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Pack_external PMPI_Pack_external

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Pack_external

/*@
   MPI_Pack_external - pack external

   Input Parameters:
+ datarep - data representation (string)  
. inbuf - input buffer start (choice)  
. incount - number of input data items (integer)  
. datatype - datatype of each input data item (handle)  
- outsize - output buffer size, in bytes (integer)  

   Output Parameter:
. outbuf - output buffer start (choice)  

   Input/Output Parameter:
. position - current position in buffer, in bytes (integer)  

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
@*/
int MPI_Pack_external(char *datarep,
		      void *inbuf,
		      int incount,
		      MPI_Datatype datatype,
		      void *outbuf,
		      MPI_Aint outcount,
		      MPI_Aint *position)
{
    static const char FCNAME[] = "MPI_Pack_external";
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint first, last;

    MPID_Segment *segp;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_PACK_EXTERNAL);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_PACK_EXTERNAL);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COUNT(incount, mpi_errno);
	    MPIR_ERRTEST_COUNT(outcount, mpi_errno);
	    /* NOTE: inbuf could be null (MPI_BOTTOM) */
	    MPIR_ERRTEST_ARGNULL(outbuf, "output buffer", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(position, "position", mpi_errno);

	    MPIR_ERRTEST_DATATYPE_NULL(datatype, "datatype", mpi_errno);
	    if (mpi_errno == MPI_SUCCESS) {
		if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
		    MPID_Datatype *datatype_ptr = NULL;

		    MPID_Datatype_get_ptr(datatype, datatype_ptr);
		    MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		    MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
		}
	    }
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    segp = MPID_Segment_alloc();
    /* --BEGIN ERROR HANDLING-- */
    if (segp == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
					 MPIR_ERR_RECOVERABLE,
					 FCNAME,
					 __LINE__,
					 MPI_ERR_OTHER,
					 "**nomem",
					 "**nomem %s",
					 "MPID_Segment");
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    mpi_errno = MPID_Segment_init(inbuf, incount, datatype, segp, 1);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /* NOTE: the use of buffer values and positions in MPI_Pack_external and
     * in MPID_Segment_pack_external are quite different.  See code or docs
     * or something.
     */
    first = 0;
    last  = SEGMENT_IGNORE_LAST;

    MPID_Segment_pack_external32(segp,
				 first,
				 &last,
				 (void *)((char *) outbuf + *position));

    *position += (int) last;

    MPID_Segment_free(segp);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PACK_EXTERNAL);
    return MPI_SUCCESS;
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno,
				     MPIR_ERR_RECOVERABLE,
				     FCNAME,
				     __LINE__,
				     MPI_ERR_OTHER,
				     "**mpi_pack_external",
				     "**mpi_pack_external %s %p %d %D %p %d %p",
				     datarep,
				     inbuf,
				     incount,
				     datatype,
				     outbuf,
				     outcount,
				     position);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PACK_EXTERNAL);
    return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
}
