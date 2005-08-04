/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pmi.h"

/*
 * MPID_Get_universe_size()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Get_universe_size
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Get_universe_size(int  * universe_size)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno = PMI_SUCCESS;  /* brad : added */

/* #   if defined(MPIDI_CH3_IMPLEMENTS_GET_UNIVERSE_SIZE) */
/*     { */
/* 	mpi_errno = MPIDI_CH3_Get_universe_size(universe_size); brad : original */
        /* brad : cut and pasted old sock/src/ch3_get_universe_size.c */
        pmi_errno = PMI_Get_universe_size(universe_size);
        MPIU_ERR_CHKANDJUMP1((pmi_errno != PMI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**pmi_get_universe_size",
                             "**pmi_get_universe_size %d", pmi_errno);
        if (*universe_size < 0)
        {
            *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
        }
        /* brad : end of cut and paste */
        if (mpi_errno) goto fn_fail;
/*     } */
/* #   else */
#if 0
    {  /* brad : eventually, this case will not exist, MPIDI_CH3_IMPLEMENTS_GET_UNIVERSE_SIZE
        *         will be unneccessary, and ch3_get_universe_size.c can be removed from
        *         <channel>/src/Makefile.sm
        */
	*universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
    }
#endif
    
  fn_exit:
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
/* #if defined(MPIDI_CH3_IMPLEMENTS_GET_UNIVERSE_SIZE) */
  fn_fail:
/* #endif */
    *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
