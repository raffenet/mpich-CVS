/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pmi.h"

/*@
   mm_get_vc - get the virtual connection pointer

   Parameters:
+  MPID_Comm *comm_ptr - communicator
-  int dest - destination

   Notes:
@*/
MPIDI_VC *mm_get_vc(MPID_Comm *comm_ptr, int rank)
{
    MPIDI_VC *vc_ptr;

    if ((rank < 0) || (rank >= comm_ptr->size))
	return NULL;

    if (comm_ptr->vcrt == NULL)
    {
	MPID_VCRT_Create(comm_ptr->size, &comm_ptr->vcrt);
	MPID_VCRT_Get_ptr(comm_ptr->vcrt, &comm_ptr->vcr);
    }

    vc_ptr = comm_ptr->vcr[rank];
    if (vc_ptr == NULL)
    {
	/* insert code to figure out which method to use for this rank */
	char key[100], *value;
	int valuelen = PMI_KVS_Get_value_length_max();

	value = (char*)malloc(valuelen+1);
	snprintf(key, 100, "tcpinfo-%d", rank);
	PMI_KVS_Get(MPID_Process.pmi_kvsname, key, value);
	/*tcp_alloc_vc(comm_ptr);*/
    }

    return vc_ptr;
}
