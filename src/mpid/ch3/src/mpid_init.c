/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#if defined(HAVE_LIMITS_H)
#include <limits.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif


/* The value 128 is returned by the ch3/Makefile for the echomaxprocname target.  */
#if !defined(MPIDI_PROCESSOR_NAME_SIZE)
#   define MPIDI_PROCESSOR_NAME_SIZE 128
#endif

int MPIDI_Use_optimized_rma=0;

MPIDI_Process_t MPIDI_Process;

#undef FUNCNAME
#define FUNCNAME MPID_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Init(int * argc, char *** argv, int requested, int * provided, int * has_args, int * has_env)
{
    int has_parent;
    MPIDI_PG_t * pg;
    int pg_rank;
    int pg_size;
    MPID_Comm * comm;
    int p;
    char * env;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_INIT);
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));

    /*
     * Initialize the device's process information structure
     */
    MPIDI_Process.recvq_posted_head = NULL;
    MPIDI_Process.recvq_posted_tail = NULL;
    MPIDI_Process.recvq_unexpected_head = NULL;
    MPIDI_Process.recvq_unexpected_tail = NULL;
    MPIDI_Process.lpid_counter = 0;
    MPIDI_Process.processor_name = NULL;
    MPIDI_Process.warnings_enabled = TRUE;
    
#   if defined(HAVE_GETHOSTNAME)
    {
	MPIDI_Process.processor_name = MPIU_Malloc(MPIDI_PROCESSOR_NAME_SIZE);
        if (MPIDI_Process.processor_name == NULL) {
            /* --BEGIN ERROR HANDLING-- */
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_fail;
            /* --END ERROR HANDLING-- */
        }
	
#       if defined(HAVE_WINDOWS_H)
	{
	    DWORD size = MPIDI_PROCESSOR_NAME_SIZE;
	    
	    /*if (!GetComputerName(MPIDI_Process.processor_name, &size))*/
	    if (!GetComputerNameEx(ComputerNameDnsFullyQualified, MPIDI_Process.processor_name, &size))
	    {
		MPIU_Free(MPIDI_Process.processor_name);
		MPIDI_Process.processor_name = NULL;
	    }
	}
#       else
	{
	    if(gethostname(MPIDI_Process.processor_name, MPIDI_PROCESSOR_NAME_SIZE) != 0)
	    {
		/* --BEGIN ERROR HANDLING-- */
		MPIU_Free(MPIDI_Process.processor_name);
		MPIDI_Process.processor_name = NULL;
		/* --END ERROR HANDLING-- */
	    }
	}
#       endif
    }
#   endif


    env = getenv("MPICH_WARNINGS");
    if (env)
    {
	if (strcmp(env, "1") == 0 || strcmp(env, "on") == 0 || strcmp(env, "yes") == 0)
	{ 
	    MPIDI_Process.warnings_enabled = TRUE;
	}
	if (strcmp(env, "0") == 0 || strcmp(env, "off") == 0 || strcmp(env, "no") == 0)
	{ 
	    MPIDI_Process.warnings_enabled = FALSE;
	}
    }

    
    /*
     * Set global process attributes.  These can be overridden by the channel if necessary.
     */
    MPIR_Process.attrs.tag_ub          = MPIDI_TAG_UB;

    
    /*
     * Let the channel perform any necessary initialization
     */
    mpi_errno = MPIDI_CH3_Init(has_args, has_env, &has_parent, &pg, &pg_rank);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|ch3_init", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    pg_size = MPIDI_PG_Get_size(pg);
    MPIDI_Process.my_pg = pg;
    MPIDI_Process.my_pg_rank = pg_rank;
    MPIDI_PG_Add_ref(pg);

    /*
     * Initialize the MPI_COMM_WORLD object
     */
    comm = MPIR_Process.comm_world;

    comm->rank = pg_rank;
    comm->remote_size = pg_size;
    comm->local_size = pg_size;
    
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**ch3|vcrt_create", "**ch3|vcrt_create %s", "MPI_COMM_WORLD");
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**ch3|vcrt_get_ptr", "ch3|vcrt_get_ptr %s", "MPI_COMM_WORLD");
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    for (p = 0; p < pg_size; p++)
    {
	MPID_VCR_Dup(&pg->vct[p], &comm->vcr[p]);
    }

    
    /*
     * Initialize the MPI_COMM_SELF object
     */
    comm = MPIR_Process.comm_self;
    comm->rank = 0;
    comm->remote_size = 1;
    comm->local_size = 1;
    
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**ch3|vcrt_create", "**ch3|vcrt_create %s", "MPI_COMM_SELF");
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**ch3|vcrt_get_ptr", "ch3|vcrt_get_ptr %s", "MPI_COMM_WORLD");
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    MPID_VCR_Dup(&pg->vct[pg_rank], &comm->vcr[0]);

    
    /*
     * If this process group was spawned by a MPI application, the form the MPI_COMM_PARENT inter-communicator.
     */
    
    if (has_parent)
    {
#	if defined(MPIDI_CH3_IMPLEMENTS_COMM_GET_PARENT)
	{
	    mpi_errno = MPIDI_CH3_Comm_get_parent(&comm);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		/* --BEGIN ERROR HANDLING-- */
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|conn_parent", "**ch3|conn_parent %s", val);
		goto fn_fail;
		/* --END ERROR HANDLING-- */
	    }
	}
