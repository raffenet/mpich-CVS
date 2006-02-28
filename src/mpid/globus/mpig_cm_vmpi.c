/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"


/*
 * mpig_cm_vmpi_init()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_init
int mpig_cm_vmpi_init(int * argc, char *** argv)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

#   if defined(MPIG_VMPI)
    {
	int vmpi_rc;
	
	/* initialize vendor MPI */
	vpi_rc = mpig_vmpi_init(argc, argv);
	MPIU_ERR_CHKANDJUMP((vmpi_rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|vmpi_init");

	/* XXX: set the VMPI_COMM_WORLD error handler to VMPI_ERRORS_RETURN */

	/* get VMPI_COMM_WORLD, and this process' rank and size.  store the information in the process structure. */
	vmpi_rc = mpig_vmpi_comm_get_world(&mpig_process.vmpi_cw);
	MPIU_ERR_CHKANDJUMP((vmpi_rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|vmpi_get_comm_world");
	
	vmpi_rc = mpig_vmpi_comm_get_size(&mpig_process.vmpi_cw, &mpig_process.vmpi_cw_size);
	MPIU_ERR_CHKANDJUMP((vmpi_rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|vmpi_get_comm_size");
	
	vmpi_rc = mpig_vmpi_comm_get_rank(&mpig_process.vmpi_cw, &mpig_process.vmpi_cw_rank);
	MPIU_ERR_CHKANDJUMP((vmpi_rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|vmpi_get_comm_rank");

	/* XXX: add contact information to the business card */

      fn_return:
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", mpi_errno));
	return mpi_errno;

      fn_fail:
	{   /* --BEGIN ERROR HANDLING-- */
	    goto fn_return;
	}   /* --END ERROR HANDLING-- */
    }
#   else
    {
	/* ...nothing to do... */
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", mpi_errno));
	return mpi_errno;
    }
#   endif
    
#if defined(MPIG_VMPI)
#else
    return mpi_errno;
#endif
}
/* mpig_cm_vmpi_init() */


/*
 * mpig_cm_vmpi_finalize()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_finalize
int mpig_cm_vmpi_finalize(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

#   if defined(MPIG_VMPI)
    {
	vmpi_rc = mpig_vmpi_finalize();
	MPIU_ERR_CHKANDJUMP((vmpi_rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|vmpi_finalize");
	
      fn_return:
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", mpi_errno));
	return mpi_errno;

      fn_fail:
	{   /* --BEGIN ERROR HANDLING-- */
	    goto fn_return;
	}   /* --END ERROR HANDLING-- */
    }
#   else
    {
	/* ...nothing to do... */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", mpi_errno));
	return mpi_errno;
    }
#endif
}
/* mpig_cm_vmpi_finalize() */


/*
 * mpig_cm_vmpi_add_contact_info([IN/OUT] business card)
 *
 * Add any and all contact information for this communication module to the supplied business card.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_add_contact_info
int mpig_cm_vmpi_add_contact_info(mpig_bc_t * bc)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));
    
#   if defined(MPIG_VMPI)
    {
	/*
	 * XXX: broadcast a unique signature from VMPI process 0 to all other VMPI processes so that all processes will recognize
	 * the other processes in their subjob.
	 */
	
      fn_return:
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", mpi_errno));
	return mpi_errno;

      fn_fail:
	{   /* --BEGIN ERROR HANDLING-- */
	    goto fn_return;
	}   /* --END ERROR HANDLING-- */
    }
#   else
    {
	/* ...nothing to do... */
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", mpi_errno));
	return mpi_errno;
    }
#   endif
}
/* mpig_cm_vmpi_add_contact_info() */


/*
 * int mpig_cm_vmpi_select_module([IN] bc, [IN/OUT] vc, [OUT] selected)
 *
 * Check the business card to see if the connection module can communicate with the remote process associated with the supplied
 * VC.  If it can, then the VC will be initialized accordingly.
 *
 * Parameters:
 *
 * bc [IN] - business card containing contact information
 * vc [IN] - vc object to initialize if the communication module is capable of performing communication with the associated process
 * selected [OUT] - TRUE if the communication module can communicate with the remote process; otherwise FALSE
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_select_module
int mpig_cm_vmpi_select_module(mpig_bc_t * bc, mpig_vc_t * vc, bool_t * selected)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));
    
#   if defined(MPIG_VMPI)
    {
	/*
	 * XXX: compare signature acquired from VMPI process 0 to determine if the other process is in the same subjob.
	 */
	
      fn_return:
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", mpi_errno));
	return mpi_errno;

      fn_fail:
	{   /* --BEGIN ERROR HANDLING-- */
	    goto fn_return;
	}   /* --END ERROR HANDLING-- */
    }
#   else
    {
	*selected = FALSE;

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", mpi_errno));
	return mpi_errno;
    }
#   endif
}
/* int mpig_cm_vmpi_select_module() */
