/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>
#include <assert.h>

/* #define MPID_TYPE_ALLOC_DEBUG */

/*@
  MPID_Type_vector - create a vector datatype
 
  Input Parameters:
+ count - number of blocks in vector
. blocklength - number of elements in each block
. stride - distance from beginning of one block to the next (see next
  parameter for units)
. strideinbytes - if nonzero, then stride is in bytes, otherwise stride
  is in terms of extent of oldtype
- oldtype - type (using handle) of datatype on which vector is based

  Output Parameters:
. newtype - handle of new vector datatype

  Return Value:
  0 on success, MPI error code on failure.
@*/
int MPID_Type_vector(int count,
		     int blocklength,
		     MPI_Aint stride,
		     int strideinbytes,
		     MPI_Datatype oldtype,
		     MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int is_builtin, old_is_contig;
    MPI_Aint el_sz;
    MPI_Datatype el_type;
    MPI_Aint old_lb, old_ub, old_extent, old_true_lb, old_true_ub, eff_stride;

    MPID_Datatype *new_dtp;

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    if (!new_dtp) {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					 "MPID_Type_vector", __LINE__,
					 MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }

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

    if (is_builtin) {
	el_sz   = MPID_Datatype_get_basic_size(oldtype);
	el_type = oldtype;

	old_lb        = 0;
	old_true_lb   = 0;
	old_ub        = el_sz;
	old_true_ub   = el_sz;
	old_extent    = el_sz;
	old_is_contig = 1;

	new_dtp->size           = count * blocklength * el_sz;
	new_dtp->has_sticky_lb  = 0;
	new_dtp->has_sticky_ub  = 0;

	new_dtp->alignsize    = el_sz; /* ??? */
	new_dtp->n_elements   = count * blocklength;
	new_dtp->element_size = el_sz;
	new_dtp->eltype       = el_type;

	eff_stride = (strideinbytes) ? stride : (stride * el_sz);
    }
    else /* user-defined base type (oldtype) */ {
	MPID_Datatype *old_dtp;

	MPID_Datatype_get_ptr(oldtype, old_dtp);
	el_sz   = old_dtp->element_size;
	el_type = old_dtp->eltype;

	old_lb        = old_dtp->lb;
	old_true_lb   = old_dtp->true_lb;
	old_ub        = old_dtp->ub;
	old_true_ub   = old_dtp->true_ub;
	old_extent    = old_dtp->extent;
	old_is_contig = old_dtp->is_contig;

	new_dtp->size           = count * blocklength * old_dtp->size;
	new_dtp->has_sticky_lb  = old_dtp->has_sticky_lb;
	new_dtp->has_sticky_ub  = old_dtp->has_sticky_ub;

	new_dtp->alignsize    = old_dtp->alignsize;
	new_dtp->n_elements   = count * blocklength * old_dtp->n_elements;
	new_dtp->element_size = el_sz;
	new_dtp->eltype       = el_type;

	eff_stride = (strideinbytes) ? stride : (stride * old_dtp->extent);

    }

    MPID_DATATYPE_VECTOR_LB_UB(count,
			       eff_stride,
			       blocklength,
			       old_lb,
			       old_ub,
			       old_extent,
			       new_dtp->lb,
			       new_dtp->ub);
    new_dtp->true_lb = new_dtp->lb + (old_true_lb - old_lb);
    new_dtp->true_ub = new_dtp->ub + (old_true_ub - old_ub);
    new_dtp->extent  = new_dtp->ub - new_dtp->lb;

    /* new type is only contig for N types if old one was and
     * size and extent of new type are equivalent.
     *
     * Q: is this really stringent enough?  is there an overlap case
     *    for which this would fail?
     */
    new_dtp->is_contig = (new_dtp->size == new_dtp->extent) ? old_is_contig : 0;

    /* fill in dataloop */
    MPID_Dataloop_create_vector(count,
				blocklength,
				stride,
				strideinbytes,
				oldtype,
				&(new_dtp->loopinfo),
				&(new_dtp->loopsize),
				&(new_dtp->loopinfo_depth),
				0);

    *newtype = new_dtp->handle;

#ifdef MPID_TYPE_ALLOC_DEBUG
    MPIU_dbg_printf("(h)vector type %x created.\n", new_dtp->handle);
#endif
    return MPI_SUCCESS;
}


