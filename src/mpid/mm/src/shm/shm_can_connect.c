/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "shmimpl.h"

int shm_can_connect(char *business_card)
{
    MM_ENTER_FUNC(SHM_CAN_CONNECT);

    if (strncmp(business_card, SHM_Process.host, 100))
    {
	MM_EXIT_FUNC(SHM_CAN_CONNECT);
	return TRUE;
    }

    MM_EXIT_FUNC(SHM_CAN_CONNECT);
    return FALSE;
}
