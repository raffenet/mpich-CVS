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
    assert (barrier_init);
    
    if (MPID_NEM_FETCH_AND_ADD (&MPID_nem_mem_region.barrier->val, 1) == MPID_nem_mem_region.num_local - 1)
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
