/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* The following is used to tell a debugger the location of the shared
   library that the debugger can load in order to access information about
   the parallel program, such as message queues */
#ifdef HAVE_DEBUGGER_SUPPORT
#ifdef MPICH_INFODLL_LOC
char MPIR_dll_name[] = MPICH_INFODLL_LOC;
#endif

/* 
 * The following variables are used to interact with the debugger.
 *
 * MPIR_debug_state 
 *    Values are 0 (before MPI_Init), 1 (after MPI_init), and 2 (Aborting).
 * MPIR_debug_gate
 *    The debugger will set this to 1 when the debugger attaches 
 *    to the process. 
 * MPIR_being_debugged
 *    Set to 1 if the process is *started* under the debugger 
 *    This is *not* set by the debugger, but is set by the
 *    presence of an environment variable (MPIEXEC_BEGIN_DEBUGGED)
 */
volatile int MPIR_debug_state = 0;
volatile int MPIR_debug_gate  = 0;
int MPIR_being_debugged       = 0;
#endif

void MPIR_WaitForDebugger( void )
{
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
	while (!MPIR_debug_gate) ; 
    }
}

void *MPIR_Breakpoint(void);
/* 
 * This routine is a special dummy routine that is used to provide a
 * location for a debugger to set a breakpoint on, allowing a user (and the
 * debugger) to attach to MPI processes after MPI_Init succeeds but before
 * MPI_Init returns control to the user.
 *
 * This routine can also initialize any datastructures that are required
 * 
 */
void * MPIR_Breakpoint( void )
{
    return 0;
}
