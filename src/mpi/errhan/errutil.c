/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* style: allow:fprintf:4 sig:0 */

#include "mpiimpl.h"
#include "errcodes.h"

/* defmsg is generated automatically from the source files and contains
   all of the error messages */
#ifdef USE_WINCONF_H
#include "windefmsg.h"
#else
#include "defmsg.h" 
#endif

/* stdarg is required to handle the variable argument lists for 
   MPIR_Err_create_code */
#include <stdarg.h>

/* stdio is needed for vsprintf and vsnprintf */
#include <stdio.h>

static const char *get_class_msg( int class );
    
/*
 * Instance-specific error messages are stored in a ring.
 * Messages are written into the error_ring; the corresponding entry in
 * error_ring_seq is used to keep a sort of "checkvalue" to ensure that the
 * error code that points at this message is in fact for this particular 
 * message.  This is used to handle the unlikely but possible situation where 
 * so many error messages are generated that the ring is overlapped.
 *
 * The message arrays are preallocated to ensure that there is space for these
 * messages when an error occurs.  One variation would be to allow these
 * to be dynamically allocated, but it is probably better to either preallocate
 * these or turn off all error message generation (which will eliminate these
 * arrays).
 *
 * One possible alternative is to use the message ring *only* for instance
 * messages and use the predefined messages in-place for the generic
 * messages.  This approach is used to provide uniform handling of all 
 * error messages.
 */
#if MPICH_ERROR_MSG_LEVEL >= MPICH_ERROR_MSG_ALL
#define MAX_ERROR_RING ERROR_SPECIFIC_INDEX_SIZE
#define MAX_FCNAME_LEN 63

typedef struct MPIR_Err_msg
{
    /* identifier used to check for validity of instance-specific messages; consists of the class, generic index and hash of the
       specific message */
    int  id;
    
    /* The previous error code that caused this error to be generated; this allows errors to be chained together */
    int  prev_error;
    
    /* function name and line number where the error occurred */
    char fcname[MAX_FCNAME_LEN+1];
    
    /* actual error message */
    char msg[MPI_MAX_ERROR_STRING+1];
}
MPIR_Err_msg_t;

static MPIR_Err_msg_t ErrorRing[MAX_ERROR_RING];
static volatile unsigned int error_ring_loc = 0;
#if !defined(MPICH_SINGLE_THREADED)
static MPID_Thread_lock_t error_ring_mutex;
#endif
#endif

int MPIR_Err_print_stack_flag = FALSE;
static int MPIR_Err_abort_on_error = FALSE;

void MPIR_Err_init( void )
{
#   if MPICH_ERROR_MSG_LEVEL >= MPICH_ERROR_MSG_ALL
    {
	char *env;

	MPID_Thread_lock_init(&error_ring_mutex);
	/* TODO: get environment setting to see if single message or message stack should be printed */
	MPIR_Err_print_stack_flag = TRUE; /* for the moment... */
	env = getenv("MPICH_ABORT_ON_ERROR");
	if (env)
	{
	    if (strcmp(env, "1") == 0 || strcmp(env, "on") == 0 || strcmp(env, "yes") == 0)
		MPIR_Err_abort_on_error = TRUE;
	}
    }
#   endif
}

/* Special error handler to call if we are not yet initialized */
/* FIXME - NOT YET CALLED ANYWHERE */
void MPIR_Err_preinit( void )
{
    fprintf( stderr, "Error encountered before initializing MPICH\n" );
}

/*
 * This is the routine that is invoked by most MPI routines to 
 * report an error 
 */
