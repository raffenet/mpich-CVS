/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* 
 * This routine computes the extent of a datatype.  
 * This routine handles not only the various upper bound and lower bound
 * markers but also the alignment rules set by the environment (the PAD).
 */
/* *** NOT DONE *** */
#ifdef FOO
MPI_Aint MPIR_Type_compute_extent( MPID_Datatype *datatype_ptr )
{
    /* Compute the \mpids{MPI_Datatype}{ub} */
    If a sticky ub exists for the old datatype (datatypes for struct) {
	use the \mpids{MPI_Datatype}{sticky_ub} and set the sticky ub flag
        (\mpids{MPI_Datatype}{MPID_TYPE_STICKY_UB}).
    }
    else {
        use the \mpids{MPI_Datatype}{true_ub}
    }
   /* Similar for the \mpids{MPI_Datatype}{lb},
   \mpids{MPI_Datatype}{sticky_lb},
   \mpids{MPI_Datatype}{MPID_TYPE_STICKY_LB}   and
   \mpids{MPI_Datatype}{true_lb}. */

   /* Determine PAD from alignment rules */
   datatype_ptr->extent = datatype_ptr->ub - datatype_ptr->lb + PAD;

   /* Similar for \mpids{MPI_Datatype}{true_extent} */
									       }
#endif
