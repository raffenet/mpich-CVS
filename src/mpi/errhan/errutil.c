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
#include "defmsg.h" 

/* stdarg is required to handle the variable argument lists for 
   MPIR_Err_create_code */
#include <stdarg.h>

/* stdio is needed for vsprintf and vsnprintf */
#include <stdio.h>

/*
 * Instance-specific error messages are stored in a ring.
 * Messages are written into the error_ring; the corresponding entry in
 * error_ring_seq is used to keep a sort of "checkvalue" to ensure that the
 * error code that points at this message is in fact for this particular 
 * message.  This is used to handle the unlikely but possible situation where 
 * so many error messages are generated that the ring is overlapped.
 */
#if MPICH_ERROR_MSG_LEVEL >= MPICH_ERROR_MSG_ALL
#define MAX_ERROR_RING 32
#define MAX_ERROR_BIGRING 8192

static char error_ring[MAX_ERROR_RING][MPI_MAX_ERROR_STRING+1];
static int error_ring_seq[MAX_ERROR_RING];
static volatile unsigned int error_ring_loc = 0;
#endif

/* Special error handler to call if we are not yet initialized */
/* FIXME - NOT YET CALLED ANYWHERE */
void MPIR_Err_preinit( void )
{
    fprintf( stderr, "Error encountered before initializing MPICH\n" );
}


/* void for now until error handlers are defined */
int MPIR_Err_return_comm( MPID_Comm  *comm_ptr, const char fcname[], 
			  int errcode )
{
    /* First, check the nesting level */
    if (MPIR_Nest_value()) return errcode;
    
    if (!comm_ptr) {
	/* Try to replace with the default handler, which is the one
	   on MPI_COMM_WORLD.  This gives us correct behavior
	   for the case where the error handler on MPI_COMM_WORLD
	   has been changed. */
	if (MPIR_Process.comm_world) {
	    comm_ptr = MPIR_Process.comm_world;
	}
    }

    /* Now, invoke the error handler for the communicator */
    if (!comm_ptr || !(comm_ptr->errhandler) || 
	comm_ptr->errhandler->handle == MPI_ERRORS_ARE_FATAL) {
	/* Either no communicator, error handler, so errors are fatal,
	   or an explicit selection of the fatal error handler */
	/* Try to print the associated message */
	const char *p = MPIR_Err_get_string( errcode );
	
	if (p) {
	    fprintf( stderr, "Fatal error: %s in %s\n", p, fcname );
	}
	else
	{
	    fprintf( stderr, "Fatal error (code %d) in %s\n", errcode, fcname );
	}
	exit(1); /* Change this to MPID_Abort */
    }
    else if (comm_ptr->errhandler->handle == MPI_ERRORS_RETURN) {
	return errcode;
    }
    else {
	switch (comm_ptr->errhandler->language) {
	case MPID_LANG_C:
#ifdef HAVE_CXX_BINDING
	case MPID_LANG_CXX:
#endif
	    (*comm_ptr->errhandler->errfn.C_Comm_Handler_function)( 
		&comm_ptr->handle, &errcode );
	    break;
#ifdef HAVE_FORTRAN_BINDING
	case MPID_LANG_FORTRAN90:
	case MPID_LANG_FORTRAN:
	    (*comm_ptr->errhandler->errfn.F77_Handler_function)( 
		(MPI_Fint *)&comm_ptr->handle, &errcode );
	    break;
#endif
	}
    }
    return errcode;
}

