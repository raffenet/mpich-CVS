/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>

/* #define MPID_TYPE_ALLOC_DEBUG */

/*@
  MPID_Type_contiguous - create a contiguous datatype
 
  Input Parameters:
+ count - number of elements in the contiguous block
- oldtype - type (using handle) of datatype on which vector is based

  Output Parameters:
. newtype - handle of new contiguous datatype

  Return Value:
  MPI_SUCCESS on success, MPI error code on failure.
@*/
int MPID_Type_contiguous(int count,
			 MPI_Datatype oldtype,
			 MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int is_builtin;
    int el_sz;
    MPI_Datatype el_type;
    MPID_Datatype *new_dtp;

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					 "MPID_Type_contiguous",
					 __LINE__, MPI_ERR_OTHER,
					 "**nomem", 0);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* handle is filled in by MPIU_Handle_obj_alloc() */
    MPIU_Object_set_ref(new_dtp, 1);
    new_dtp->is_permanent = 0;
    new_dtp->is_committed = 0;
    new_dtp->attributes   = NULL;
    new_dtp->cache_id     = 0;
    new_dtp->name[0]      = 0;
    new_dtp->contents     = NULL;

    new_dtp->loopsize       = -1;
    new_dtp->loopinfo       = NULL;
    new_dtp->loopinfo_depth = -1;
    
    is_builtin = (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN);

    if (count == 0)
    {
	/* we are interpreting the standard here based on the fact that
	 * with a zero count there is nothing in the typemap.
	 *
	 * we handle this case explicitly to get it out of the way.
	 */
	new_dtp->size          = 0;
	new_dtp->has_sticky_ub = 0;
	new_dtp->has_sticky_lb = 0;
	new_dtp->lb            = 0;
	new_dtp->ub            = 0;
	new_dtp->true_lb       = 0;
	new_dtp->true_ub       = 0;
	new_dtp->extent        = 0;

	new_dtp->alignsize     = 0;
	new_dtp->element_size  = 0;
	new_dtp->eltype        = 0;
	new_dtp->n_elements    = 0;
	new_dtp->is_contig     = 1;

	mpi_errno = MPID_Dataloop_create_contiguous(0,
						    MPI_INT, /* dummy type */
						    &(new_dtp->loopinfo),
						    &(new_dtp->loopsize),
						    &(new_dtp->loopinfo_depth),
						    0);
	*newtype = new_dtp->handle;
	
	return mpi_errno;
    }
    else if (is_builtin)
    {
	el_sz   = MPID_Datatype_get_basic_size(oldtype);
	el_type = oldtype;

	new_dtp->size          = count * el_sz;
	new_dtp->has_sticky_ub = 0;
	new_dtp->has_sticky_lb = 0;
	new_dtp->true_lb       = 0;
	new_dtp->lb            = 0;
	new_dtp->true_ub       = count * el_sz;
	new_dtp->ub            = new_dtp->true_ub;
	new_dtp->extent        = new_dtp->ub - new_dtp->lb;

	new_dtp->alignsize     = el_sz;
	new_dtp->n_elements    = count;
	new_dtp->element_size  = el_sz;
        new_dtp->eltype        = el_type;
	new_dtp->is_contig     = 1;

    }
    else
    {
	/* user-defined base type (oldtype) */
	MPID_Datatype *old_dtp;

	MPID_Datatype_get_ptr(oldtype, old_dtp);
	el_sz   = old_dtp->element_size;
	el_type = old_dtp->eltype;

	new_dtp->size           = count * old_dtp->size;
	new_dtp->has_sticky_ub  = old_dtp->has_sticky_ub;
	new_dtp->has_sticky_lb  = old_dtp->has_sticky_lb;

	MPID_DATATYPE_CONTIG_LB_UB(count,
				   old_dtp->lb,
				   old_dtp->ub,
				   old_dtp->extent,
				   new_dtp->lb,
				   new_dtp->ub);

	/* easiest to calc true lb/ub relative to lb/ub; doesn't matter
	 * if there are sticky lb/ubs or not when doing this.
	 */
	new_dtp->true_lb = new_dtp->lb + (old_dtp->true_lb - old_dtp->lb);
	new_dtp->true_ub = new_dtp->ub + (old_dtp->true_ub - old_dtp->ub);
	new_dtp->extent  = new_dtp->ub - new_dtp->lb;

	new_dtp->alignsize    = old_dtp->alignsize;
	new_dtp->n_elements   = count * old_dtp->n_elements;
	new_dtp->element_size = old_dtp->element_size;
        new_dtp->eltype       = el_type;

	new_dtp->is_contig    = old_dtp->is_contig;
    }

    /* fill in dataloop */
    mpi_errno = MPID_Dataloop_create_contiguous(count,
						oldtype,
						&(new_dtp->loopinfo),
						&(new_dtp->loopsize),
						&(new_dtp->loopinfo_depth),
						0);

    *newtype = new_dtp->handle;

