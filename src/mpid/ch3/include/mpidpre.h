/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDPRE_H_INCLUDED)
#define MPICH_MPIDPRE_H_INCLUDED

#include "mpid_ch3_pre.h"

typedef struct MPID_VC
{
    volatile int ref_count;
    int lpid;
    struct MPID_Request * sendq_head;
    struct MPID_Request * sendq_tail;
    struct MPID_Request * recv_active;
    
#if defined(MPID_CH3_VC_DECL)
    MPID_CH3_VC_DECL
#endif
}
MPID_VC;

#endif /* !defined(MPICH_MPIDPRE_H_INCLUDED) */
