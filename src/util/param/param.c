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

  This implements a relatively simple system that stores key/value pairs.
  Values are normally set at initialization and then not changed, so
  a simple sorted array of entries can be used.
*/

typedef struct {
    char *name;
    enum { MPIU_STRING, MPIU_INT } kind;
    union {
	char *string_value;
	int  int_value;
    };
} Param_entry;

static int nentries = 0;
static Param_entry *param_table = 0;


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
    if (def_file && def_file[0]) {
	/* Read the file */
	;
    }
    return 0;
}

int MPIU_Param_bcast( void )
{
    return;
}

int MPIU_Param_register( const char name[], const char envname[], 
                         const char description[] )
{
    return 0;
}

int MPIU_Param_get_int( const char name[], int default_val, int *value )
{
    int i, cmp;
    /* Search through the ordered table that is param_table.  
       Linear search for now; binary tree search is almost as easy */
    for (i=0; i<nentries; i++) {
	cmp = strcmp( param_table[i].name, name );
	if (cmp == 0) {
	    if (param_table[i].kind == MPIU_INT) {
		*value = param_table[i].int_value;
		return 0;
	    }
	    else {
		return 2;
	    }
	}
	else if (cmp < 0) {
	    *value = default_val;
	    return 1;
	}
    }
    return 0;
}

int MPIU_Param_get_string( const char name[], const char *default_val,
                           char **value )
{
    return 0;
}

void MPIU_Param_finalize( void )
{
    return;
}