int MPIR_Err_return_comm( MPID_Comm  *comm_ptr, const char fcname[], int errcode )
{
    const int class = ERROR_GET_CLASS(errcode);
    
    if (class > MPICH_ERR_LAST_CLASS)
    {
	if (errcode & ~ERROR_CLASS_MASK)
	{
	    MPIU_Error_printf("INTERNAL ERROR: Invalid error class (%d) encountered while returning from\n%s().  "
			      "Please file a bug report.  The error stack follows:\n", class);
	    MPIR_Err_print_stack(stderr, errcode);
	}
	else
	{
	    MPIU_Error_printf("INTERNAL ERROR: Invalid error class (%d) encountered while returning from\n%s().  "
			      "Please file a bug report.  No error stack is available.\n");
	}
	
	errcode = (errcode & ~ERROR_CLASS_MASK) | MPI_ERR_UNKNOWN;
    }
    
    /* First, check the nesting level */
    if (MPIR_Nest_value()) return errcode;
    
    if (!comm_ptr || comm_ptr->errhandler == NULL) {
	/* Try to replace with the default handler, which is the one on MPI_COMM_WORLD.  This gives us correct behavior for the
	   case where the error handler on MPI_COMM_WORLD has been changed. */
	if (MPIR_Process.comm_world)
	{
	    comm_ptr = MPIR_Process.comm_world;
	}
    }
    
    if (MPIR_Err_is_fatal(errcode) ||
	comm_ptr == NULL || comm_ptr->errhandler == NULL || comm_ptr->errhandler->handle == MPI_ERRORS_ARE_FATAL)
    {
	if (MPIR_Err_print_stack_flag)
	{
	    fprintf( stderr, "Fatal error (code 0x%08x) in %s():\n", errcode, fcname);
	    MPIR_Err_print_stack(stderr, errcode);
	}
	else
	{
	    /* The default handler should try the following: Provide the rank in comm_world.  If the process is not in comm world,
	       use something else.  If the communicator exists and has a name, provide that name */
	    char msg[MPI_MAX_ERROR_STRING];
	
	    MPIR_Err_get_string( errcode, msg );
	    fprintf( stderr, "Fatal error (code 0x%08x) in %s(): %s\n", errcode, fcname, msg);
	}
	
	MPID_Abort(comm_ptr, MPI_SUCCESS, 13);
    }

    if (comm_ptr->errhandler->handle == MPI_ERRORS_RETURN)
    {
	return errcode;
    }
    else
    {
	/* We pass a final 0 (for a null pointer) to these routines
	   because MPICH-1 expected that */
	switch (comm_ptr->errhandler->language) {
	case MPID_LANG_C:
#ifdef HAVE_CXX_BINDING
	case MPID_LANG_CXX:
#endif
	    (*comm_ptr->errhandler->errfn.C_Comm_Handler_function)( 
		&comm_ptr->handle, &errcode, 0 );
	    break;
#ifdef HAVE_FORTRAN_BINDING
	case MPID_LANG_FORTRAN90:
	case MPID_LANG_FORTRAN:
	    (*comm_ptr->errhandler->errfn.F77_Handler_function)( 
		(MPI_Fint *)&comm_ptr->handle, &errcode, 0 );
	    break;
#endif
	}
    }
    return errcode;
}

/* 
 * MPI routines that detect errors on window objects use this to report errors
 */
