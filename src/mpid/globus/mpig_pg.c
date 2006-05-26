/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

globus_mutex_t mpig_pg_global_mutex;

MPIG_STATIC mpig_pg_t * mpig_pg_list_head = NULL;
MPIG_STATIC mpig_pg_t * mpig_pg_list_tail = NULL;

/* Local process ID counter for assigning a local ID to each virtual connection.  A local ID is necessary for the MPI_Group
   routines, implemented at the MPICH layer, to function properly.  MT: this requires a thread safe fetch-and-increment */
MPIG_STATIC int mpig_pg_lpid_counter;


/* internal function declarations */
MPIG_STATIC int mpig_pg_create(const char * pg_id, int pg_rank, mpig_pg_t ** pgp);

MPIG_STATIC void mpig_pg_destroy(mpig_pg_t * pg);


/*
 * mpig_pg_init(void)
 */
#undef FUNCNAME
#define FUNCNAME mpig_pg_init
int mpig_pg_init(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pg_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pg_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PG, "entering"));

    mpig_pg_global_mutex_create();
    
    /*  fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PG, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pg_init);
    return mpi_errno;
}
/* mpig_pg_init() */

/*
 * mpig_pg_finalize(void)
 */
#undef FUNCNAME
#define FUNCNAME mpig_pg_finalize
int mpig_pg_finalize(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pg_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pg_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PG, "entering"));

#if XXX    
    MPIU_ERR_CHKANDJUMP((mpig_pg_list_head != NULL), *mpi_errno_p, MPI_ERR_INTERN, "**dev|pg_finalize|list_not_empty");
#endif
    
    mpig_pg_global_mutex_destroy();
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PG, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pg_finalize);
    return mpi_errno;
}
/* mpig_pg_finalize() */


/*
 * mpig_pg_acquire_ref_locked([IN] pg_id, [IN] pg_size, [OUT] pg, [IN/OUT] mpi_errno, [OUT] failed_p)
 *
 * MT-NOTE: to avoid deadlock, the mutex of a PG object should never be acquired while the mutex for a VC contained within that
 * PG object is being held.  should the need arise to lock a VC followed by the PG containing that VC, lock the PG, increment its
 * reference count and the unlock the PG.  the VC mutex may now be safely acquired without fear of deadlock, and the PG object is
 * guaranteed to persist until the extra reference count is released.
 */
#undef FUNCNAME
#define FUNCNAME mpig_pg_acquire_ref_locked
int mpig_pg_acquire_ref_locked(const char * const pg_id, const int pg_size, mpig_pg_t ** const pg_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t pg_global_locked = FALSE;
    mpig_pg_t * pg;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pg_acquire_ref_locked);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pg_acquire_ref_locked);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PG, "entering: pg_id=%s, pg_size=%d", pg_id, pg_size));

    mpig_pg_global_mutex_lock();
    pg_global_locked = TRUE;
    {
	/* attempt to find PG in the list of active process groups */
	*pg_p = NULL;
	pg = mpig_pg_list_head;
	while (pg != NULL)
	{
	    if (mpig_pg_compare_ids(pg_id, pg->id) == 0)
	    {
		*pg_p = pg;
		break;
	    }
	    
	    pg = pg->next;
	}

	if (pg == NULL)
	{
	    /* no matching process group was found, so create a new one */
	    mpi_errno = mpig_pg_create(pg_id, pg_size, pg_p);
	    MPIU_ERR_CHKANDJUMP2((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|pg_create", "**globus|pg_create %s %d",
		pg_id, pg_size);
	}

	/* increment the reference count to reflect reference being returned */
	mpig_pg_inc_ref_count(*pg_p);
    }
    /* mpig_pg_global_mutex_unlock(); -- global mutex is left locked */
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PG, "exiting: pg_id=%s, pg_size=%d, pg=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, pg_id, pg_size, (MPIG_PTR_CAST) pg, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pg_acquire_ref_locked);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	if (pg_global_locked) mpig_pg_global_mutex_unlock();
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_pg_acquire_ref_locked() */


/*
 * mpig_pg_commit([IN/MOD] pg)
 *
 * MT-NOTE: this routine assumes that the neither the global process group module's mutex nor the PG's mutex are held by the
 * current context.
 */
#undef FUNCNAME
#define FUNCNAME mpig_pg_commit
void mpig_pg_commit(mpig_pg_t * const pg)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_pg_commit);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pg_commit);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PG,
		       "entering: pg=" MPIG_PTR_FMT, (MPIG_PTR_CAST) pg));
    
    mpig_pg_global_mutex_lock();
    {
	pg->committed = TRUE;
	if (pg->ref_count == 0)
	{
	    mpig_pg_destroy(pg);
	}
    }
    mpig_pg_global_mutex_unlock();
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PG,
		       "exiting: pg=" MPIG_PTR_FMT, (MPIG_PTR_CAST) pg));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pg_commit);
    return;
}
/* mpig_pg_commit() */


