/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Get_universe_size
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Get_universe_size(int * universe_size)
{
    int pmi_errno = PMI_SUCCESS;
    int mpi_errno = MPI_SUCCESS;
    
    pmi_errno = PMI_Get_universe_size(universe_size);
    MPIU_ERR_CHKANDJUMP1((pmi_errno != PMI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**pmi_get_universe_size",
			"**pmi_get_universe_size %d", pmi_errno);
    if (universe_size < 0)
    {
	*universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
    }

  fn_exit:
    return mpi_errno;

  fn_fail:
    *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
    goto fn_exit;
}