int MPIR_Err_return_win( MPID_Win  *win_ptr, const char fcname[], 
			  int errcode )
{
    const int class = ERROR_GET_CLASS(errcode);
    
    if (class > MPICH_ERR_LAST_CLASS)
    {
	if (errcode & ~ERROR_CLASS_MASK)
	{
	    MPIU_Error_printf("INTERNAL ERROR: Invalid error class (%d) encountered while returning from\n%s().  "
			      "Please file a bug report.  The error stack follows:\n", class);
	    MPIR_Err_print_stack(stderr, errcode);
	}
	else
	{
	    MPIU_Error_printf("INTERNAL ERROR: Invalid error class (%d) encountered while returning from\n%s().  "
			      "Please file a bug report.  No error stack is available.\n");
	}
	
	errcode = (errcode & ~ERROR_CLASS_MASK) | MPI_ERR_UNKNOWN;
    }
    
    /* First, check the nesting level */
    if (MPIR_Nest_value()) return errcode;

    /* Now, invoke the error handler for the window */
    if (win_ptr && win_ptr->errhandler) {
	switch (win_ptr->errhandler->language) {
	case MPID_LANG_C:
#ifdef HAVE_CXX_BINDING
	case MPID_LANG_CXX:
#endif
	    (*win_ptr->errhandler->errfn.C_Win_Handler_function)( 
		&win_ptr->handle, &errcode );
	    break;
#ifdef HAVE_FORTRAN_BINDING
	case MPID_LANG_FORTRAN90:
	case MPID_LANG_FORTRAN:
	    (*win_ptr->errhandler->errfn.F77_Handler_function)( 
		(MPI_Fint *)&win_ptr->handle, &errcode );
	    break;
#endif
	}
    }
    else
    {
	MPID_Comm * comm_ptr;

	if (win_ptr != NULL)
	{
	    MPID_Comm_get_ptr(win_ptr->comm, comm_ptr);
	}
	else
	{
	    comm_ptr = NULL;
	}
	
	/* No window, so errors are fatal */
	fprintf( stderr, "Fatal error %d in %s\n", errcode, fcname );
	MPID_Abort(comm_ptr, MPI_SUCCESS, 13);
    }
    return errcode;
}

/* 
 * MPI routines that detect errors on files use this to report errors 
 */
#if 0
int MPIR_Err_return_file( MPID_File  *file_ptr, const char fcname[], 
			  int errcode )
{
    const int class = ERROR_GET_CLASS(errcode);
    
    if (class > MPICH_ERR_LAST_CLASS)
    {
	if (errcode & ~ERROR_CLASS_MASK)
	{
	    MPIU_Error_printf("INTERNAL ERROR: Invalid error class (%d) encountered while returning from\n%s().  "
			      "Please file a bug report.  The error stack follows:\n", class);
	    MPIR_Err_print_stack(stderr, errcode);
	}
	else
	{
	    MPIU_Error_printf("INTERNAL ERROR: Invalid error class (%d) encountered while returning from\n%s().  "
			      "Please file a bug report.  No error stack is available.\n");
	}
	
	errcode = (errcode & ~ERROR_CLASS_MASK) | MPI_ERR_UNKNOWN;
    }
    
    /* First, check the nesting level */
    if (MPIR_Nest_value()) return errcode;

    /* Now, invoke the error handler for the file */
    if (file_ptr && file_ptr->errhandler) {
    /* Now, invoke the error handler for the communicator */
        if (file_ptr->errhandler->handle == MPI_ERRORS_ARE_FATAL) {
	    /* Try to print the associated message */
	    const char *p = MPIR_Err_get_string( errcode );
	    
	    /* The default handler should try the following:
	       Provide the rank in comm_world.  If the process is not
	       in comm world, use something else.  If the communicator
	       exists and has a name, provide that name */
	    if (p) {
		fprintf( stderr, "Fatal error: %s in %s\n", p, fcname );
	    }
	    else
	    {
		fprintf( stderr, "Fatal error (code %d) in %s\n", errcode, fcname );
	    }
	    MPID_Abort(comm_ptr, MPI_SUCCESS, 13);
	}
        else if (file_ptr->errhandler->handle == MPI_ERRORS_RETURN) {
	    return errcode;
	}
	else {
	    /* Invoke the provided error handler */
	    switch (file_ptr->errhandler->language) {
	    case MPID_LANG_C:
#ifdef HAVE_CXX_BINDING
	    case MPID_LANG_CXX:
#endif
		(*file_ptr->errhandler->errfn.C_File_Handler_function)( 
		(MPI_File *)&file_ptr->handle, &errcode );
		break;
#ifdef HAVE_FORTRAN_BINDING
	    case MPID_LANG_FORTRAN90:
	    case MPID_LANG_FORTRAN:
		(*file_ptr->errhandler->errfn.F77_Handler_function)( 
		    (MPI_Fint *)&file_ptr->handle, &errcode );
		break;
#endif
	    }
	}
    }
    else {
	/* No file, so errors return */
	return errcode;
    }
    return errcode;
}
#endif

