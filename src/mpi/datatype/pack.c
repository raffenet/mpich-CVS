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
    int mpi_errno = MPI_SUCCESS, first, last;
    MPID_Comm *comm_ptr = NULL;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_Segment *segp;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_PACK);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_PACK);
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
/*             MPID_Comm_valid_ptr( comm_ptr, mpi_errno ); */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PACK);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    /* TODO: CHECK RETURN VALUES?? */
    /* TODO: SHOULD THIS ALL BE IN A MPID_PACK??? */
    segp = MPID_Segment_alloc();
    MPID_Segment_init(inbuf, incount, datatype, segp);

    /* NOTE: the use of buffer values and positions in MPI_Pack and in
     * MPID_Segment_pack are quite different.  See code or docs or something.
     */
    first = *position;
    last  = outcount;

    MPID_Segment_pack(segp,
		      first,
		      &last,
		      (void *) ((char *) outbuf + first));

    *position = last;

    MPID_Segment_free(segp);

#if 0
    /* This is a temporary call */
    MPIR_Segment_pack( datatype_ptr->opt_loopinfo, inbuf, outbuf );
#endif

    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PACK);
    return MPI_SUCCESS;
}






