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
/* BUG - NOT YET CALLED ANYWHERE */
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

    /* Now, invoke the error handler for the communicator */
    if (comm_ptr && comm_ptr->errhandler) {
	switch (comm_ptr->errhandler->language) {
	case MPID_LANG_C:
	case MPID_LANG_CXX:
	    (*comm_ptr->errhandler->errfn.C_Comm_Handler_function)( 
		&comm_ptr->handle, &errcode );
	    break;
	case MPID_LANG_FORTRAN90:
	case MPID_LANG_FORTRAN:
	    (*comm_ptr->errhandler->errfn.F77_Handler_function)( 
		(MPI_Fint *)&comm_ptr->handle, &errcode );
	    break;
	}
    }
    else {
	/* No communicator, so errors are fatal */
	fprintf( stderr, "Fatal error %d in %s\n", errcode, fcname );
	exit(1); /* Change this to MPID_Abort */
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
	case MPID_LANG_CXX:
	    (*win_ptr->errhandler->errfn.C_Comm_Handler_function)( 
		&win_ptr->handle, &errcode );
	    break;
	case MPID_LANG_FORTRAN90:
	case MPID_LANG_FORTRAN:
	    (*win_ptr->errhandler->errfn.F77_Handler_function)( 
		(MPI_Fint *)&win_ptr->handle, &errcode );
	    break;
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
	case MPID_LANG_CXX:
	    (*file_ptr->errhandler->errfn.C_Comm_Handler_function)( 
		&file_ptr->handle, &errcode );
	    break;
	case MPID_LANG_FORTRAN90:
	case MPID_LANG_FORTRAN:
	    (*file_ptr->errhandler->errfn.F77_Handler_function)( 
		(MPI_Fint *)&file_ptr->handle, &errcode );
	    break;
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
/* Given a generic message string, return the corresponding index */
static int FindMsgIndex( const char *msg )
{
    int i, c;
    for (i=0; i<generic_msgs_len; i++) {
	c = strcmp( generic_msg[i].short_name, msg );
	if (c == 0) return i;
	if (c < 0) return -1;
    }
    return -1;
}
/* Here is an alternate search routine based on bisection.
   int i_low, i_mid, i_high, c;
   i_low = 0; i_high = generic_msg_len - 1;
   while (i_high - i_low >= 0) {
       i_mid = (i_high + i_low) / 2;
       c = strcmp( generic_msg[i].short_name, msg );
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
    /* Create the code from the class and the message ring index */

    va_start( Argp, def_string );

    err_code = class;

    /* Handle the generic message.  This selects a subclass, based on a 
       text string */
#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_CLASS
    {
	int specific_idx = FindMsgIndex( def_string );
	if (specific_idx >= 0)
	    err_code |= specific_idx << ERROR_GENERIC_SHIFT;
    }
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
	    /* An instance string is available.  Use it */
#ifdef HAVE_VSNPRINTF
	    vsnprintf( str, MPI_MAX_ERROR_STRING, inst_string, Argp );
#elif defined(HAVE_VSPRINTF)
	    vsprintf( str, inst_string, Argp );
#else
	    /* For now, just punt */
	    strncpy( str, def_string, MPI_MAX_ERROR_STRING );
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
	else {
	    strncpy( str, def_string, MPI_MAX_ERROR_STRING );
	}
	err_code |= (ring_idx << ERROR_SPECIFIC_INDEX_SHIFT);
	err_code |= (ring_seq << ERROR_SPECIFIC_SEQ_SHIFT);
    }
#endif

    va_end( Argp );

    return err_code;
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
