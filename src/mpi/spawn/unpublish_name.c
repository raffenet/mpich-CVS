/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "namepub.h"

/* -- Begin Profiling Symbol Block for routine MPI_Unpublish_name */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Unpublish_name = PMPI_Unpublish_name
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Unpublish_name  MPI_Unpublish_name
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Unpublish_name as PMPI_Unpublish_name
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Unpublish_name PMPI_Unpublish_name

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Unpublish_name

/*@
   MPI_Unpublish_name - Unpublish a service name published with MPI_Publish_name
 Input Parameters:
+ service_name - a service name (string) 
. info - implementation-specific information (handle) 
- port_name - a port name (string) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_INFO
.N MPI_ERR_ARG
.N MPI_ERR_OTHER
@*/
int MPI_Unpublish_name(char *service_name, MPI_Info info, char *port_name)
{
    static const char FCNAME[] = "MPI_Unpublish_name";
    int mpi_errno = MPI_SUCCESS;
    MPID_Info *info_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_UNPUBLISH_NAME);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_UNPUBLISH_NAME);
    /* Get handles to MPI objects. */
    MPID_Info_get_ptr( info, info_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate info_ptr (only if not null) */
	    if (info_ptr) 
		MPID_Info_valid_ptr( info_ptr, mpi_errno );
	    MPIR_ERRTEST_ARGNULL( service_name, "service_name", mpi_errno );
	    MPIR_ERRTEST_ARGNULL( port_name, "port_name", mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_UNPUBLISH_NAME);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

#ifdef HAVE_NAMEPUB_SERVICE
    if (!MPIR_Namepub)
    {
	mpi_errno = MPID_NS_Create( info_ptr, &MPIR_Namepub );
    }
    if (mpi_errno == MPI_SUCCESS) 
	mpi_errno = MPID_NS_Unpublish( MPIR_Namepub, info_ptr, 
				       (const char *)service_name );
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_UNPUBLISH_NAME);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_unpublish_name", "**mpi_unpublish_name %s %I %s", service_name, info, port_name);
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    }
    return MPI_SUCCESS;
#else
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_UNPUBLISH_NAME);
    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nonamepub", 0 );
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
#endif    
}