#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_CLASS
/* Given a message string abbreviation (e.g., one that starts "**"), return the corresponding index.  For the generic (non
 * parameterized messages), use idx = FindGenericMsgIndex( "**msg" );
 */
static int FindGenericMsgIndex( const char *msg )
{
    int i, c;
    for (i=0; i<generic_msgs_len; i++) {
	c = strcmp( generic_err_msgs[i].short_name, msg );
	if (c == 0) return i;
	if (c > 0) return -1;
    }
    return -1;
}
/* Here is an alternate search routine based on bisection.
   int i_low, i_mid, i_high, c;
   i_low = 0; i_high = generic_msg_len - 1;
   while (i_high - i_low >= 0) {
       i_mid = (i_high + i_low) / 2;
       c = strcmp( generic_err_msgs[i].short_name, msg );
       if (c == 0) return i_mid;
       if (c < 0) { i_low = i_mid + 1; }
       else       { i_high = i_mid - 1; }
   }
   return -1;
*/
#endif

#if MPICH_ERROR_MSG_LEVEL >= MPICH_ERROR_MSG_ALL
/* Given a message string abbreviation (e.g., one that starts "**"), return the corresponding index.  For the specific
 * (parameterized messages), use idx = FindGenericMsgIndex( "**msg" );
 */
static int FindSpecificMsgIndex( const char *msg )
{
    int i, c;
    for (i=0; i<specific_msgs_len; i++) {
	c = strcmp( specific_err_msgs[i].short_name, msg );
	if (c == 0) return i;
	if (c > 0) return -1;
    }
    return -1;
}
#endif

int MPIR_Err_is_fatal(int errcode)
{
    return (errcode & ERROR_FATAL_MASK) ? TRUE : FALSE;
}