void MPID_Dataloop_create_vector(int count,
				 int blocklength,
				 MPI_Aint stride,
				 int strideinbytes,
				 MPI_Datatype oldtype,
				 MPID_Dataloop **dlp_p,
				 int *dlsz_p,
				 int *dldepth_p,
				 int flags)
{
    int is_builtin;
    int new_loop_sz, new_loop_depth;

    MPID_Datatype *old_dtp = NULL;
    struct MPID_Dataloop *new_dlp;

    /* optimization:
     *
     * if count == 1, store as a contiguous rather than a vector dataloop.
     */
    if (count == 1) {
	MPID_Dataloop_create_contiguous(blocklength,
					oldtype,
					dlp_p,
					dlsz_p,
					dldepth_p,
					flags);
	return;
    }

    is_builtin = (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN);

    if (is_builtin) {
	new_loop_sz = sizeof(struct MPID_Dataloop);
	new_loop_depth = 1;
    }
    else {
	MPID_Datatype_get_ptr(oldtype, old_dtp);
	new_loop_sz = sizeof(struct MPID_Dataloop) + old_dtp->loopsize;
	/* TODO: ACCOUNT FOR PADDING IN LOOP_SZ HERE */

	new_loop_depth = old_dtp->loopinfo_depth + 1;
    }

    new_dlp = (struct MPID_Dataloop *) MPIU_Malloc(new_loop_sz);
    assert(new_dlp != NULL);

    if (is_builtin) {
	new_dlp->kind                  = DLOOP_KIND_VECTOR | DLOOP_FINAL_MASK;
#if 0
	new_dlp->handle                = new_dtp->handle;
#endif
	if (flags & MPID_DATALOOP_ALL_BYTES) {
	    blocklength       *= MPID_Datatype_get_basic_size(oldtype);
	    new_dlp->el_size   = 1;
	    new_dlp->el_extent = 1;
	    new_dlp->el_type   = MPI_BYTE;
	}
	else {
	    new_dlp->el_size   = MPID_Datatype_get_basic_size(oldtype);
	    new_dlp->el_extent = new_dlp->el_size;
	    new_dlp->el_type   = oldtype;
	}

	new_dlp->loop_params.c_t.dataloop = NULL;
    }
    else /* user-defined base type (oldtype) */ {
	char *curpos;

	new_dlp->kind      = DLOOP_KIND_VECTOR;
#if 0
	new_dlp->handle    = new_dtp->handle;
#endif
	new_dlp->el_size   = old_dtp->size;
	new_dlp->el_extent = old_dtp->extent;
	new_dlp->el_type   = old_dtp->eltype;

	/* copy in old dataloop */
	curpos = (char *) new_dlp;
	curpos += sizeof(struct MPID_Dataloop);
	/* TODO: ACCOUNT FOR PADDING HERE */

	MPID_Dataloop_copy(curpos, old_dtp->loopinfo, old_dtp->loopsize);
	new_dlp->loop_params.c_t.dataloop = (struct MPID_Dataloop *) curpos;
    }

    /* vector-specific members
     *
     * stride stored in dataloop is always in bytes for local rep of type
     */
    new_dlp->loop_params.v_t.count     = count;
    new_dlp->loop_params.v_t.blocksize = blocklength;
    new_dlp->loop_params.v_t.stride    = (strideinbytes) ? stride :
	stride * new_dlp->el_extent;

    *dlp_p  = new_dlp;
    *dlsz_p = new_loop_sz;
    *dldepth_p = new_loop_depth;

    return;
}
