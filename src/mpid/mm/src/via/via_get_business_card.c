/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "viaimpl.h"

int via_get_business_card(char *value, int length)
{
    MM_ENTER_FUNC(VIA_GET_BUSINESS_CARD);

    if (length < 1)
    {
	MM_EXIT_FUNC(VIA_GET_BUSINESS_CARD);
	return -1;
    }
    snprintf(value, length, "none");

    MM_EXIT_FUNC(VIA_GET_BUSINESS_CARD);
    return MPI_SUCCESS;
}
