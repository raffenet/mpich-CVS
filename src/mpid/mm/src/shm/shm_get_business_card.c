/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "shmimpl.h"

int shm_get_business_card(char *value, int length)
{
    MM_ENTER_FUNC(SHM_GET_BUSINESS_CARD);

    strncpy(value, SHM_Process.host, length-1);
    value[length-1] = '\0';

    MM_EXIT_FUNC(SHM_GET_BUSINESS_CARD);
    return MPI_SUCCESS;
}
