/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Alloc_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void *MPID_Alloc_mem( size_t size, MPID_Info *info_ptr )
{
    void *ap;
    MPIDI_STATE_DECL(MPID_STATE_MPID_ALLOC_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_ALLOC_MEM);

#   if defined(MPIDI_CH3_IMPLEMENTS_ALLOC_MEM)
    {
	ap = MPIDI_CH3_Alloc_mem(size, info_ptr);
    }
#   else
    MPIU_UNREFERENCED_ARG(info_ptr);
    {
        ap = MPIU_Malloc(size);
    }
#   endif
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_ALLOC_MEM);
    return ap;
}
