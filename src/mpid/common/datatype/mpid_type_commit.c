/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>
#include <assert.h>

/*@
  MPID_Type_commit
 
  Input Parameters:
. datatype_p - pointer to MPI datatype

  Output Parameters:

  Return Value:
  0 on success, -1 on failure.
@*/
int MPID_Type_commit(MPI_Datatype *datatype_p)
{
    int count;
    MPI_Aint first, last;
    MPID_Datatype *datatype_ptr;
    MPID_Segment *segp;

    if (HANDLE_GET_KIND(*datatype_p) == HANDLE_KIND_BUILTIN) return 0;

    MPID_Datatype_get_ptr( *datatype_p, datatype_ptr );
    datatype_ptr->is_committed = 1;

    /* determine number of contiguous blocks in the type */
    segp = MPID_Segment_alloc();
    MPID_Segment_init(0, 1, *datatype_p, segp); /* 0 is bufptr, 1 is count */

    first = 0;
    last  = SEGMENT_IGNORE_LAST;

    MPID_Segment_count_contig_blocks(segp,
				     first,
				     &last,
				     &count);

    datatype_ptr->n_contig_blocks = count;

    MPID_Segment_free(segp);

    MPIU_DBG_PRINTF(("# contig blocks = %d\n",
		     (int) datatype_ptr->n_contig_blocks));

#if 0
    MPIDI_Dataloop_dot_printf(datatype_ptr->loopinfo, 0, 1);
#endif

    return 0;
}

