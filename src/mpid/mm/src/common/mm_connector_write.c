/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pmi.h"

/*@
   mm_connector_write - write

   Parameters:
+  struct MPIDI_VC *vc_ptr - virtual connection pointer
-  MM_Car *car_ptr - communication agent request pointer

   Notes:
@*/
int mm_connector_write(struct MPIDI_VC *vc_ptr, MM_Car *car_ptr)
{
    mm_connector_connect(vc_ptr, car_ptr);
    return 0;
}
