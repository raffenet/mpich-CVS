/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "errcodes.h"

#include <string.h>

/*
 * This file contains the routines needed to implement the MPI routines that
 * can add error classes and codes during runtime.  This file is organized
 * so that applications that do not use the MPI-2 routines to create new
 * error codes will not load any of this code.  
 * 
 * ROMIO will be customized to provide error messages with the same tools
 * as the rest of MPICH2 and will not rely on the dynamically assigned
 * error classes.  This leaves all of the classes and codes for the user.
 */

/* Local data structures.
   A message may be associated with each class and code.
   Since we limit the number of user-defined classes and code (no more
   than 256 of each), we allocate an array of pointers to the messages here.

   We also allow the use of instance-specific messages.  This allows an 
   implementation of ADI-3 to add messages without changing the predefined
   set of error messages.

   We *could* allow 256 codes with each class.  However, we don't expect 
   any need for this many codes, so we simply allow 256 (actually
   ERROR_MAX_NCODE) codes, and distribute these among the error codes.
*/

static int  not_initialized = 1;  /* This allows us to use atomic decr */
static char *(user_class_msgs[ERROR_MAX_NCLASS]) = { 0 };
static char *(user_code_msgs[ERROR_MAX_NCODE]) = { 0 };
static char *(user_instance_msgs[ERROR_MAX_NCODE]) = { 0 };
static int  first_free_class = 0;
static int  first_free_code  = 0;
#if MPID_MAX_THREAD_LEVEL >= MPI_THREAD_FUNNELED
volatile static int ready = 0;
#endif

/* Forward reference */
const char *MPIR_Err_get_dynerr_string( int code );

/* Each user_code_msgs is allowed to have a single related instance message,
   so no separate counter is required for allocating user_instance_msgs */

/* This external allows this package to define the routine that converts
   dynamically assigned codes and classes to their corresponding strings. 
   A cleaner implementation could replace this exposed global with a method
   defined in the error_string.c file that allowed this package to set 
   the routine. */

/* Local routine to initialize the data structures for the dynamic
   error classes and codes.

   MPIR_Init_err_dyncodes is called if initialized is false.  In
   a multithreaded case, it must check *again* in case two threads
   are in a race to call this routine
 */
static void MPIR_Init_err_dyncodes( void )
{
    int i;
#if MPID_MAX_THREAD_LEVEL > MPI_THREAD_FUNNELED
    { 
	int init_value;

	MPID_Atomic_decr_flag( &not_initialized, &inuse );
	if (inuse != 0) {
	    /* Some other thread is initializing the data.  Wait
	       until that thread completes */
	    while (!ready) {
		MPID_Thread_yield();
	    }
	}
    }
#else
    not_initialized = 0;
#endif
    
    for (i=0; i<ERROR_MAX_NCLASS; i++) {
	user_class_msgs[i] = 0;
    }
    for (i=0; i<ERROR_MAX_NCODE; i++) {
	user_code_msgs[i] = 0;
	user_instance_msgs[i] = 0;
    }
    /* Set the routine to provides access to the dynamically created
       error strings */
    MPIR_Process.errcode_to_string = MPIR_Err_get_dynerr_string;

#if MPID_MAX_THREAD_LEVEL >= MPI_THREAD_FUNNELED
    /* Release the other threads */
    /* FIXME - Add MPID_Write_barrier for thread-safe operation,
       or consider using a flag incr that contains a write barrier */
    ready = 1;
#endif
}

const char *MPIR_Err_get_dynerr_string( int code );

/*+
  MPIR_Err_set_msg - Change the message for an error code or class

  Input Parameter:
+ code - Error code or class
- msg  - New message to use

  Notes:
  This routine is needed to implement 'MPI_Add_error_string'.
 
  Module:
  Error
@*/
int MPIR_Err_set_msg( int code, const char *msg_string )
{
    if (not_initialized)
	MPIR_Init_err_dyncodes();

    /* FIXME 
       get index from code.  
       Get length of string
       save in usercodemsgs 
    */
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

    if (not_initialized)
	MPIR_Init_err_dyncodes();
	
    /* Get new class */
    MPIR_Fetch_and_increment( &first_free_class, &new_class );

    if (new_class >= ERROR_MAX_NCLASS) {
	/* Fail if out of classes */
	return -1;
    }
    
    if (msg_string) {
	user_class_msgs[new_class] = MPIU_Strdup( msg_string );
    }
    else {
	user_class_msgs[new_class] = 0;
    }
    return new_class;
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
    int new_code;

    if (not_initialized)
	MPIR_Init_err_dyncodes();

    /* Get the new code */
    MPIR_Fetch_and_increment( &first_free_code, &new_code );
    if (new_code >= ERROR_MAX_NCODE) {
	/* Fail if out of codes */
	return -1;
    }

    /* Create the full error code */
    new_code = class | ERROR_DYN_MASK | (new_code << ERROR_GENERIC_SHIFT);

    return new_code;
}

/*+
  MPIR_Err_delete_code - Delete an error code and its associated string

  Input Parameter:
. code - Code to delete.
 
  Notes:
  This routine is not needed to implement any MPI routine (there are no
  routines for deleting error codes or classes in MPI-2), but it is 
  included both for completeness and to remind the implementation to 
  carefully manage the memory used for dynamically created error codes and
  classes.

  Module:
  Error
  @*/
void MPIR_Err_delete_code( int code )
{
    if (not_initialized)
	MPIR_Init_err_dyncodes();
    /* FIXME - mark as free */
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
    if (not_initialized)
	MPIR_Init_err_dyncodes();
    /* FIXME - mark as free */
}

/* FIXME: For the delete code/class, at least delete if at the top of the
   list; slightly better is to keep minvalue of freed and count; whenever
   the minvalue + number = current top; reset.  This allows modular 
   alloc/dealloc to recover codes and classes independent of the order in
   which they are freed.
*/

/*+
  MPIR_Err_get_dynerr_string - Get the message string that corresponds to a
  dynamically created error class or code

  Input Parameter:
+ code - An error class or code.  If a code, it must have been created by 
  'MPIR_Err_create_code'.

  Return value:
  A poiner to a null-terminated text string with the corresponding error 
  message.  A null return indicates an error; usually the value of 'code' is 
  neither a valid error class or code.

  Notes:
  This routine is used to implement 'MPI_ERROR_STRING'.

  Module:
  Error 

  @*/
const char *MPIR_Err_get_dynerr_string( int code )
{
    return "unknown";
}

