/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "viaimpl.h"

int via_get_business_card(char *value, int length)
{
    if (length < 1)
    {
	return -1;
    }
    snprintf(value, length, "none");
    return MPI_SUCCESS;
}
