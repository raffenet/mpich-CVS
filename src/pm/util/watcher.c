/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "pmutilconf.h"

/*
 * This file implements a "watcher" process.  It detects the exit of 
 * the watched process, and takes registered actions based on how the
 * process exits.  For example, it can report that a process exited due
 * to a signal.  
 *
 * Eventually, this package may also provide a way to register resources that
 * should also be watched, such as SYSV IPCs or other processes.
 *
 * NOT YET IMPLEMENTED
 *
 * Currently, this is just a place to hold code drawn from older sources
 */

/*
  Make sure that
  
  We close any open fds
  
  provide for logging of data to a file so that stdout/err is not required

  We are in a separate process group (to avoid being killed as part of the 
  same process group).  But be careful of the process group routine,
  since SYSV and non-SYSV are different (even to the number of
  arguments).  Some take a single arg; some take two args (and want
  0,0).

  

  
  
 */
