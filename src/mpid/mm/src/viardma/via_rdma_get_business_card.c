/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "via_rdmaimpl.h"

int via_rdma_get_business_card(char *value, int length)
{
    if (length < 1)
    {
	return -1;
    }
    *value = '\0';
    return MPI_SUCCESS;
}