#       elif (defined(MPIDI_CH3_IMPLEMENTS_GET_PARENT_PORT))
	{
	    char * parent_port;

#         if 0
	    int p0_mpi_errno = MPI_SUCCESS;
  	    /*
	     * Ideally, only process zero would call MPIDI_CH3_Get_parent_port(), but MPI/R_Bcast() cannot be called until
	     * initialization of MPI is complete.  Obviously that's not the case here... (we are in MPID_Init :-)
	     */ 
	    if (pg_rank == 0)
	    { 
		p0_mpi_errno = MPIDI_CH3_Get_parent_port(&parent_port);
	    }
	    else
	    {
		parent_port = NULL;
	    }

	    mpi_errno = MPIR_Bcast(&p0_mpi_errno, 1, MPI_INT, 0, MPIR_Process.comm_world);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|get_parent_port_err_bcast", NULL);
		    goto fn_fail;
	    }
	    if (p0_mpi_errno != MPI_SUCCESS)
	    {
		if (pg_rank == 0)
		{
		    mpi_errno = p0_mpi_errno;
		}
		
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|get_parent_port", NULL);
		goto fn_fail;
	    }
	    /* --END ERROR HANDLING-- */
#	  else
	    mpi_errno = MPIDI_CH3_Get_parent_port(&parent_port);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    { 
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|get_parent_port", NULL);
		goto fn_fail;
	    }
	    /* --END ERROR HANDLING-- */
#         endif
	    
	    mpi_errno = MPID_Comm_connect(parent_port, NULL, 0, MPIR_Process.comm_world, &comm);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		/* --BEGIN ERROR HANDLING-- */
		if (pg_rank == 0)
		{ 
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**ch3|conn_parent", "**ch3|conn_parent %s", parent_port);
		}
		else
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**ch3|conn_parent", NULL);
		}
		goto fn_fail;
		/* --END ERROR HANDLING-- */
	    }
	}
#	else
	{
	    /* --BEGIN ERROR HANDLING-- */
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl",
					     "**notimpl %s", "MPIDI_CH3_Comm_get_parent");
	    /* --END ERROR HANDLING-- */
	}
#	endif

	MPIR_Process.comm_parent = comm;
	MPIU_Assert(MPIR_Process.comm_parent != NULL);
	MPIU_Strncpy(comm->name, "MPI_COMM_PARENT", MPI_MAX_OBJECT_NAME);
	
	/* TODO: Check that this intercommunicator gets freed in MPI_Finalize if not already freed.  */
    }
	
    
    /*
     * Set provided thread level
     */
    if (provided != NULL)
    {
	*provided = MPICH_THREAD_LEVEL;
    }

  fn_exit:
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_INIT);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (MPIDI_Process.processor_name != NULL)
    { 
	MPIU_Free(MPIDI_Process.processor_name);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
