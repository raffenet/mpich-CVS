/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: mpid_init.c
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pmi.h"

MPID_PerProcess_t MPID_Process;

/*@
   MPID_Init - Initialize the mm device

   Notes:

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Init( void )
{
    static const char FCNAME[] = "MPID_Init";
    char pszPortName[MPI_MAX_PORT_NAME];
    int spawned;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPID_INIT);

    MPID_Process.pmi_dbname[0] = '\0';
    MPID_Process.comm_parent = (MPID_Comm *)0;
    //MPID_Process.lock
    MPID_Process.port_list = (OpenPortNode_t *)0;

    PMI_Init(&spawned);
    PMI_KVS_Get_my_name(MPID_Process.pmi_dbname);
    PMI_Barrier();

    if (spawned)
    {
	PMI_KVS_Get(MPID_Process.pmi_dbname, MPICH_PARENT_PORT_KEY, pszPortName);
	//PMPI_Comm_connect(pszPortName, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &MPID_Process.comm_parent);
    }
    else
    {
	MPID_Process.comm_parent = (MPID_Comm *)0;
    }

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPID_INIT);
    return MPI_SUCCESS;
}
