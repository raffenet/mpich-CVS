/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "bsocket.h"

int mm_close(int conn)
{
    MM_ENTER_FUNC(MM_CLOSE);
    beasy_closesocket(conn);
    MM_EXIT_FUNC(MM_CLOSE);
    return 0;
}

