/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
*  (C) 2001 by Argonne National Laboratory.
*      See COPYRIGHT in top-level directory.
*/

#include "mpidimpl.h"
#include "pmi.h"

#undef FUNCNAME
#define FUNCNAME MPID_Parse_option
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Parse_option(int num_args, char *args[], int *num_parsed, MPI_Info *info)
{
    int mpi_errno = MPI_SUCCESS;
    PMI_keyval_t *keyvals;
    int i, size;
    MPIDI_STATE_DECL(MPID_STATE_MPID_PARSE_OPTION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_PARSE_OPTION);

    MPIR_Nest_incr();
    
    mpi_errno = PMI_Parse_option(num_args, args, num_parsed, &keyvals, &size);
    if (mpi_errno != PMI_SUCCESS)
    {
	switch (mpi_errno)
	{
	case PMI_ERR_INVALID_NUM_ARGS:
	    break;
	case PMI_ERR_INVALID_ARGS:
	    break;
	case PMI_ERR_INVALID_NUM_PARSED:
	    break;
	case PMI_ERR_INVALID_KEYVALP:
	    break;
	case PMI_ERR_INVALID_SIZE:
	    break;
	default:
	    break;
	}
	MPIU_ERR_CHKANDJUMP((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**fail");
    }
    mpi_errno = NMPI_Info_create(info);
    MPIU_ERR_CHKANDJUMP((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**fail");
    
    for (i=0; i<size; i++)
    {
	mpi_errno = NMPI_Info_set(*info, keyvals[i].key, keyvals[i].val);
	MPIU_ERR_CHKANDJUMP((mpi_errno != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**fail");
    }
    PMI_Free_keyvals(keyvals, size);

  fn_exit:
    MPIR_Nest_decr();
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_PARSE_OPTION);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
