/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"
#include "adio_extern.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Register_datarep = PMPI_Register_datarep
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Register_datarep MPI_Register_datarep
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Register_datarep as PMPI_Register_datarep
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

int MPI_Register_datarep(char *name,
			 MPI_Datarep_conversion_function *read_conv_fn,
			 MPI_Datarep_conversion_function *write_conv_fn,
			 MPI_Datarep_extent_function *extent_fn,
			 void *state)
{
    int error_code;
    ADIOI_Datarep *datarep;
    static char myname[] = "MPI_REGISTER_DATAREP";

    /* --BEGIN ERROR HANDLING-- */
    /* check datarep name */
    if (name == NULL ||
	strnlen(name, 2) < 1 || 
	strnlen(name, MPI_MAX_DATAREP_STRING+1) > MPI_MAX_DATAREP_STRING)
    {
	error_code = MPIO_Err_create_code(MPI_SUCCESS,
					  MPIR_ERR_RECOVERABLE,
					  myname, __LINE__,
					  MPI_ERR_ARG,
					  "**datarepname", 0);
	return MPIO_Err_return_file(MPI_FILE_NULL, error_code);
    }

    /* check datarep isn't already registered */
    for (datarep = ADIOI_Datarep_head; datarep; datarep = datarep->next) {
	if (!strncmp(name, datarep->name, MPI_MAX_DATAREP_STRING)) {
	    error_code = MPIO_Err_create_code(MPI_SUCCESS,
					      MPIR_ERR_RECOVERABLE,
					      myname, __LINE__,
					      MPI_ERR_DUP_DATAREP,
					      "**datarepused",
					      "**datarepused %s",
					      name);
	    return MPIO_Err_return_file(MPI_FILE_NULL, error_code);
	}
    }

    /* check extent function pointer */
    if (extent_fn == NULL)
    {
	error_code = MPIO_Err_create_code(MPI_SUCCESS,
					  MPIR_ERR_RECOVERABLE,
					  myname, __LINE__,
					  MPI_ERR_ARG,
					  "**datarepextent", 0);
	return MPIO_Err_return_file(MPI_FILE_NULL, error_code);
    }
    /* --END ERROR HANDLING-- */

    datarep = ADIOI_Malloc(sizeof(ADIOI_Datarep));
    /* until we get this ansi thing figured out, prototypes are missing */
    datarep->name          = (char *) strdup(datarep);
    datarep->state         = state;
    datarep->read_conv_fn  = read_conv_fn;
    datarep->write_conv_fn = write_conv_fn;
    datarep->extent_fn     = extent_fn;
    datarep->next          = ADIOI_Datarep_head;

    ADIOI_Datarep_head = datarep;

    return MPI_SUCCESS;
}
