/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "errcodes.h"

/*
 * This file contains the routines needed to implement the MPI routines that
 * can add error classes and codes during runtime.  This file is organized
 * so that applications that do not use the MPI-2 routines to create new
 * error codes will not load any of the 
 */

/* Local data structures.
   A message may be associated with each class and code.
   Since we limit the number of user-defined classes and code (no more
   than 256 of each), we allocate an array of pointers to the messages here.

   We also allow the use of instance-specific messages.  This allows an 
   implementation of ADI-3 to add messages without changing the predefined
   set of error messages.
*/

static int  initialized = 0;
static char *(user_class_msgs[256]) = { 0 };
static char *(user_code_msgs[256]) = { 0 };
static char *(user_instance_msgs[256]) = { 0 };
static int  first_free_class = 0;
static int  first_free_code  = 0;
/* Each user_code_msgs is allowed to have a single related instance message,
   so no separate counter is required for allocating user_instance_msgs */

/* This external allows this package to define the routine that converts
   dynamically assigned codes and classes to their corresponding strings. 
   A cleaner implementation could replace this exposed global with a method
   defined in the error_string.c file that allowed this package to set 
   the routine. */
extern void (*MPIR_dnyErr_to_string)( int, char *, int );

int MPIR_Err_get_string( int code, char *msg, int msg_len );

/*+
  MPIR_Err_set_msg - Change the message for an error code or class

  Input Parameter:
+ code - Error code or class
- msg  - New message to use

  Notes:
  This routine is needed to implement 'MPI_Add_error_string'
 
  Module:
  Error
@*/
int MPIR_Err_set_msg( int code, const char *msg_string )
{
    return MPI_SUCCESS;
}

/*+
  MPIR_Err_add_class - Add an error class with an associated string

  Input Parameters:
+ msg_string - Message text for this class.  A null string may be used, in
  which case 'MPIR_Err_set_msg' should be called later.
- instance_msg_string - Instance-specific message string.  See the error 
  reporting overview.  May be the null string.

  Return value:
  An error class.

  Notes:
  This is used to implement 'MPI_Add_error_class'; it may also be used by a 
  device to add device-specific error classes.  Unlike the MPI version, this 
  combines returning a class with setting the associated message.  

  Predefined classes are handled directly; this routine is not used to 
  initialize the predefined MPI error classes.  This is done to reduce the
  number of steps that must be executed when starting an MPI program.

  Module:
  Error
  @*/
int MPIR_Err_add_class( const char *msg_string, 
			const char *instance_msg_string )
{
    int new_class;

    /* Get new class */
    MPIR_Fetch_and_increment( &first_free_class, &new_class );

    if (new_class >= ERROR_MAX_NCLASS) {
	/* Fail if out of classes */
	return -1;
    }
    
    if (new_class == 0) {
	initialized = 1;
	MPIR_dnyErr_to_string = MPIR_Err_get_string;
    }

    if (msg_string) {
	user_class_msgs[new_class] = MPIU_Strdup( msg_string );
    }
    else {
	user_class_msgs[new_class] = 0;
    }
    return MPI_SUCCESS;
}

/*+
  MPIR_Err_add_code - Add an error code with an associated string

  Input Parameters:
+ class - Class that the code belongs to
. msg_string - Message text for this code
- instance_msg_string - Instance-specific message string.  See the error 
  reporting overview.  May be the null string.

  Return value:
  An error code.

  Notes:
  This is used to implement 'MPI_Add_error_code'; it may also be used by a 
  device to add device-specific error codes.  Unlike the MPI version, this 
  combines returning a code with setting the associated message.  
 
  Module:
  Error
  @*/
int MPIR_Err_add_code( int class, const char *msg_string, 
		       const char *instance_msg_string )
{
    return MPI_SUCCESS;
}

/*+
  MPIR_Err_delete_code - Delete an error code and its associated string

  Input Parameter:
. code - Code to delete.
 
  Notes:
  This routine is not needed to implement any MPI routine (there are no
  routines for deleting error codes or classes in MPI-2), but it is 
  included both for completeness and to remind the implementation to 
  carefully manage the memory used for dynamically create error codes and
  classes.

  Module:
  Error
  @*/
void MPIR_Err_delete_code( int code )
{
}

/*+
  MPIR_Err_delete_class - Delete an error class and its associated string

  Input Parameter:
. class - Class to delete.
 
  Module:
  Error
  @*/
void MPIR_Err_delete_class( int class )
{
}

/*+
  MPIR_Err_get_string - Get the message string that corresponds to an error
  class or code

  Input Parameter:
+ code - An error class or code.  If a code, it must have been created by 
  'MPIR_Err_create_code'.
- msg_len - Length of 'msg'.

  Output Parameter:
. msg - A null-terminated text string of length (including the null) of no
  more than 'msg_len'.  

  Return value:
  Zero on success.  Non-zero returns indicate either (a) 'msg_len' is too
  small for the message or (b) the value of 'code' is neither a valid 
  error class or code.

  Notes:
  This routine is used to implement 'MPI_ERROR_STRING'.

  Module:
  Error 

  Question:
  What values should be used for the error returns?  Should they be
  valid error codes?

  How do we get a good value for 'MPI_MAX_ERROR_STRING' for 'mpi.h'?
  See 'errgetmsg' for one idea.

  @*/
int MPIR_Err_get_string( int code, char *msg, int msg_len )
{
    return MPI_SUCCESS;
}

