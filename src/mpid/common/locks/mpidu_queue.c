/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidu_queue.h"

char *MPIDU_Shm_asymm_base_addr = (char *)(MPIDU_SHM_ASYMM_NULL_VAL);

#undef FUNCNAME
#define FUNCNAME MPIDU_Shm_asymm_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDU_Shm_asymm_init(char *base)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIDU_Shm_asymm_base_addr != (char *)MPIDU_SHM_ASYMM_NULL_VAL) {
        /* the _base_addr has already been initialized */
        mpi_errno = MPI_ERR_INTERN;
        MPIU_ERR_POP (mpi_errno);
        goto fn_exit;
    }

    MPIDU_Shm_asymm_base_addr = base;

fn_exit:
    return mpi_errno;
}

