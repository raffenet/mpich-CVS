/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   mm_packer_write - write

   Parameters:
+  struct MPIDI_VC *vc_ptr - virtual connection pointer

   Notes:
@*/
int mm_packer_write(struct MPIDI_VC *vc_ptr)
{
    MM_Car *car_ptr;
    car_ptr = vc_ptr->writeq_head;
    while (car_ptr)
    {
	switch (car_ptr->request_ptr->mm.buf_type)
	{
	case MM_NULL_BUFFER:
	    err_printf("error, cannot pack from a null buffer\n");
	    break;
	case MM_TMP_BUFFER:
	    break;
	case MM_VEC_BUFFER:
	    break;
#ifdef WITH_METHOD_SHM
	case MM_SHM_BUFFER:
	    break;
#endif
#ifdef WITH_METHOD_VIA
	case MM_VIA_BUFFER:
	    break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
	case MM_VIA_RDMA_BUFFER:
	    break;
#endif
#ifdef WITH_METHOD_NEW
	case MM_NEW_METHOD_BUFFER:
	    break;
#endif
	default:
	    err_printf("illegal buffer type: %d\n", car_ptr->request_ptr->mm.buf_type);
	    break;
	}
	car_ptr = car_ptr->qnext_ptr;
    }

    return 0;
}
