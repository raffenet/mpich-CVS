/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpi_init.h"
#include "bnr.h"


/* -- Begin Profiling Symbol Block for routine MPI_Init */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Init = PMPI_Init
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Init  MPI_Init
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Init as PMPI_Init
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Init PMPI_Init

/* Any internal routines can go here.  Make them static if possible */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Init

/*@
   MPI_Init - Initialize the MPI execution environment

   Input Parameters:
+  argc - Pointer to the number of arguments 
-  argv - Pointer to the argument vector

   Notes:

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Init( int *argc, char ***argv )
{
    static const char FCNAME[] = "MPI_Init";
    int mpi_errno = MPI_SUCCESS;
    char pszPortName[MPI_MAX_PORT_NAME];
    int spawned;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INIT);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_PRE_INIT) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**inittwice", 0 );
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INIT);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* 
     * This routine and MPI_Init_thread should share the same code; there 
     * should be no BNR calls in this file.  Most likely, all BNR calls
     * should be within the device's implementation of MPID_Init, which 
     * either this routine or a common routine called by both MPI_Init
     * and MPI_Init_thread use.  The routine MPIR_Init_thread is
     * the intended common routine.
     */

#ifdef FOO
    BNR_Init(&spawned);
#endif

    MPIR_Init_thread( MPI_THREAD_SINGLE, (int *)0 );

#ifdef FOO
    BNR_DB_Get_my_name(MPIR_Process.bnr_dbname);
    BNR_Barrier();

    if (spawned)
    {
	BNR_DB_Get(MPIR_Process.bnr_dbname, MPICH_PARENT_PORT_KEY, pszPortName);
	PMPI_Comm_connect(pszPortName, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &MPIR_Process.comm_parent);
    }
    else
    {
	MPIR_Process.comm_parent = MPI_COMM_NULL;
    }

#endif

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INIT);
    return MPI_SUCCESS;
}
