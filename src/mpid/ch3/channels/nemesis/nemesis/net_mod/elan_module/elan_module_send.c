/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "elan_module_impl.h"
#include "elan.h"
#include "elan_module.h"
#include "my_papi_defs.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int 
MPID_nem_elan_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{   
   int                    dest = vc->lpid;
   int                    mpi_errno = MPI_SUCCESS;
   
   MPIU_Assert (datalen <= MPID_NEM_MPICH2_DATA_LEN);
   
   fn_exit:
      return mpi_errno;
   fn_fail:
      goto fn_exit;   
}