/*
 * mpig_pg_release_ref([IN/MOD] pg)
 *
 * MT-NOTE: this routine assumes that the neither the global process group module's mutex nor the PG's mutex are held by the
 * current context.  futhermore, when the last reference to the PG is released, none of mutexes associated with the VCs in the PG
 * may be held by any context.  this should occur naturally as long as a VC's mutex is unlocked before its reference is released.
 */
#undef FUNCNAME
#define FUNCNAME mpig_pg_release_ref
void mpig_pg_release_ref(mpig_pg_t * const pg)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t pg_inuse;
    MPIG_STATE_DECL(MPID_STATE_mpig_pg_release_ref);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pg_release_ref);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PG,
		       "entering: pg=" MPIG_PTR_FMT, (MPIG_PTR_CAST) pg));
    
    mpig_pg_global_mutex_lock();
    {
	/* mpig_pg_mutex_lock(pg); -- mpig_pg_global_mutex_lock() protects all PG objects */
	{
	    mpig_pg_dec_ref_count(pg, &pg_inuse);
	    if (pg_inuse == FALSE)
	    {
		/*
		 * if the process group has not been committed, then do not delete this process group because it still has not
		 * been initialized by the MPI/MPID routines.  although the process group may exist as a result of asynchronous
		 * communication in one of the communication modules, we need to avoid destroying it until the MPI/MPID routines
		 * have had a chance to properly initialize it.  this feature is particularly important for the process group
		 * associated with MPI_COMM_WORLD as it prevents the process group associated with the local process from being
		 * created, destroyed and recreated again.  such an action would cause the local process id (lpid) values to be
		 * something other than the ranks MPI_COMM_WORLD, which is prohibited.
		 *
		 * NOTE: care must be taken to avoid leaking process groups when errors occur prior to full initialization (ex:
		 * during a MPID_Comm_connect/accept).  uncommitted process groups will remain allocated even if the reference
		 * count reaches zero.  as a result, depending on the situation, synchronization may be necessary to prevent
		 * communication before the process groups are committed.
		 */
		if (pg->committed)
		{
	    
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PG | MPIG_DEBUG_LEVEL_COUNT,
				       "destroying pg: pg=" MPIG_PTR_FMT ", ref_count=%d", (MPIG_PTR_CAST) pg, pg->ref_count));
		    mpig_pg_destroy(pg);
		}
		else
		{
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PG | MPIG_DEBUG_LEVEL_COUNT,
				       "pg not committed; keeping pg alive: pg=" MPIG_PTR_FMT ", ref_count=%d",
				       (MPIG_PTR_CAST) pg, pg->ref_count));
		}
	    }
	    else
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PG | MPIG_DEBUG_LEVEL_COUNT,
				   "pg still active: pg=" MPIG_PTR_FMT ", ref_count=%d", (MPIG_PTR_CAST) pg, pg->ref_count));
	    }
	}
	/* mpig_pg_mutex_lock(pg); -- mpig_pg_global_mutex_unlock() protects all PG objects */
    }
    mpig_pg_global_mutex_unlock();
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PG,
		       "exiting: pg=" MPIG_PTR_FMT, (MPIG_PTR_CAST) pg));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pg_release_ref);
    return;
}
/* mpig_pg_release_ref() */


