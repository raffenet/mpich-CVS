/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "namepub.h"

/* -- Begin Profiling Symbol Block for routine MPI_Lookup_name */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Lookup_name = PMPI_Lookup_name
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Lookup_name  MPI_Lookup_name
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Lookup_name as PMPI_Lookup_name
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Lookup_name PMPI_Lookup_name

/* One of these routines needs to define the global handle.  Since
   Most routines will use lookup (if they use any of the name publishing
   interface at all), we place this in MPI_Lookup_name. 
*/
MPID_NS_Handle MPIR_Namepub = 0;

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Lookup_name

/*@
   MPI_Lookup_name - Lookup a port given a service name

   Input Parameters:
+ service_name - a service name (string) 
- info - implementation-specific information (handle) 

   Output Parameter:
.  port_name - a port name (string) 


.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_INFO
.N MPI_ERR_OTHER
.N MPI_ERR_ARG
@*/
int MPI_Lookup_name(char *service_name, MPI_Info info, char *port_name)
{
    static const char FCNAME[] = "MPI_Lookup_name";
    int mpi_errno = MPI_SUCCESS;
    MPID_Info *info_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_LOOKUP_NAME);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_LOOKUP_NAME);

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
	    /* Validate character pointers */
	    MPIR_ERRTEST_ARGNULL( service_name, "service_name", mpi_errno );
	    MPIR_ERRTEST_ARGNULL( port_name, "port_name", mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

#ifdef HAVE_NAMEPUB_SERVICE
    if (!MPIR_Namepub)
    {
	mpi_errno = MPID_NS_Create( info_ptr, &MPIR_Namepub );
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */
    }

    mpi_errno = MPID_NS_Lookup( MPIR_Namepub, info_ptr,
	(const char *)service_name, port_name );
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS && 
	MPIR_ERR_GET_CLASS(mpi_errno) != MPI_ERR_NAME)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_LOOKUP_NAME);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_lookup_name", "**mpi_lookup_name %s %I %p", service_name, info, port_name);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_LOOKUP_NAME);
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */

#else

    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nonamepub", 0 );

    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_lookup_name", "**mpi_lookup_name %s %I %p", service_name, info, port_name);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_LOOKUP_NAME);
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
#endif    
}
