/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

void MPIR_WaitForDebugger( void )
{
    volatile int dbg_release = 0;
#ifdef MPID_HAS_PROC_INFO
    /* Check to see if we're not the master,
     * and wait for the debugger to attach if we're 
     * a slave. The debugger will reset the debug_gate.
     * There is no code in the library which will do it !
     */
    if (MPIR_being_debugged && MPID_MyWorldRank != 0) {
	while (MPIR_debug_gate == 0) {
	    /* Wait to be attached to, select avoids 
	     * signaling and allows a smaller timeout than 
	     * sleep(1)
	     */
	    struct timeval timeout;
	    timeout.tv_sec  = 0;
	    timeout.tv_usec = 250000;
	    select( 0, (void *)0, (void *)0, (void *)0,
		    &timeout );
	}
    }
#endif
    if (getenv("MPIEXEC_DEBUG")) {
	while (!dbg_release) ; 
    }
}
