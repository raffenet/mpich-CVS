/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "param.h"

/*
  This file implements the parameter routines.  These routines provide a 
  uniform mechanism to access parameters that are used within the mpich2 code.
*/


/*@
  MPIU_Param_init - Initialize the parameter code

  Input/Output Parameters:
+ argc_p - Pointer to argument count
- argv   - Argument vector

  Comments:
  This routine extracts parameter values from the command line, as
  well as initializing any values from the environment and from 
  a defaults file.  The default file is read by only one process; the
  routine 'MPIU_Param_bcast' propagates the values to other processes.
  
  See Also:
  MPIU_Param_bcast, MPIU_Param_get_int, MPIU_Param_get_string,
  MPIU_Param_finalize
  @*/
int MPIU_Param_init( int *argc_p, char *argv_p[], const char def_file[] )
{
    return 0;
}