int MPIR_Err_return_win( MPID_Win  *win_ptr, const char fcname[], 
			  int errcode )
{
    /* First, check the nesting level */
    if (MPIR_Nest_value()) return errcode;

    /* Now, invoke the error handler for the communicator */
    if (win_ptr && win_ptr->errhandler) {
	switch (win_ptr->errhandler->language) {
	case MPID_LANG_C:
#ifdef HAVE_CXX_BINDING
	case MPID_LANG_CXX:
#endif
	    (*win_ptr->errhandler->errfn.C_Comm_Handler_function)( 
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
    else {
	/* No communicator, so errors are fatal */
	fprintf( stderr, "Fatal error %d in %s\n", errcode, fcname );
	exit(1); /* Change this to MPID_Abort */
    }
    return errcode;
}

int MPIR_Err_return_file( MPID_File  *file_ptr, const char fcname[], 
			  int errcode )
{
    /* First, check the nesting level */
    if (MPIR_Nest_value()) return errcode;

    /* Now, invoke the error handler for the communicator */
    if (file_ptr && file_ptr->errhandler) {
	switch (file_ptr->errhandler->language) {
	case MPID_LANG_C:
#ifdef HAVE_CXX_BINDING
	case MPID_LANG_CXX:
#endif
	    (*file_ptr->errhandler->errfn.C_Comm_Handler_function)( 
		&file_ptr->handle, &errcode );
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
    else {
	/* No communicator, so errors are fatal */
	fprintf( stderr, "Fatal error %d in %s\n", errcode, fcname );
	exit(1); /* Change this to MPID_Abort */
    }
    return errcode;
}

#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_CLASS
/* Given a message string abbreviation (e.g., one that starts "**"), 
   return the corresponding index.  For the generic (non parameterized
   messages), use 
   idx =     FindMsgIndex( "**msg" );
   The instance-specific message should have the same idx value
*/
static int FindMsgIndex( const char *msg )
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

int MPIR_Err_create_code( int class, const char def_string[], ... )
{
    va_list Argp;
    int err_code;
    int specific_idx = -1;
    /* Create the code from the class and the message ring index */

    va_start( Argp, def_string );

    err_code = class;

    /* Handle the generic message.  This selects a subclass, based on a 
       text string */
#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_CLASS
    specific_idx = FindMsgIndex( def_string );
    if (specific_idx >= 0)
	err_code |= specific_idx << ERROR_GENERIC_SHIFT;
#endif

    /* Handle the instance-specific part of the error message */
#if MPICH_ERROR_MSG_LEVEL >= MPICH_ERROR_MSG_ALL
    {
	const char *inst_string = 0;
	int  ring_idx, ring_seq=0;
	char *str;
	
	/* THIS NEEDS TO BE ATOMIC AND RELIABLE */
	ring_idx = error_ring_loc++;
	
	if (ring_idx >= MAX_ERROR_RING) ring_idx %= MAX_ERROR_RING;
	str = error_ring[ring_idx];

	inst_string = va_arg( Argp, const char * );
	if (inst_string) {
	    int i;

	    if (specific_idx >=0 && 
		specific_err_msgs[specific_idx].short_name &&
		strcmp( inst_string, 
			specific_err_msgs[specific_idx].short_name)==0) {
		/* If this name is in the lookup table, use the 
		   replacement */
		inst_string = specific_err_msgs[specific_idx].long_name;
	    }
	    /* An instance string is available.  Use it */
#ifdef HAVE_VSNPRINTF
	    vsnprintf( str, MPI_MAX_ERROR_STRING, inst_string, Argp );
#elif defined(HAVE_VSPRINTF)
	    vsprintf( str, inst_string, Argp );
#else
	    /* For now, just punt */
	    MPIU_Strncpy( str, def_string, MPI_MAX_ERROR_STRING );
#endif

	    /* Create a simple hash function of the message to serve as
	       the sequence number */
	    ring_seq = 0;
	    for (i=0; str[i]; i++) {
		ring_seq += (unsigned int)str[i];
	    }
	    ring_seq %= ERROR_SPECIFIC_SEQ_SIZE;
	    error_ring_seq[ring_idx] = ring_seq;
	}
	else if (specific_idx >= 0) {
	    MPIU_Strncpy( str, generic_err_msgs[specific_idx].long_name, 
			  MPI_MAX_ERROR_STRING );
	}
	else {
	    MPIU_Strncpy( str, def_string, MPI_MAX_ERROR_STRING );
	}
	err_code |= (ring_idx << ERROR_SPECIFIC_INDEX_SHIFT);
	err_code |= (ring_seq << ERROR_SPECIFIC_SEQ_SHIFT);
    }
#endif

    va_end( Argp );

    return err_code;
}

/*
 * Accessor routines for the predefined messages.  These can be
 * used by the other routines (such as MPI_Error_string) to
 * access the messages in this file, or the messages that may be
 * available through any message catalog facility 
 */
const char *MPIR_Err_get_generic_string( int class )
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

const char *MPIR_Err_get_string( int errorcode )
{
    const char *p;

    /* Convert the code to a string.  The cases are:
       simple class.  Find the corresponding string.
       <not done>
       if (user code) { go to code that extracts user error messages }
       else {
           is specific message code set and available?  if so, use it
	   else use generic code (lookup index in table of messages)
       }
     */
    if (errorcode & ERROR_DYN_MASK) {
	/* This is a dynamically created error code (e.g., with
	   MPI_Err_add_class) */
	if (!MPIR_Process.errcode_to_string) {
	    p = "Undefined dynamic error code";
	}
	else {
	    p = MPIR_Process.errcode_to_string( errorcode );
	}
    }
    else if ( (errorcode & ERROR_CLASS_MASK) == errorcode) {
	/* code is a raw error class.  Convert the class to an index */
	p = MPIR_Err_get_generic_string( errorcode );
    }
    else {
	/* error code encodes a message.  Find it and make sure that
	   it is still valid (seq number matches the stored value in the
	   error message ring).  If the seq number is *not* valid,
	   use the generic message.
	 */
	int ring_idx;
	int ring_seq;
	int generic_idx;

	ring_idx    = (errorcode & ERROR_SPECIFIC_INDEX_MASK) >> ERROR_SPECIFIC_INDEX_SHIFT;
	ring_seq    = (errorcode & ERROR_SPECIFIC_SEQ_MASK) >> ERROR_SPECIFIC_SEQ_SHIFT;
	generic_idx = (errorcode & ERROR_GENERIC_MASK) >> ERROR_GENERIC_SHIFT;

#if MPICH_ERROR_MSG_LEVEL >= MPICH_ERROR_MSG_ALL
	if (error_ring_seq[ring_idx] == ring_seq) {
	    p = error_ring[ring_idx];
	}
	else if (generic_idx > 0) {
	    p = generic_err_msgs[generic_idx].long_name;
	}
	else
#endif
	{
	    p = MPIR_Err_get_generic_string( ERROR_GET_CLASS(errorcode) );
	}
    }
    return p;
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

#ifdef FOO
/*
 * Should we have a simple set code, call error handler for the nomem case?
 * If we do this, we need to modify the coding check to accept this as
 * an alternative to the MPID_MPI_FUNC_EXIT call.
 *
 * Unfortunately, we can't do this without the timestamp from the state call.
 * Do we want to pass that in as well?
 */
int MPIR_Err_comm_nomem( MPID_Comm *comm_ptr, const char fcname[], 
			 int stateid )
{
    int mpi_errno;

    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
    MPID_MPI_FUNC_EXIT(stateid);
    return MPIR_Err_return_comm( 0, fcname, mpi_errno );
}
#endif
