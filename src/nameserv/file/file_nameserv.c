/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This file contains a simple implementation of the name server routines,
 * using a file written to a shared file system to communication the
 * data.  
 */
#include <stdio.h>

#include "mpiimpl.h"

/* Define the name service handle */
#define MPID_MAX_NAMEPUB 64
struct MPID_NS_Handle { 
    int nactive;                         /* Number of active files */
    int mypid;                           /* My process id */
    char dirname[MAXPATHLEN];            /* Directory for all files */
    char *(filenames[MPID_MAX_NAMEPUB]); /* All created files */
};
typedef struct MPID_NS_Handle *MPID_NS_Handle;

/* Create a structure that we will use to remember files created for
   publishing.  */
#undef FUNCNAME
#define FUNCNAME MPID_NS_Create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_NS_Create( const MPID_Info *info_ptr, MPID_NS_Handle *handle_ptr )
{
    int  i;
    const char *dirname;
    *handle_ptr = (MPID_NS_Handle)MPIU_Malloc( sizeof(struct MPID_NS_Handle) );
    if (!*handle_ptr) {
	err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
	return err;
    }
    handle_ptr->nactive = 0;
    handle_ptr->mypid   = getpid();

    /* Get the dirname.  Currently, use HOME, but could use 
       an info value of NAMEPUB_CONTACT */
    dirname = getenv( "HOME" );
    if (!dirname) {
	dirname = ".";
    }
    MPIU_Strcpy( handle_ptr->dirname, dirname, MAXPATHLEN );
    MPIU_Strapp( handle_ptr->dirname, "/.mpinamepub/", MAXPATHLEN );

    /* Make the directory if necessary */
    
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Publish
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_NS_Publish( MPID_NS_Handle handle, const MPID_Info *info_ptr, 
                     const char service_name[], const char port[] )
{
    FILE *fp;
    char filename[MAXPATHLEN];
    int  err;

    /* Determine file and directory name.  The file name is from
       the service name */
    MPIU_Strncpy( filename, handle->dirname, MAXPATHLEN );
    MPIU_Strapp( filename, service_name, MAXPATHLEN );

    /* Add the file name to the known files now, in case there is 
       a failure during open or writing */
    if (handle.nactive < MPID_MAX_NAMEPUB) {
	handle.filenames[handle.nactive++] = MPIU_Strdup( filename );
    }
    else {
	err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
	return err;
    }

    /* Now, open the file and write out the port name */
    fp = fopen( filename, "w" );
    if (!fp) {
	err = MPIR_Err_create_code( 
	    MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
	    MPI_ERR_OTHER, "**namepublish",
	    "**namepublish %s", service_name );
	return err;
    }
    /* Should also add date? */
    fprintf( fp, "%s\n%d\n", port, handle.mypid );
    fclose( fp );

    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Lookup
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_NS_Lookup( MPID_NS_Handle handle, const MPID_Info *info_ptr,
                    const char service_name[], char port[] )
{
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Unpublish
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_NS_Unpublish( MPID_NS_Handle handle, const MPID_Info *info_ptr, 
                       const char service_name[] )
{
    char filename[MAXPATHLEN];
    int  err;
    int  i;

    /* Remove the file corresponding to the service name */
    /* Determine file and directory name.  The file name is from
       the service name */
    MPIU_Strncpy( filename, handle->dirname, MAXPATHLEN );
    MPIU_Strapp( filename, service_name, MAXPATHLEN );

    /* Find the filename from the list of published files */
    for (i=0; i<handle->nactive; i++) {
	if (strcmp( filename, handle->filenames[i] ) == 0) {
	    /* unlink the file only if we find it */
	    unlink( filename );
	    MPIU_Free( handle->filenames[i] );
	    handle->filenames[i] = 0;
	    break;
	}
    }

    if (i == handle->nactive) {
	/* Error: this name was not found */
	err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
				    MPI_ERR_OTHER, "**namepubnotpub",
				    "**namepubnotpub %s", service_name );
	return err;
    }

    /* Later, we can reduce the number of active and compress the list */

    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_NS_Free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_NS_Free( MPID_NS_Handle *handle_ptr )
{
    int i;
    MPID_NS_Handle handle = *handle_ptr;
    
    for (i=0; i<handle->nactive; i++) {
	if (handle->filenames[i]) {
	    /* Remove the file if it still exists */
	    unlink( handle->filenames[i] );
	    MPIU_Free( handle->filenames[i] );
	}
    }
    MPIU_Free( *handle_ptr );
    *handle_ptr = 0;

    return 0;
}


