/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "shmimpl.h"

int shm_can_connect(char *business_card)
{
    MPID_STATE_DECLS;
    MPID_FUNC_ENTER(MPID_STATE_SHM_CAN_CONNECT);

    if (strncmp(business_card, SHM_Process.host, 100))
    {
	MPID_FUNC_EXIT(MPID_STATE_SHM_CAN_CONNECT);
	return TRUE;
    }

    MPID_FUNC_EXIT(MPID_STATE_SHM_CAN_CONNECT);
    return FALSE;
}
