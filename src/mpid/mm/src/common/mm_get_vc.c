/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   mm_get_vc - get the virtual connection pointer

   Parameters:
+  MPID_Comm *comm_ptr - communicator
-  int dest - destination

   Notes:
@*/
MPIDI_VC *mm_get_vc(MPID_Comm *comm_ptr, int dest)
{
    MPIDI_VC *vc_ptr;

    if ((dest < 0) || (dest >= comm_ptr->size))
	return NULL;

    if (comm_ptr->vcrt == NULL)
    {
	mm_alloc_vc_table(comm_ptr);
    }

    vc_ptr = comm_ptr->vcrt[dest];
    if (vc_ptr == NULL)
    {
	/* insert code to figure out which method to use to
	   connect to this destination */
    }
    
    return vc_ptr;
}
