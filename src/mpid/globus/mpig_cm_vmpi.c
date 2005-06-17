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
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_cm_vmpi_init(int * argc, char *** argv)
{
    int mpi_errno = MPI_SUCCESS;

#   if defined(MPIG_VMPI)
    {
	/*
	 * XXX: Should this be done here, or does it need to be a special case in MPID_Init() in order to support calling VMPI
	 * from the MPI macros?
	 *
	 * rc = mpig_vmpi_init(argc, argv);
	 */
    }
#   else
    {
	/* ...nothing to do... */
    }
#   endif
    
#if 1
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;
#else
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
#endif
}
/* mpig_cm_vmpi_init() */


/*
 * mpig_cm_vmpi_finalize()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_finalize
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_cm_vmpi_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

#   if defined(MPIG_VMPI)
    {
	/*
	 * XXX: should this be done here, or does it need to be a special case in MPID_Finalize() in order to support calling
	 * VMPI from the MPI macros?
	 *
	 * rc = mpig_vmpi_init(argc, argv);
	 */
    }
#   else
    {
	/* ...nothing to do... */
    }
#   endif
    
#if 1
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;
#else
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
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
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_cm_vmpi_add_contact_info(mpig_bc_t * bc)
{
    int mpi_errno = MPI_SUCCESS;
    
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
#   if defined(MPIG_VMPI)
    {
	/*
	 * XXX: broadcast a unique signature from VMPI process 0 to all other VMPI processes so that all processes will recognize
	 * the other processes in their subjob.
	 */
    }
#   else
    {
	/* ...nothing to do... */
    }
#   endif

#if 1
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;
#else
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
#endif
}
/* mpig_cm_vmpi_add_contact_info() */


/*
 * int mpig_cm_vmpi_select_module([IN] bc, [IN/OUT] vc, [OUT] flag)
 *
 * Check the business card to see if the connection module can communicate with the remote process associated with the supplied
 * VC.  If it can, then the VC will be initialized accordingly.
 *
 * Parameters:
 *
 * bc [IN] - business card containing contact information
 * vc [IN] - vc object to initialize if the communication module is capable of performing communication with the associated process
 * flag [OUT] - TRUE if the communication module can communicate with the remote process; otherwise FALSE
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_select_module
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_cm_vmpi_select_module(mpig_bc_t * bc, mpig_vc_t * vc, int * flag)
{
    int mpi_errno = MPI_SUCCESS;
    
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
#   if defined(MPIG_VMPI)
    {
	/*
	 * XXX: compare signature acquired from VMPI process 0 to determine if the other process is in the same subjob.
	 */
    }
#   else
    {
	*flag = FALSE;
    }
#   endif

#if 1
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;
#else
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
#endif
}
/* int mpig_cm_vmpi_select_module([IN] business card, [IN/OUT] virtual connection, [OUT] flag) */