int MPIR_Err_create_code( int lastcode, int fatal, const char fcname[], int line, int class, const char generic_msg[],
			  const char specific_msg[], ... )
{
    va_list Argp;
    int err_code;
    int generic_idx;
    /* Create the code from the class and the message ring index */

    if (MPIR_Err_abort_on_error)
    {
	/*printf("aborting from %s, line %d\n", fcname, line);fflush(stdout);*/
	abort();
    }

    va_start(Argp, specific_msg);

    err_code = class;

    /* Handle the generic message.  This selects a subclass, based on a text string */
#   if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_CLASS
    {
	generic_idx = FindGenericMsgIndex(generic_msg);
	if (generic_idx >= 0)
	{
	    err_code |= (generic_idx + 1) << ERROR_GENERIC_SHIFT;
	}
	else
	{
	    /* TODO: lookup index for class error message */
	    err_code &= ~ERROR_GENERIC_MASK;
	    
#           ifdef DEBUG
	    {
		if (generic_msg[0] == '*' && generic_msg[1] == '*')
		{
		    /* Internal error.  Generate some debugging information; FIXME for the general release */
		    fprintf( stderr, "Could not find %s in list of messages\n", generic_msg );
		}
	    }
#           endif
	}
    }
#   endif

    /* Handle the instance-specific part of the error message */
#   if MPICH_ERROR_MSG_LEVEL >= MPICH_ERROR_MSG_ALL
    {
	int specific_idx;
	const char * specific_fmt = 0;
	int  ring_idx, ring_seq=0;
	char * ring_msg;
	int i;
	
	MPID_Thread_lock(&error_ring_mutex);
	{
	    ring_idx = error_ring_loc++;
	    if (error_ring_loc >= MAX_ERROR_RING) error_ring_loc %= MAX_ERROR_RING;
	
	    ring_msg = ErrorRing[ring_idx].msg;

	    if (specific_msg != NULL)
	    {
		specific_idx = FindSpecificMsgIndex(specific_msg);
		if (specific_idx >= 0)
		{
		    specific_fmt = specific_err_msgs[specific_idx].long_name;
		}
		else
		{
		    specific_fmt = specific_msg;
		}

#               ifdef HAVE_VSNPRINTF
		{
		    vsnprintf( ring_msg, MPI_MAX_ERROR_STRING, specific_fmt, Argp );
		}
#               elif defined(HAVE_VSPRINTF)
		{
		    vsprintf( ring_msg, specific_fmt, Argp );
		}
#               else
		{
		    /* For now, just punt */
		    if (generic_idx >= 0)
		    {
			MPIU_Strncpy( ring_msg, generic_err_msgs[generic_idx].long_name, MPI_MAX_ERROR_STRING );
		    }
		    else
		    {
			MPIU_Strncpy( ring_msg, generic_msg, MPI_MAX_ERROR_STRING );
		    }
		}
#               endif
	    }
	    else if (generic_idx >= 0)
	    {
		MPIU_Strncpy( ring_msg, generic_err_msgs[generic_idx].long_name, MPI_MAX_ERROR_STRING );
	    }
	    else
	    {
		MPIU_Strncpy( ring_msg, generic_msg, MPI_MAX_ERROR_STRING );
	    }

	    ring_msg[MPI_MAX_ERROR_STRING] = '\0';
	
	    /* Create a simple hash function of the message to serve as the sequence number */
	    ring_seq = 0;
	    for (i=0; ring_msg[i]; i++)
	    {
		ring_seq += (unsigned int) ring_msg[i];
	    }
	    ring_seq %= ERROR_SPECIFIC_SEQ_SIZE;
	    
	    ErrorRing[ring_idx].id = class & ERROR_CLASS_MASK;
	    ErrorRing[ring_idx].id |= (generic_idx + 1) << ERROR_GENERIC_SHIFT;
	    ErrorRing[ring_idx].id |= ring_seq << ERROR_SPECIFIC_SEQ_SHIFT;
	    ErrorRing[ring_idx].prev_error = lastcode;
	    if (fcname != NULL)
	    {
		MPIU_Snprintf(ErrorRing[ring_idx].fcname, MAX_FCNAME_LEN, "%s(%d)", fcname, line);
		ErrorRing[ring_idx].fcname[MAX_FCNAME_LEN] = '\0';
	    }
	    else
	    {
		ErrorRing[ring_idx].fcname[0] = '\0';
	    }
	}
	MPID_Thread_unlock(&error_ring_mutex);
	
	err_code |= ring_idx << ERROR_SPECIFIC_INDEX_SHIFT;
	err_code |= ring_seq << ERROR_SPECIFIC_SEQ_SHIFT;

    }
#   endif

    if (fatal || MPIR_Err_is_fatal(lastcode))
    {
	err_code |= ERROR_FATAL_MASK;
    }
    
    va_end( Argp );

    return err_code;
}

/*
 * Accessor routines for the predefined messages.  These can be
 * used by the other routines (such as MPI_Error_string) to
 * access the messages in this file, or the messages that may be
 * available through any message catalog facility 
 */
static const char *get_class_msg( int class )
{
#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_NONE
    if (class >= 0 && class < MPIR_MAX_ERROR_CLASS_INDEX) {
	return generic_err_msgs[class_to_index[class]].long_name;
    }
    else {
	return "Unknown error class";
    }
#else 
    return "Error message texts are not available";
#endif
}

