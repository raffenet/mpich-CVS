/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Open_port */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Open_port = PMPI_Open_port
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Open_port  MPI_Open_port
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Open_port as PMPI_Open_port
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Open_port PMPI_Open_port

/* Any internal routines can go here.  Make them static if possible */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Open_port

/*@
   MPI_Open_port - Establish an address that can be used to establish 
   connections between groups of MPI processes

 Input Parameter:
. info - implementation-specific information on how to establish a 
   port for 'MPI_Comm_accept' (handle) 

 Output Parameter:
. port_name - newly established port (string) 

Notes:
MPI copies a system-supplied port name into 'port_name'. 'port_name' identifies
the newly opened port and can be used by a client to contact the server. 
The maximum size string that may be supplied by the system is 
'MPI_MAX_PORT_NAME'. 

 Reserved Info Key Values:
+ ip_port - Value contains IP port number at which to establish a port. 
- ip_address - Value contains IP address at which to establish a port.
 If the address is not a valid IP address of the host on which the
 'MPI_OPEN_PORT' call is made, the results are undefined. 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Open_port(MPI_Info info, char *port_name)
{
    static const char FCNAME[] = "MPI_Open_port";
    int mpi_errno = MPI_SUCCESS;
    MPID_Info *info_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_OPEN_PORT);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_OPEN_PORT);
    MPIR_ERRTEST_INITIALIZED_FIRSTORJUMP;

    /* Get handles to MPI objects. */
#   ifdef HAVE_ERROR_CHECKING
    {
	MPID_Info_get_ptr( info, info_ptr );
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Info_valid_ptr(info_ptr,mpi_errno);
	    MPIR_ERRTEST_ARGNULL(port_name,"port_name",mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   else
    MPID_Info_get_ptr( info, info_ptr );
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Open_port(info_ptr, port_name);

    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_OPEN_PORT);
	return MPI_SUCCESS;
    }
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_open_port", "**mpi_open_port %I %p", info, port_name);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_OPEN_PORT);
    return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
    /* --END ERROR HANDLING-- */
}
