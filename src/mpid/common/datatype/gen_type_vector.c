/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpid_dataloop.h>
#include <mpiimpl.h>

/*@
   MPID_Dataloop_create_vector

   Arguments:
+  int count
.  int blocklength
.  MPI_Aint stride
.  int strideinbytes
.  MPI_Datatype oldtype
.  MPID_Dataloop **dlp_p
.  int *dlsz_p
.  int *dldepth_p
-  int flags

@*/
int PREPEND_PREFIX(Dataloop_create_vector)(int count,
					   int blocklength,
					   MPI_Aint stride,
					   int strideinbytes,
					   DLOOP_Type oldtype,
					   DLOOP_Dataloop **dlp_p,
					   int *dlsz_p,
					   int *dldepth_p,
					   int flags)
{
    int mpi_errno, is_builtin;
    int new_loop_sz, new_loop_depth;

    DLOOP_Dataloop *new_dlp;

    /* if count or blocklength are zero, handle with contig code,
     * call it a int
     */
    if (count == 0 || blocklength == 0)
    {

	mpi_errno = PREPEND_PREFIX(Dataloop_create_contiguous)(0,
							       MPI_INT,
							       dlp_p,
							       dlsz_p,
							       dldepth_p,
							       flags);
	return mpi_errno;
    }

    /* optimization:
     *
     * if count == 1, store as a contiguous rather than a vector dataloop.
     */
    if (count == 1) {
	mpi_errno = PREPEND_PREFIX(Dataloop_create_contiguous)(blocklength,
							       oldtype,
							       dlp_p,
							       dlsz_p,
							       dldepth_p,
							       flags);
	return mpi_errno;
    }

    is_builtin = (DLOOP_Handle_hasloop_macro(oldtype)) ? 0 : 1;

    if (is_builtin) {
	new_loop_sz = sizeof(DLOOP_Dataloop);
	new_loop_depth = 1;
    }
    else {
	int old_loop_sz = 0, old_loop_depth = 0;

	DLOOP_Handle_get_loopsize_macro(oldtype, old_loop_sz, 0);
	DLOOP_Handle_get_loopdepth_macro(oldtype, old_loop_depth, 0);

	/* TODO: ACCOUNT FOR PADDING IN LOOP_SZ HERE */
	new_loop_sz = sizeof(DLOOP_Dataloop) + old_loop_sz;
	new_loop_depth = old_loop_depth + 1;
    }

    new_dlp = PREPEND_PREFIX(Dataloop_alloc)(new_loop_sz);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dlp)
    {
	/* TODO: NO MORE MPI ERROR CREATE CODES */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
					 MPIR_ERR_RECOVERABLE,
					 "MPID_Dataloop_create_vector",
					 __LINE__,
					 MPI_ERR_OTHER,
					 "**nomem",
					 0);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    if (is_builtin) {
	int basic_sz = 0;

	DLOOP_Handle_get_size_macro(oldtype, basic_sz);
	new_dlp->kind = DLOOP_KIND_VECTOR | DLOOP_FINAL_MASK;

	if (flags & MPID_DATALOOP_ALL_BYTES)
	{

	    blocklength       *= basic_sz;
	    new_dlp->el_size   = 1;
	    new_dlp->el_extent = 1;
	    new_dlp->el_type   = MPI_BYTE;
	}
	else
	{
	    new_dlp->el_size   = basic_sz;
	    new_dlp->el_extent = new_dlp->el_size;
	    new_dlp->el_type   = oldtype;
	}

	new_dlp->loop_params.c_t.dataloop = NULL;
    }
    else /* user-defined base type (oldtype) */ {
	char *curpos;
	DLOOP_Dataloop *old_loop_ptr;
	int old_loop_sz = 0;

	new_dlp->kind = DLOOP_KIND_VECTOR;
	DLOOP_Handle_get_size_macro(oldtype, new_dlp->el_size);
	DLOOP_Handle_get_extent_macro(oldtype, new_dlp->el_extent);
	DLOOP_Handle_get_basic_type_macro(oldtype, new_dlp->el_type);

	DLOOP_Handle_get_loopptr_macro(oldtype, old_loop_ptr, 0);
	DLOOP_Handle_get_loopsize_macro(oldtype, old_loop_sz, 0);

	/* copy in old dataloop */
	curpos = (char *) new_dlp;
	curpos += sizeof(struct MPID_Dataloop);
	/* TODO: ACCOUNT FOR PADDING HERE */

	PREPEND_PREFIX(Dataloop_copy)(curpos, old_loop_ptr, old_loop_sz);
	new_dlp->loop_params.c_t.dataloop = (DLOOP_Dataloop *) curpos;
    }

    /* vector-specific members
     *
     * stride stored in dataloop is always in bytes for local rep of type
     */
    new_dlp->loop_params.v_t.count     = count;
    new_dlp->loop_params.v_t.blocksize = blocklength;
    new_dlp->loop_params.v_t.stride    = (strideinbytes) ? stride :
	stride * new_dlp->el_extent;

    *dlp_p     = new_dlp;
    *dlsz_p    = new_loop_sz;
    *dldepth_p = new_loop_depth;

    return MPI_SUCCESS;
}
