/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

static MPIDI_PG_t * MPIDI_PG_list = NULL;
static MPIDI_PG_t * MPIDI_PG_iterator_next = NULL;
static MPIDI_PG_Compare_ids_fn_t MPIDI_PG_Compare_ids_fn;
static MPIDI_PG_Destroy_fn_t MPIDI_PG_Destroy_fn;


#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Init(MPIDI_PG_Compare_ids_fn_t compare_ids_fn, MPIDI_PG_Destroy_fn_t destroy_fn)
{
    int mpi_errno = MPI_SUCCESS;
    
    MPIDI_PG_Compare_ids_fn = compare_ids_fn;
    MPIDI_PG_Destroy_fn = destroy_fn;

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

/* ifdefing out this check because the list will not be NULL in Ch3_finalize because
   one additional reference is retained in MPIDI_Process.my_pg. That reference is released
   only after ch3_finalize returns. If I release it before ch3_finalize, the ssm channel
   crashes.
*/
    
#ifdef FOO

    if (MPIDI_PG_list != NULL)
    { 
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
        "**dev|pg_finalize|list_not_empty", NULL); 
	/* --END ERROR HANDLING-- */
    }
#endif

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Create(int vct_sz, void * pg_id, MPIDI_PG_t ** pg_ptr)
{
    MPIDI_PG_t * pg = NULL;
    int p;
    int mpi_errno = MPI_SUCCESS;
    
    pg = MPIU_Malloc(sizeof(MPIDI_PG_t));
    if (pg == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    pg->vct = MPIU_Malloc(sizeof(MPIDI_VC_t) * vct_sz);
    if (pg->vct == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    pg->handle = 0;
    MPIU_Object_set_ref(pg, vct_sz);
    pg->size = vct_sz;
    pg->id = pg_id;
    
    for (p = 0; p < vct_sz; p++)
    {
	/* Initialize device fields in the VC object */
	MPIDI_VC_Init(&pg->vct[p], pg, p);
    }

    pg->next = MPIDI_PG_list;
    if (MPIDI_PG_iterator_next == MPIDI_PG_list)
    {
	MPIDI_PG_iterator_next = pg;
    }
    MPIDI_PG_list = pg;
    
    *pg_ptr = pg;
    
  fn_exit:
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (pg->vct != NULL)
    {
	MPIU_Free(pg->vct);
    }

    if (pg != NULL)
    {
	MPIU_Free(pg);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Destroy(MPIDI_PG_t * pg)
{
    MPIDI_PG_t * pg_prev;
    MPIDI_PG_t * pg_cur;
    int mpi_errno = MPI_SUCCESS;

    pg_prev = NULL;
    pg_cur = MPIDI_PG_list;
    while(pg_cur != NULL)
    {
	if (pg_cur == pg)
	{
	    if (MPIDI_PG_iterator_next == pg)
	    { 
		MPIDI_PG_iterator_next = MPIDI_PG_iterator_next->next;
	    }

            if (pg_prev == NULL)
                MPIDI_PG_list = pg->next; 
            else
                pg_prev->next = pg->next;

	    MPIDI_PG_Destroy_fn(pg, pg->id);
	    MPIU_Free(pg->vct);
	    MPIU_Free(pg);

	    goto fn_exit;
	}

	pg_prev = pg_cur;
	pg_cur = pg_cur->next;
    }

    /* --BEGIN ERROR HANDLING-- */
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
				     "**dev|pg_not_found", "**dev|pg_not_found %p", pg);
    /* --END ERROR HANDLING-- */

  fn_exit:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Find
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Find(void * id, MPIDI_PG_t ** pg_ptr)
{
    MPIDI_PG_t * pg;
    int mpi_errno = MPI_SUCCESS;
    
    pg = MPIDI_PG_list;
    while (pg != NULL)
    {
	if (MPIDI_PG_Compare_ids_fn(id, pg->id) != FALSE)
	{
	    *pg_ptr = pg;
	    goto fn_exit;
	}

	pg = pg->next;
    }

    *pg_ptr = NULL;

  fn_exit:
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Get_next
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Get_next(MPIDI_PG_t ** pg_ptr)
{
    *pg_ptr = MPIDI_PG_iterator_next;
    if (MPIDI_PG_iterator_next != NULL)
    { 
	MPIDI_PG_iterator_next = MPIDI_PG_iterator_next->next;
    }

    return MPI_SUCCESS;
}
