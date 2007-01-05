/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpid_dataloop.h>
#include <stdlib.h>
#include <limits.h>

void PREPEND_PREFIX(Dataloop_create)(MPI_Datatype type,
				     DLOOP_Dataloop **dlp_p,
				     int *dlsz_p,
				     int *dldepth_p,
				     int flags)
{
    int i;

    int align_sz=8, types_sz, struct_sz, ints_sz, epsilon;
    int nr_ints, nr_aints, nr_types, combiner;
    MPI_Datatype *types;
    int *ints;
    MPI_Aint *aints;

    MPID_Datatype_contents *cp;
    MPID_Datatype *dtp;
    MPID_Datatype *old_dtp;
    DLOOP_Dataloop *old_dlp;
    int old_dlsz, old_dldepth;

    int dummy1, dummy2, dummy3, type0_combiner, ndims;
    MPI_Datatype tmptype;

    MPI_Aint stride;
    MPI_Aint *disps;

    /* TODO: USE THE MPI FUNCTION */
    PMPI_Type_get_envelope(type, &nr_ints, &nr_aints, &nr_types, &combiner);

    /* exit prematurely if what we're given is a basic type */
    if (combiner == MPI_COMBINER_NAMED) {
	*dlp_p = NULL;
	*dlsz_p = 0;
	*dldepth_p = 0;
	return;
    }

    /* TODO: HANDLE MPI_FLOAT_INT ETC... */

    MPID_Datatype_get_ptr(type, dtp);

    cp = dtp->contents;

    /* TODO: THINK ABOUT PADDING... */
#ifdef HAVE_MAX_STRUCT_ALIGNMENT
    if (align_sz > HAVE_MAX_STRUCT_ALIGNMENT) {
	align_sz = HAVE_MAX_STRUCT_ALIGNMENT;
    }
#endif

    struct_sz = sizeof(MPID_Datatype_contents);
    types_sz  = nr_types * sizeof(MPI_Datatype);
    ints_sz   = nr_ints * sizeof(int);

    /* pad the struct, types, and ints before we allocate.
     *
     * note: it's not necessary that we pad the aints,
     *       because they are last in the region.
     */
    if ((epsilon = struct_sz % align_sz)) {
	struct_sz += align_sz - epsilon;
    }
    if ((epsilon = types_sz % align_sz)) {
	types_sz += align_sz - epsilon;
    }
    if ((epsilon = ints_sz % align_sz)) {
	ints_sz += align_sz - epsilon;
    }
    types = (MPI_Datatype *) (((char *) cp) + struct_sz);
    ints  = (int *) (((char *) types) + types_sz);
    aints = (MPI_Aint *) (((char *) ints) + ints_sz);

    /* first check for zero count on types where that makes sense */
    switch(combiner) {
	case MPI_COMBINER_CONTIGUOUS:
	case MPI_COMBINER_VECTOR:
	case MPI_COMBINER_HVECTOR_INTEGER:
	case MPI_COMBINER_HVECTOR:
	case MPI_COMBINER_INDEXED_BLOCK:
	case MPI_COMBINER_INDEXED:
	case MPI_COMBINER_HINDEXED_INTEGER:
	case MPI_COMBINER_HINDEXED:
	case MPI_COMBINER_STRUCT_INTEGER:
	case MPI_COMBINER_STRUCT:
	    if (ints[0] == 0) {
		PREPEND_PREFIX(Dataloop_create_contiguous)(0,
							   MPI_INT,
							   dlp_p,
							   dlsz_p,
							   dldepth_p,
							   flags);
		return;
	    }
	    break;
	default:
	    break;
    }

    /* recurse, processing types "below" this one before processing
     * this one, if those type don't already have dataloops.
     *
     * note: we'll have to recurse more in the struct case below.
     */
    PMPI_Type_get_envelope(types[0], &dummy1, &dummy2, &dummy3,
			   &type0_combiner);
    if (type0_combiner != MPI_COMBINER_NAMED)
    {
	MPID_Datatype_get_ptr(types[0], old_dtp);
	if (old_dtp->dataloop == NULL)
	{
	    PREPEND_PREFIX(Dataloop_create)(types[0],
					    &old_dlp,
					    &old_dlsz,
					    &old_dldepth,
					    flags);

	    /* TODO: MACROIZE */
	    old_dtp->dataloop = old_dlp;
	    old_dtp->dataloop_size = old_dlsz;
	    old_dtp->dataloop_depth = old_dldepth;
	}
    }
       
    switch(combiner)
    {
	case MPI_COMBINER_DUP:
	    if (type0_combiner != MPI_COMBINER_NAMED) {
		PREPEND_PREFIX(Dataloop_dup)(old_dlp, old_dlsz, dlp_p);
		*dlsz_p    = old_dlsz;
		*dldepth_p = old_dldepth;
	    }
	    else {
		PREPEND_PREFIX(Dataloop_create_contiguous)(1,
							   types[0], 
							   dlp_p, dlsz_p,
							   dldepth_p,
							   flags);
	    }
	    break;
	case MPI_COMBINER_RESIZED:
	    if (type0_combiner != MPI_COMBINER_NAMED) {
		PREPEND_PREFIX(Dataloop_dup)(old_dlp, old_dlsz, dlp_p);
		*dlsz_p    = old_dlsz;
		*dldepth_p = old_dldepth;
	    }
	    else {
		PREPEND_PREFIX(Dataloop_create_contiguous)(1,
							   types[0], 
							   dlp_p, dlsz_p,
							   dldepth_p,
							   flags);

		(*dlp_p)->el_extent = aints[1]; /* extent */
	    }
	    break;
	case MPI_COMBINER_CONTIGUOUS:
	    PREPEND_PREFIX(Dataloop_create_contiguous)(ints[0] /* count */,
						       types[0] /* oldtype */,
						       dlp_p, dlsz_p,
						       dldepth_p,
						       flags);
	    break;
	case MPI_COMBINER_VECTOR:
	    PREPEND_PREFIX(Dataloop_create_vector)(ints[0] /* count */,
						   ints[1] /* blklen */,
						   ints[2] /* stride */,
						   0 /* stride not bytes */,
						   types[0] /* oldtype */,
						   dlp_p, dlsz_p, dldepth_p,
						   flags);
	    break;
	case MPI_COMBINER_HVECTOR_INTEGER:
	case MPI_COMBINER_HVECTOR:
	    /* fortran hvector has integer stride in bytes */
	    if (combiner == MPI_COMBINER_HVECTOR_INTEGER) {
		stride = (MPI_Aint) ints[2];
	    }
	    else {
		stride = aints[0];
	    }

	    PREPEND_PREFIX(Dataloop_create_vector)(ints[0] /* count */,
						   ints[1] /* blklen */,
						   stride,
						   1 /* stride in bytes */,
						   types[0] /* oldtype */,
						   dlp_p, dlsz_p, dldepth_p,
						   flags);
	    break;
	case MPI_COMBINER_INDEXED_BLOCK:
	    PREPEND_PREFIX(Dataloop_create_blockindexed)(ints[0] /* count */,
							 ints[1] /* blklen */,
							 &ints[2] /* disps */,
							 0 /* disp not bytes */,
							 types[0] /* oldtype */,
							 dlp_p, dlsz_p,
							 dldepth_p,
							 flags);
	    break;
	case MPI_COMBINER_INDEXED:
	    PREPEND_PREFIX(Dataloop_create_indexed)(ints[0] /* count */,
						    &ints[1] /* blklens */,
						    &ints[ints[0]+1] /* disp */,
						    0 /* disp not in bytes */,
						    types[0] /* oldtype */,
						    dlp_p, dlsz_p, dldepth_p,
						    flags);
	    break;
	case MPI_COMBINER_HINDEXED_INTEGER:
	case MPI_COMBINER_HINDEXED:
	    if (combiner == MPI_COMBINER_HINDEXED_INTEGER) {
		disps = (MPI_Aint *) DLOOP_Malloc(ints[0] * sizeof(MPI_Aint));

		for (i=0; i < ints[0]; i++) {
		    disps[i] = (MPI_Aint) ints[ints[0] + 1 + i];
		}
	    }
	    else {
		disps = aints;
	    }

	    PREPEND_PREFIX(Dataloop_create_indexed)(ints[0] /* count */,
						    &ints[1] /* blklens */,
						    disps,
						    1 /* disp in bytes */,
						    types[0] /* oldtype */,
						    dlp_p, dlsz_p, dldepth_p,
						    flags);

	    if (combiner == MPI_COMBINER_HINDEXED_INTEGER) {
		DLOOP_Free(disps);
	    }

	    break;
	case MPI_COMBINER_STRUCT_INTEGER:
	case MPI_COMBINER_STRUCT:
	    for (i = 1; i < ints[0]; i++) {
		int type_combiner;
		PMPI_Type_get_envelope(types[i], &dummy1, &dummy2, &dummy3,
				       &type_combiner);

		if (type_combiner != MPI_COMBINER_NAMED) {
		    MPID_Datatype_get_ptr(types[i], old_dtp);
		    if (old_dtp->dataloop == NULL)
		    {
			PREPEND_PREFIX(Dataloop_create)(types[i],
							&old_dlp,
							&old_dlsz,
							&old_dldepth,
							flags);
			
			/* TODO: MACROIZE */
			old_dtp->dataloop = old_dlp;
			old_dtp->dataloop_size = old_dlsz;
			old_dtp->dataloop_depth = old_dldepth;
		    }
		}
	    }
	    if (combiner == MPI_COMBINER_STRUCT_INTEGER) {
		disps = (MPI_Aint *) MPIU_Malloc(ints[0] * sizeof(MPI_Aint));

		for (i=0; i < ints[0]; i++) {
		    disps[i] = (MPI_Aint) ints[ints[0] + 1 + i];
		}
	    }
	    else {
		disps = aints;
	    }

	    PREPEND_PREFIX(Dataloop_create_struct)(ints[0] /* count */,
						   &ints[1] /* blklens */,
						   disps,
						   types /* oldtype array */,
						   dlp_p, dlsz_p, dldepth_p,
						   flags);

	    if (combiner == MPI_COMBINER_STRUCT_INTEGER) {
		MPIU_Free(disps);
	    }

	    break;
	case MPI_COMBINER_SUBARRAY:
	    ndims = ints[0];
	    PREPEND_PREFIX(Type_convert_subarray)(ndims,
						  &ints[1] /* sizes */,
						  &ints[1+ndims] /* subsizes */,
						  &ints[1+2*ndims] /* starts */,
						  ints[1+3*ndims] /* order */,
						  types[0],
						  &tmptype);

	    PREPEND_PREFIX(Dataloop_create)(tmptype,
					    dlp_p,
					    dlsz_p,
					    dldepth_p,
					    flags);
	    
	    PMPI_Type_free(&tmptype);
	    break;
	case MPI_COMBINER_DARRAY:
	    ndims = ints[2];
	    PREPEND_PREFIX(Type_convert_darray)(ints[0] /* size */,
						ints[1] /* rank */,
						ndims,
						&ints[3] /* gsizes */,
						&ints[3+ndims] /*distribs */,
						&ints[3+2*ndims] /* dargs */,
						&ints[3+3*ndims] /* psizes */,
						ints[3+4*ndims] /* order */,
						types[0],
						&tmptype);

	    PREPEND_PREFIX(Dataloop_create)(tmptype,
					    dlp_p,
					    dlsz_p,
					    dldepth_p,
					    flags);

	    PMPI_Type_free(&tmptype);
	    break;
	case MPI_COMBINER_F90_REAL:
	case MPI_COMBINER_F90_COMPLEX:
	case MPI_COMBINER_F90_INTEGER:
	    /* TODO: WHAT DO I DO HERE? */
	default:
	    DLOOP_Assert(0);
    }

    /* TODO: HOW DO WE CLEAN UP ALL THE EXTRA DATALOOPS THAT WE HAVE
     * CREATED IN THIS PROCESS???
     *
     * IN OTHER WORDS, ONE REASON TO DO THIS WAS TO AVOID DATALOOPS ON 
     * INTERMEDIATE TYPES...
     *
     * CLEAN UP IF WE CREATED IT? ARE THERE SCARY THREAD SAFETY ISSUES
     * TO THINK ABOUT HERE?
     *
     * I think I can safely just remove the dataloops on sub-types if
     * I created them here? With an assert that they aren't committed?
     */

    /* IDEA: KEEP TRACK OF # OF USES OF DATALOOP ON TYPE, KEEP IF
     * EXCEED SOME THRESHOLD EVEN IF INTERMEDIATE TYPE?
     */
    return;
}
