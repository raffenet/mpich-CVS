/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Get_parent_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Get_parent_port(char ** parent_port)
{
    int val_max_sz;
    int pmi_errno;
    int mpi_errno = MPI_SUCCESS;
#ifdef MPIDI_CH3_IMPLEMENTS_GET_PARENT_PORT
    
    if (MPIDI_CH3I_Process.parent_port_name == NULL)
    { 
	pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
	if (pmi_errno != PMI_SUCCESS)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**pmi_kvs_get_value_length_max", "**pmi_kvs_get_value_length_max %d", pmi_errno);
	    goto fn_exit;
	    /* --END ERROR HANDLING-- */
	}
	
	MPIDI_CH3I_Process.parent_port_name = MPIU_Malloc(val_max_sz);
	if (MPIDI_CH3I_Process.parent_port_name == NULL)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**nomem", NULL);
	    goto fn_exit;
	    /* --END ERROR HANDLING-- */
	}

	/* get the port name of the root of the parents */
	pmi_errno = PMI_KVS_Get(MPIDI_Process.my_pg->ch.kvs_name, "PARENT_ROOT_PORT_NAME", MPIDI_CH3I_Process.parent_port_name,
				val_max_sz);
	if (pmi_errno != PMI_SUCCESS)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**pmi_kvs_get", "**pmi_kvs_get_parent %d", pmi_errno);
	    goto fn_exit;
	    /* --END ERROR HANDLING-- */
	}

    }
    
    *parent_port = MPIDI_CH3I_Process.parent_port_name;

  fn_exit:
#endif    
    return mpi_errno;
}
