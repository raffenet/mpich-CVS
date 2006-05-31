/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem.h"

static int sense;
static int barrier_init = 0;


void MPID_nem_barrier_init (MPID_nem_barrier_t *barrier_region)
{
    MPID_nem_mem_region.barrier = barrier_region;
    MPID_nem_mem_region.barrier->val = 0;
    MPID_nem_mem_region.barrier->wait = 0;
    sense = 0;
    barrier_init = 1;
    MPID_NEM_WRITE_BARRIER();
}

/* FIXME: this is not a scalable algorithm because everyone is polling on the same cacheline */
void MPID_nem_barrier (int num_processes, int rank)
{
    MPIU_Assert (barrier_init);
    
    if (MPID_NEM_FETCH_AND_INC (&MPID_nem_mem_region.barrier->val) == MPID_nem_mem_region.num_local - 1)
    {
	MPID_nem_mem_region.barrier->val = 0;
	MPID_nem_mem_region.barrier->wait = 1 - sense;
	MPID_NEM_WRITE_BARRIER();
    }
    else
    {
	/* wait */
	while (MPID_nem_mem_region.barrier->wait == sense)
	    ; /* skip */
    }
    sense = 1 - sense;
}
