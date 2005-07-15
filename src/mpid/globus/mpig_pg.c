/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

MPIG_STATIC mpig_pg_t * mpig_pg_list_head = NULL;
MPIG_STATIC mpig_pg_t * mpig_pg_list_tail = NULL;
MPIG_STATIC mpig_pg_t * mpig_pg_iterator = NULL;

#undef FUNCNAME
#define FUNCNAME mpig_pg_init
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_pg_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pg_init);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pg_init);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    
    /*  fn_return: */
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pg_init);
    return mpi_errno;
}
/* mpig_pg_init() */

#undef FUNCNAME
#define FUNCNAME mpig_pg_finalize
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_pg_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pg_finalize);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pg_finalize);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

#if XXX    
    MPIU_ERR_CHKANDJUMP((mpig_pg_list_head != NULL), mpi_errno, MPI_ERR_INTERN, "**dev|pg_finalize|list_not_empty");
#endif
    
#if 1
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pg_finalize);
    return mpi_errno;
#else    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pg_finalize);
    return mpi_errno;

  fn_fail:
    goto fn_return;
#endif
}
/* mpig_pg_finalize() */


#undef FUNCNAME
#define FUNCNAME mpig_pg_create
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_pg_create(int pg_size, mpig_pg_t ** pg_ptr)
{
    mpig_pg_t * pg = NULL;
    int p;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(2);
    MPIG_STATE_DECL(MPID_STATE_mpig_pg_create);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pg_create);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    MPIU_CHKPMEM_MALLOC(pg, mpig_pg_t *, sizeof(mpig_pg_t), mpi_errno, "process group object");
    MPIU_CHKPMEM_MALLOC(pg->vct, mpig_vc_t *, sizeof(mpig_vc_t) * pg_size, mpi_errno, "virtual connection table");

    pg->ref_count = 0;
    pg->size = pg_size;
    pg->id = NULL;

    /*
     * Initialize the VCs in PG
     */
    for (p = 0; p < pg_size; p++)
    {
	mpig_vc_construct(&pg->vct[p]);
	pg->vct[p].pg = pg;
	pg->vct[p].pg_rank = p;
	mpig_lpid_get_next(&pg->vct[p].lpid);
    }

    /*
     * Add the new process group into the list of outstanding process group structures
     */
    if (mpig_pg_list_head == NULL)
    {
	mpig_pg_list_head = pg;
    }
    
    if (mpig_pg_list_tail != NULL)
    {
	mpig_pg_list_tail->next = pg;
    }
    else
    { 
	mpig_pg_list_head = pg;
    }
    mpig_pg_list_tail = pg;
    
    *pg_ptr = pg;
    MPIU_CHKPMEM_COMMIT();
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pg_create);
    return mpi_errno;
    
  fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_pg_create() */


#undef FUNCNAME
#define FUNCNAME mpig_pg_destroy
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_pg_destroy(mpig_pg_t * pg)
{
    mpig_pg_t * pg_prev;
    mpig_pg_t * pg_cur;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pg_destroy);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pg_destroy);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    pg_prev = NULL;
    pg_cur = mpig_pg_list_head;
    while(pg_cur != NULL)
    {
	if (pg_cur == pg)
	{
	    if (mpig_pg_iterator == pg)
	    { 
		mpig_pg_iterator = pg_prev;
	    }

	    if (pg_prev != NULL)
	    {
		pg_prev->next = pg->next;
	    }
	    else
	    {
		mpig_pg_list_head = pg->next;
	    }
	    
	    if (mpig_pg_list_tail == pg)
	    {
		mpig_pg_list_tail = NULL;
	    }
	    
	    mpig_pg_id_clear(pg);
	    MPIU_Free(pg->vct);
	    MPIU_Free(pg);

	    goto fn_exit;
	}

	pg_prev = pg_cur;
	pg_cur = pg_cur->next;
    }

    MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**dev|pg_not_found", "**dev|pg_not_found %p", pg);

  fn_exit:
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pg_destroy);
    return mpi_errno;

  fn_fail:
    goto fn_return;
}
/* mpig_pg_destroy() */


#undef FUNCNAME
#define FUNCNAME mpig_pg_find
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_pg_find(char * id, mpig_pg_t ** pg_ptr)
{
    mpig_pg_t * pg;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pg_find);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pg_find);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    
    pg = mpig_pg_list_head;
    while (pg != NULL)
    {
	if (mpig_pg_compare_ids(id, pg->id) == 0)
	{
	    *pg_ptr = pg;
	    goto fn_exit;
	}

	pg = pg->next;
    }

    *pg_ptr = NULL;

  fn_exit:
    /* fn_return: */
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pg_find);
    return mpi_errno;
}
/* mpig_pg_find() */


#undef FUNCNAME
#define FUNCNAME mpig_pg_get_next
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_pg_get_next(mpig_pg_t ** pg_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pg_get_next);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pg_get_next);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    if (mpig_pg_iterator == NULL)
    {
	mpig_pg_iterator = mpig_pg_list_head;
    }
    else
    {
	mpig_pg_iterator = mpig_pg_iterator->next;
    }
    
    *pg_ptr = mpig_pg_iterator;

    /* fn_return: */
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pg_get_next);
    return mpi_errno;
}
/* mpig_pg_get_next() */