void MPIR_Err_get_string( int errorcode, char * msg )
{
    /* Convert the code to a string.  The cases are:
       simple class.  Find the corresponding string.
       <not done>
       if (user code) { go to code that extracts user error messages }
       else {
           is specific message code set and available?  if so, use it
	   else use generic code (lookup index in table of messages)
       }
     */
    if (errorcode & ERROR_DYN_MASK)
    {
	/* This is a dynamically created error code (e.g., with MPI_Err_add_class) */
	if (!MPIR_Process.errcode_to_string)
	{
	    if (MPIU_Strncpy(msg, "Undefined dynamic error code", MPI_MAX_ERROR_STRING))
	    {
		msg[MPI_MAX_ERROR_STRING - 1] = '\0';
	    }
	    
	}
	else
	{
	    if (MPIU_Strncpy(msg, MPIR_Process.errcode_to_string( errorcode ), MPI_MAX_ERROR_STRING))
	    {
		msg[MPI_MAX_ERROR_STRING - 1] = '\0';
	    }
	}
    }
    else if ( (errorcode & ERROR_CLASS_MASK) == errorcode) {
	/* code is a raw error class.  Convert the class to an index */
	if (MPIU_Strncpy(msg, get_class_msg( errorcode ), MPI_MAX_ERROR_STRING))
	{
	    msg[MPI_MAX_ERROR_STRING - 1] = '\0';
	}
    }
    else
    {
	/* error code encodes a message.  Find it and make sure that
	   it is still valid (seq number matches the stored value in the
	   error message ring).  If the seq number is *not* valid,
	   use the generic message.
	 */
	int class;
	int ring_idx;
	int ring_seq;
	int generic_idx;

	ring_idx    = (errorcode & ERROR_SPECIFIC_INDEX_MASK) >> ERROR_SPECIFIC_INDEX_SHIFT;
	ring_seq    = (errorcode & ERROR_SPECIFIC_SEQ_MASK) >> ERROR_SPECIFIC_SEQ_SHIFT;
	generic_idx = ((errorcode & ERROR_GENERIC_MASK) >> ERROR_GENERIC_SHIFT) - 1;

#       if MPICH_ERROR_MSG_LEVEL >= MPICH_ERROR_MSG_ALL
	{
	    if (generic_idx >= 0)
	    {
		int flag = FALSE;

		MPID_Thread_lock(&error_ring_mutex);
		{
		    int ring_id;

		    ring_id = errorcode & (ERROR_CLASS_MASK | ERROR_GENERIC_MASK | ERROR_SPECIFIC_SEQ_MASK);

		    if (ErrorRing[ring_idx].id == ring_id)
		    {
			if (MPIU_Strncpy(msg, ErrorRing[ring_idx].msg, MPI_MAX_ERROR_STRING))
			{
			    msg[MPI_MAX_ERROR_STRING - 1] = '\0';
			}
			flag = TRUE;
		    }
		}
		MPID_Thread_unlock(&error_ring_mutex);

		if (flag)
		{
		    goto fn_exit;
		}
	    }
	}
#       endif
	
#       if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_NONE
	{
	    if (generic_idx >= 0)
	    {
		if (MPIU_Strncpy(msg, generic_err_msgs[generic_idx].long_name, MPI_MAX_ERROR_STRING))
		{
		    msg[MPI_MAX_ERROR_STRING - 1] = '\0';
		}
		goto fn_exit;
	    }
	}
#       endif

	class = ERROR_GET_CLASS(errorcode);

	if (class <= MPICH_ERR_LAST_CLASS)
	{
	    if (MPIU_Strncpy(msg, get_class_msg(class), MPI_MAX_ERROR_STRING))
	    {
		msg[MPI_MAX_ERROR_STRING - 1] = '\0';
	    }
	}
	else
	{
	    MPIU_Snprintf(msg, MPI_MAX_ERROR_STRING, "Error code contains an invalid class (%d)\n", class);
	}
    }

fn_exit:
    return;
}


