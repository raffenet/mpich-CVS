/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpid_dataloop.h>
#include <mpiimpl.h>

/*@
   MPID_Dataloop_contiguous - create the dataloop representation for a
   contiguous datatype

   Arguments:
+  int count,
.  MPI_Datatype oldtype,
.  MPID_Dataloop **dlp_p,
.  int *dlsz_p,
.  int *dldepth_p,
-  int flags

.N Errors
.N MPI_SUCCESS
@*/
int PREPEND_PREFIX(Dataloop_create_contiguous)(int count,
					       DLOOP_Type oldtype,
					       DLOOP_Dataloop **dlp_p,
					       int *dlsz_p,
					       int *dldepth_p,
					       int flags)
{
    int mpi_errno, is_builtin, apply_contig_coalescing = 0;
    int new_loop_sz, new_loop_depth;

    DLOOP_Dataloop *new_dlp;

    is_builtin = (DLOOP_Handle_hasloop_macro(oldtype)) ? 0 : 1;

    if (is_builtin)
    {
	new_loop_sz    = sizeof(DLOOP_Dataloop);
	new_loop_depth = 1;
    }
    else
    {
	int old_loop_sz = 0, old_loop_depth = 0, old_size = 0;
	DLOOP_Offset old_extent = 0;
	DLOOP_Dataloop *old_loop_ptr;

	DLOOP_Handle_get_loopsize_macro(oldtype, old_loop_sz, 0);
	DLOOP_Handle_get_loopdepth_macro(oldtype, old_loop_depth, 0);
	DLOOP_Handle_get_loopptr_macro(oldtype, old_loop_ptr, 0);
	DLOOP_Handle_get_size_macro(oldtype, old_size);
	DLOOP_Handle_get_extent_macro(oldtype, old_extent);

	/* if we have a simple combination of contigs, coalesce */
	if (((old_loop_ptr->kind & DLOOP_KIND_MASK) == DLOOP_KIND_CONTIG)
	    && (old_size == old_extent))
	{
	    /* will just copy contig and multiply count */
	    apply_contig_coalescing = 1;
	    new_loop_sz             = old_loop_sz;
	    new_loop_depth          = old_loop_depth;
	}
	else
	{
	    /* TODO: ACCOUNT FOR PADDING IN LOOP_SZ HERE */
	    new_loop_sz    = sizeof(DLOOP_Dataloop) + old_loop_sz;
	    new_loop_depth = old_loop_depth + 1;
	}
    }

    new_dlp = PREPEND_PREFIX(Dataloop_alloc)(new_loop_sz);
    if (!new_dlp) {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
					 MPIR_ERR_RECOVERABLE,
					 "MPID_Dataloop_create_contiguous",
					 __LINE__,
					 MPI_ERR_OTHER,
					 "**nomem",
					 0);
	return mpi_errno;
	/* --END ERROR HANDLING-- */
    }

    if (is_builtin)
    {
	int basic_sz = 0;

	DLOOP_Handle_get_size_macro(oldtype, basic_sz);
	new_dlp->kind = DLOOP_KIND_CONTIG | DLOOP_FINAL_MASK;

	if (flags & MPID_DATALOOP_ALL_BYTES)
	{
	    count             *= basic_sz;
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

	new_dlp->loop_params.c_t.count = count;
    }
    else
    {
	/* user-defined base type (oldtype) */
	DLOOP_Dataloop *old_loop_ptr;
	int old_loop_sz = 0;

	DLOOP_Handle_get_loopptr_macro(oldtype, old_loop_ptr, 0);
	DLOOP_Handle_get_loopsize_macro(oldtype, old_loop_sz, 0);

	if (apply_contig_coalescing)
	{
	    PREPEND_PREFIX(Dataloop_copy)(new_dlp, old_loop_ptr,
					  old_loop_sz);
	    new_dlp->loop_params.c_t.count *= count;
	}
	else
	{
	    char *curpos;

	    new_dlp->kind = DLOOP_KIND_CONTIG;
	    DLOOP_Handle_get_size_macro(oldtype, new_dlp->el_size);
	    DLOOP_Handle_get_extent_macro(oldtype, new_dlp->el_extent);
	    DLOOP_Handle_get_basic_type_macro(oldtype, new_dlp->el_type);
	    
	    /* copy in old dataloop */
	    curpos = (char *) new_dlp;
	    curpos += sizeof(DLOOP_Dataloop);
	    /* TODO: ACCOUNT FOR PADDING HERE */
	    
	    PREPEND_PREFIX(Dataloop_copy)(curpos, old_loop_ptr, old_loop_sz);

	    new_dlp->loop_params.c_t.dataloop = (DLOOP_Dataloop *) curpos;
	    new_dlp->loop_params.c_t.count = count;
	}
    }

    *dlp_p     = new_dlp;
    *dlsz_p    = new_loop_sz;
    *dldepth_p = new_loop_depth;

    return MPI_SUCCESS;
}
