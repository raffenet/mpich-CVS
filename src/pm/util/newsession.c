/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "pmutilconf.h"
#include "pmutil.h"
#include <errno.h>

/* ------------------------------------------------------------------------ */
/* On some systems (SGI IRIX 6), process exit sometimes kills all processes
 * in the process GROUP.  This code attempts to fix that.  
 * We DON'T do it if stdin (0) is connected to a terminal, because that
 * disconnects the process from the terminal.
 * ------------------------------------------------------------------------ */
void CreateNewSession( void )
{
#if defined(HAVE_SETSID) && defined(HAVE_ISATTY) && \
    defined(USE_NEW_SESSION) && defined(HAVE_GETSID)
if (!isatty(0) && !isatty(1) && !isatty(2) && getsid(0) != getpid()) {
    pid_t rc;
    /* printf( "Creating a new session\n" ); */
/*    printf( "Session id = %d and process id = %d\n", getsid(0), getpid() );*/
    rc = setsid();
    if (rc < 0) {
	MPIU_Internal_sys_error_printf( "setsid", errno, 
				"Could not create new process group\n" );
	}
    }
#endif
}
