/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_Start_PT_epoch()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Start_PT_epoch
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Start_PT_epoch(int lock_type, int dest, int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS, comm_size;
    volatile int *shared_lock_state_baseaddr;
    MPIDU_Process_lock_t *locks_base_addr;
    MPID_Comm *comm_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_START_PT_EPOCH);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_START_PT_EPOCH);

    MPID_Comm_get_ptr( win_ptr->comm, comm_ptr );
    comm_size = comm_ptr->local_size;

    locks_base_addr = win_ptr->locks->addr;
    shared_lock_state_baseaddr = (volatile int *) ((char *) win_ptr->locks->addr + 
        comm_size * sizeof(MPIDU_Process_lock_t));

    if (lock_type == MPI_LOCK_SHARED) {

        /* acquire lock. increment shared lock state. release lock */

        MPIDU_Process_lock( &locks_base_addr[dest] );

        shared_lock_state_baseaddr[dest]++;

        MPIDU_Process_unlock( &locks_base_addr[dest] );

    }
    else {
        /* exclusive lock. acquire lock. check shared lock state. if non-zero 
           (shared lock exists), unlock and try again. else retain lock. */

        while (1) {
            MPIDU_Process_lock( &locks_base_addr[dest] );

            if (shared_lock_state_baseaddr[dest]) {
                MPIDU_Process_unlock( &locks_base_addr[dest] );
            }
            else {
                break;
            }
        }

        /* mark this as a passive target excl lock epoch so that a following accumulate 
           does not need to acquire a lock */
        win_ptr->pt_rma_excl_lock = 1;
    }


    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_START_PT_EPOCH);
    return mpi_errno;
}