#ifdef MPID_TYPE_ALLOC_DEBUG
    MPIU_dbg_printf("contig type %x created.\n", new_dtp->handle);
#endif

    return mpi_errno;
}

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
int MPID_Dataloop_create_contiguous(int count,
				    MPI_Datatype oldtype,
				    MPID_Dataloop **dlp_p,
				    int *dlsz_p,
				    int *dldepth_p,
				    int flags)
{
    int mpi_errno, is_builtin, apply_contig_coalescing = 0;
    int new_loop_sz, new_loop_depth;

    MPID_Datatype *old_dtp = NULL;
    struct MPID_Dataloop *new_dlp;

    is_builtin = (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN);

    if (is_builtin)
    {
	new_loop_sz    = sizeof(struct MPID_Dataloop);
	new_loop_depth = 1;
    }
    else
    {
	MPID_Datatype_get_ptr(oldtype, old_dtp); /* fills in old_dtp */

	/* if we have a simple combination of contigs, coalesce */
	if (((old_dtp->loopinfo->kind & DLOOP_KIND_MASK) == DLOOP_KIND_CONTIG)
	    && (old_dtp->size == old_dtp->extent))
	{
	    /* will just copy contig and multiply count */
	    apply_contig_coalescing = 1;
	    new_loop_sz             = old_dtp->loopsize;
	    new_loop_depth          = old_dtp->loopinfo_depth;
	}
	else
	{
	    /* TODO: ACCOUNT FOR PADDING IN LOOP_SZ HERE */
	    new_loop_sz    = sizeof(struct MPID_Dataloop) + old_dtp->loopsize;
	    new_loop_depth = old_dtp->loopinfo_depth + 1;
	}
    }

    new_dlp = MPID_Dataloop_alloc(new_loop_sz);
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
	new_dlp->kind      = DLOOP_KIND_CONTIG | DLOOP_FINAL_MASK;

	if (flags & MPID_DATALOOP_ALL_BYTES)
	{
	    count             *= MPID_Datatype_get_basic_size(oldtype);
	    new_dlp->el_size   = 1;
	    new_dlp->el_extent = 1;
	    new_dlp->el_type   = MPI_BYTE;
	}
	else
	{
	    new_dlp->el_size   = MPID_Datatype_get_basic_size(oldtype);
	    new_dlp->el_extent = new_dlp->el_size;
	    new_dlp->el_type   = oldtype;
	}

	new_dlp->loop_params.c_t.dataloop = NULL;

	new_dlp->loop_params.c_t.count = count;
    }
    else
    {
	/* user-defined base type (oldtype) */
	if (apply_contig_coalescing)
	{
	    MPID_Dataloop_copy(new_dlp, old_dtp->loopinfo, old_dtp->loopsize);
	    new_dlp->loop_params.c_t.count *= count;
	}
	else
	{
	    char *curpos;
	    
	    new_dlp->kind      = DLOOP_KIND_CONTIG;
	    new_dlp->el_size   = old_dtp->size;
	    new_dlp->el_extent = old_dtp->extent;
	    new_dlp->el_type   = old_dtp->eltype;
	    
	    /* copy in old dataloop */
	    curpos = (char *) new_dlp;
	    curpos += sizeof(struct MPID_Dataloop);
	    /* TODO: ACCOUNT FOR PADDING HERE */
	    
	    MPID_Dataloop_copy(curpos, old_dtp->loopinfo, old_dtp->loopsize);
	    new_dlp->loop_params.c_t.dataloop = (struct MPID_Dataloop *) curpos;

	    new_dlp->loop_params.c_t.count = count;
	}
    }

    *dlp_p  = new_dlp;
    *dlsz_p = new_loop_sz;
    *dldepth_p = new_loop_depth;
    return MPI_SUCCESS;
}
