/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "shmimpl.h"

int shm_can_connect(char *business_card)
{
    if (strncmp(business_card, SHM_Process.host, 100))
	return TRUE;
    return FALSE;
}
