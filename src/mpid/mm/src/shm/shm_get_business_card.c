/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "shmimpl.h"

int shm_get_business_card(char *value, int length)
{
    MPID_STATE_DECL(MPID_STATE_SHM_GET_BUSINESS_CARD);
    MPID_FUNC_ENTER(MPID_STATE_SHM_GET_BUSINESS_CARD);

    strncpy(value, SHM_Process.host, length-1);
    value[length-1] = '\0';

    MPID_FUNC_EXIT(MPID_STATE_SHM_GET_BUSINESS_CARD);
    return MPI_SUCCESS;
}