/*
 * mpig_pg_create([IN] pg_id, [IN] pg_size, [OUT] pg, [IN/OUT] mpi_errno, [OUT] failed_p)
 */
#undef FUNCNAME
#define FUNCNAME mpig_pg_create
MPIG_STATIC int mpig_pg_create(const char * const pg_id, const int pg_size, mpig_pg_t ** const pg_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_pg_t * pg = NULL;
    int p;
    MPIU_CHKPMEM_DECL(1);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pg_create);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pg_create);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PG, "entering: pg_id=%s, pg_size=%d", pg_id, pg_size));

    /* allocate space for the process group object */
    MPIU_CHKPMEM_MALLOC(pg, mpig_pg_t *, sizeof(mpig_pg_t) + (pg_size - 1) * sizeof(mpig_vc_t), mpi_errno, "process group object");

    /* initial PG fields */
    mpig_pg_mutex_create(pg);
    pg->ref_count = 0;
    pg->committed = FALSE;
    pg->size = pg_size;
    pg->id = MPIU_Strdup(pg_id);
    pg->next = NULL;

    /* initialize the VCs in PG */
    for (p = 0; p < pg_size; p++)
    {
	mpig_vc_t * vc = &pg->vct[p];
	
	mpig_vc_construct(vc);
	mpig_vc_set_pg_info(vc, pg, p);
	mpig_vc_set_pg_id(vc, pg->id);
	vc->lpid = mpig_pg_lpid_counter++;
	MPIU_ERR_CHKANDJUMP((mpig_pg_lpid_counter < 0), mpi_errno, MPI_ERR_INTERN, "**globus|pg|lpid_counter");
    }

    /* add the new process group into the list of outstanding process group structures */
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
    
    *pg_p = pg;
    MPIU_CHKPMEM_COMMIT();

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PG, "exiting: pg_id=%s, pg_size=%d, pg=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, pg_id, pg_size, (MPIG_PTR_CAST) pg, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pg_create);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	MPIU_CHKPMEM_REAP();
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_pg_create() */


#undef FUNCNAME
#define FUNCNAME mpig_pg_destroy
MPIG_STATIC void mpig_pg_destroy(mpig_pg_t * const pg)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_pg_t * pg_prev;
    mpig_pg_t * pg_cur;
    int p;
    MPIG_STATE_DECL(MPID_STATE_mpig_pg_destroy);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pg_destroy);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PG,
		       "entering: pg=" MPIG_PTR_FMT, (MPIG_PTR_CAST) pg));
    
    /*
     * remove the PG object from the process group list
     */
    pg_prev = NULL;
    pg_cur = mpig_pg_list_head;
    while(pg_cur != NULL)
    {
	if (pg_cur == pg)
	{
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
	    
	    break;
	}

	pg_prev = pg_cur;
	pg_cur = pg_cur->next;
    }

    MPIU_Assertp(pg_cur != NULL);

    /*
     * destroy the VC objects in the PG object
     */
    for (p = 0; p < pg->size; p++)
    {
	mpig_vc_destruct(&pg->vct[p]);
    }

    /*
     * destroy the PG object
     */
    MPIU_Free(pg->id);
    pg->id = NULL;
    pg->size = 0;
    pg->ref_count = 0;
    mpig_pg_mutex_destroy(pg);
    MPIU_Free(pg);
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PG,
		       "exiting: pg=" MPIG_PTR_FMT, (MPIG_PTR_CAST) pg));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pg_destroy);
    return;
}
/* mpig_pg_destroy() */