void MPIR_Err_print_stack(FILE * fp, int errcode)
{
#   if MPICH_ERROR_MSG_LEVEL >= MPICH_ERROR_MSG_ALL
    {
	MPID_Thread_lock(&error_ring_mutex);
	{
	    while (errcode != MPI_SUCCESS)
	    {
		int ring_idx;
		int ring_id;
		int generic_idx;

		ring_idx    = (errcode & ERROR_SPECIFIC_INDEX_MASK) >> ERROR_SPECIFIC_INDEX_SHIFT;
		ring_id = errcode & (ERROR_CLASS_MASK | ERROR_GENERIC_MASK | ERROR_SPECIFIC_SEQ_MASK);
		generic_idx = ((errcode & ERROR_GENERIC_MASK) >> ERROR_GENERIC_SHIFT) - 1;

		if (generic_idx < 0)
		{
		    break;
		}
		    
		if (ErrorRing[ring_idx].id == ring_id)
		{
		    fprintf(fp, "%s: %s\n", ErrorRing[ring_idx].fcname, ErrorRing[ring_idx].msg);
		    errcode = ErrorRing[ring_idx].prev_error;
		}
		else
		{
		    break;
		}
	    }
	}
	MPID_Thread_unlock(&error_ring_mutex);

	if (errcode == MPI_SUCCESS)
	{
	    goto fn_exit;
	}
    }
#   endif

#   if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_NONE
    {
	int generic_idx;
		    
	generic_idx = ((errcode & ERROR_GENERIC_MASK) >> ERROR_GENERIC_SHIFT) - 1;
	
	if (generic_idx >= 0)
	{
	    fprintf(fp, "(unknown)(): %s\n", generic_err_msgs[generic_idx].long_name);
	    goto fn_exit;
	}
    }
#   endif
    
    {
	int class;

	class = ERROR_GET_CLASS(errcode);
	
	if (class <= MPICH_ERR_LAST_CLASS)
	{
	    fprintf(fp, "(unknown)(): %s\n", get_class_msg(ERROR_GET_CLASS(errcode)));
	}
	else
	{
	    fprintf(fp, "Error code contains an invalid class (%d)\n", class);
	}
    }
    
  fn_exit:
    return;
}


/* 
   Nesting level for routines.
   Note that since these use per-thread data, no locks or atomic update
   routines are required.

   In a single-threaded environment, These are replaced with
   MPIR_Thread.nest_count ++, --.  These are defined in the mpiimpl.h file.
 */
#ifndef MPICH_SINGLE_THREADED
void MPIR_Nest_incr( void )
{
    MPICH_PerThread_t *p;
    MPID_GetPerThread(p);
    p->nest_count++;
}
void MPIR_Nest_decr( void )
{
    MPICH_PerThread_t *p;
    MPID_GetPerThread(p);
    p->nest_count--;
}
int MPIR_Nest_value( void )
{
    MPICH_PerThread_t *p;
    MPID_GetPerThread(p);
    return p->nest_count;
}
#endif

/*
 * Error handlers.  These are handled just like the other opaque objects
 * in MPICH
 */

#ifndef MPID_ERRHANDLER_PREALLOC 
#define MPID_ERRHANDLER_PREALLOC 8
#endif

/* Preallocated errorhandler objects */
MPID_Errhandler MPID_Errhandler_builtin[2] = 
          { { MPI_ERRORS_ARE_FATAL, 0},
	    { MPI_ERRORS_RETURN, 0} }; 
MPID_Errhandler MPID_Errhandler_direct[MPID_ERRHANDLER_PREALLOC];
MPIU_Object_alloc_t MPID_Errhandler_mem = { 0, 0, 0, 0, MPID_ERRHANDLER, 
					    sizeof(MPID_Errhandler), 
					    MPID_Errhandler_direct,
					    MPID_ERRHANDLER_PREALLOC, };

void MPID_Errhandler_free(MPID_Errhandler *errhan_ptr)
{
    MPIU_Handle_obj_free(&MPID_Errhandler_mem, errhan_ptr);
}
