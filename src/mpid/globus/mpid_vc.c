/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"


/*
 * MPID_VCRT_Create()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Create
int MPID_VCRT_Create(int size, MPID_VCRT * vcrt_ptr)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vcrt_t * vcrt;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_VCRT_CREATE);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCRT_CREATE);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_VC, "entering: size=%d, vcrt_ptr="
		       MPIG_PTR_FMT, size, (MPIG_PTR_CAST) vcrt_ptr));
    
    vcrt = MPIU_Malloc(sizeof(mpig_vcrt_t) + (size - 1) * sizeof(mpig_vc_t *));
    MPIU_ERR_CHKANDJUMP1((vcrt == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "virtual connection reference table");

    mpig_vcrt_mutex_create(vcrt);
    mpig_vcrt_rc_acq(vcrt, FALSE);
    {
	vcrt->ref_count = 1;
	vcrt->size = size;
    }
    mpig_vcrt_rc_rel(vcrt, TRUE);
    
    *vcrt_ptr = vcrt;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_VC, "exiting: vcrt_ptr=" MPIG_PTR_FMT
		       ", vcrt=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) vcrt_ptr, (MPIG_PTR_CAST) vcrt, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCRT_CREATE);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* MPID_VCRT_Create() */


/*
 * MPID_VCRT_Add_ref()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Add_ref
int MPID_VCRT_Add_ref(MPID_VCRT vcrt)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_VCRT_ADD_REF);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCRT_ADD_REF);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_VC, "entering: vcrt=" MPIG_PTR_FMT,
		       (MPIG_PTR_CAST) vcrt));
    
    mpig_vcrt_mutex_lock(vcrt);
    {
	vcrt->ref_count += 1;
    }
    mpig_vcrt_mutex_unlock(vcrt);
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_VC, "exiting: vcrt=" MPIG_PTR_FMT
		       ", mpi_errno=0x%08x", (MPIG_PTR_CAST) vcrt, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCRT_ADD_REF);
    return mpi_errno;
}
/* MPID_VCRT_Add_ref() */


/*
 * MPID_VCRT_Release()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Release
int MPID_VCRT_Release(MPID_VCRT vcrt)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t inuse;
    int errors = 0;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_VCRT_RELEASE);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCRT_RELEASE);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_VC, "entering: vcrt=" MPIG_PTR_FMT,
		       (MPIG_PTR_CAST) vcrt));

    mpig_vcrt_mutex_lock(vcrt);
    {
	inuse = (--vcrt->ref_count) ? TRUE : FALSE;
    }
    mpig_vcrt_mutex_unlock(vcrt);

    if (inuse == FALSE)
    {
	int p;
	
	for (p = 0; p < vcrt->size; p++)
	{
	    mpig_vc_release_ref(vcrt->vcr_table[p], &mpi_errno, &failed);
	    if (failed) errors++;
	}
	
	vcrt->size = 0;
	MPIU_Free(vcrt);
    }

    MPIU_ERR_CHKANDJUMP1((errors > 0), mpi_errno, MPI_ERR_OTHER, "**globus|vc_release_ref",
			 "**globus|vc_release_ref_n %d", errors);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_VC, "exiting: vcrt=" MPIG_PTR_FMT
		       "mpi_errno=0x%08x", (MPIG_PTR_CAST) vcrt, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCRT_RELEASE);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* MPID_VCRT_Release() */


/*
 * MPID_VCRT_Get_ptr()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Get_ptr
int MPID_VCRT_Get_ptr(MPID_VCRT vcrt, MPID_VCR **vcr_array_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_VCRT_GET_PTR);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCRT_GET_PTR);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_VC, "entering: vcrt=" MPIG_PTR_FMT,
		       (MPIG_PTR_CAST) vcrt));
    
    mpig_vcrt_rc_acq(vcrt, TRUE);
    {
	*vcr_array_p = vcrt->vcr_table;
    }
    mpig_vcrt_rc_rel(vcrt, FALSE);
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_VC, "exiting: vcrt=" MPIG_PTR_FMT
		       ", vcr_array=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) vcrt, (MPIG_PTR_CAST) *vcr_array_p,
		       mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCRT_GET_PTR);
    return mpi_errno;
}
/* MPID_VCRT_Get_ptr() */


/*
 * MPID_VCR_Dup()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCR_Dup
int MPID_VCR_Dup(MPID_VCR orig_vcr, MPID_VCR * new_vcr_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t vc_was_inuse;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_VCR_DUP);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCR_DUP);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_VC, "entering: orig_vcr=" MPIG_PTR_FMT
		       ", new_vcr_p=" MPIG_PTR_FMT, (MPIG_PTR_CAST) orig_vcr, (MPIG_PTR_CAST) new_vcr_p));

    mpig_vc_mutex_lock(orig_vcr);
    {
	mpig_vc_inc_ref_count(orig_vcr, &vc_was_inuse, &mpi_errno, &failed);
    }
    mpig_vc_mutex_unlock(orig_vcr);
    /* if (vc_was_inuse == FALSE) mpig_pg_add_ref(pg); -- not necessary since orig_vcr would already hold a ref to the PG */

    *new_vcr_p = orig_vcr;
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_VC, "exiting: orig_vcr=" MPIG_PTR_FMT
		       ", new_vcr_p=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) orig_vcr, (MPIG_PTR_CAST) new_vcr_p,
		       mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCR_DUP);
    return mpi_errno;
}
/* MPID_VCR_Dup() */


/*
 * MPID_VCR_Get_lpid()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCR_Get_lpid
int MPID_VCR_Get_lpid(MPID_VCR vcr, int * lpid_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_VCR_GET_LPID);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCR_GET_LPID);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_VC, "entering: vcr=" MPIG_PTR_FMT
		       ", lpid_p=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vcr, (MPIG_PTR_CAST) lpid_p));
    
    mpig_vc_rc_acq(vcr, TRUE);
    {
	*lpid_p = vcr->lpid;
    }
    mpig_vc_rc_rel(vcr, FALSE);
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_VC, "entering: vcr=" MPIG_PTR_FMT
		       ", lpid_p=" MPIG_PTR_FMT ", lpid=%d, mpi_errno=0x%08x", (MPIG_PTR_CAST) vcr, (MPIG_PTR_CAST) lpid_p,
		       *lpid_p, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCR_GET_LPID);
    return mpi_errno;
}
/* MPID_VCR_Get_lpid() */


/*
 * void mpig_vc_release_ref([IN/MOD] vc)
 *
 * Paramters:
 *
 * vc - [IN/MOD] virtual connection object
 *
 * MT-NOTE: this routine locks the VC mutex, and may lock the mutex of the associated PG object as well.  therefore, no other
 * routines on call stack of the current context may be not be holding those mutexes when this routine is called.
 */
#undef FUNCNAME
#define FUNCNAME mpig_vc_release_ref
void mpig_vc_release_ref(mpig_vc_t * const vc, int * const mpi_errno_p, bool_t * const failed_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t vc_inuse;
    mpig_pg_t * pg = NULL;
    MPIG_STATE_DECL(MPID_STATE_mpig_vc_release_ref);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_vc_release_ref);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_VC, "entering: vc=" MPIG_PTR_FMT,
		       (MPIG_PTR_CAST) vc));
    
    mpig_vc_mutex_lock(vc);
    {
	pg = mpig_vc_get_pg(vc);
	mpig_vc_dec_ref_count(vc, &vc_inuse, mpi_errno_p, failed_p);
    }
    mpig_vc_mutex_unlock(vc);

    if (*failed_p) goto fn_fail;
    /*
     * note: a communication module may decide that a VC is still in use even if the VC's reference count has reached zero.  for
     * example: this can occur when a shutdown protocol is required to cleanly terminate the underlying connection, and the
     * presence of the VC object is required until that protocol completes.  in cases such as this, it is the responsibility of
     * the CM to release the PG once the reference count once the VC is no longer needed.
     */
    if (vc_inuse == FALSE) mpig_pg_release_ref(pg);
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_VC, "exiting: vc=" MPIG_PTR_FMT
		       ", mpi_errno=0x%08x, failed=%s", (MPIG_PTR_CAST) vc, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_vc_release_ref);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}


/*
 * void mpig_vc_null_func(none)
 *
 * this routine serves as the last function in the VC function table.  its purpose is to help detect when a communication
 * module's VC table has not be updated when a function be added or removed from the table.  it is not fool proof as it requires
 * the type signature not to match, but there should be few (if any) routines in the VC table that have no arguments and return
 * no values.
 */
#undef FUNCNAME
#define FUNCNAME mpig_vc_null_func
void mpig_vc_null_func(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_vc_null_func);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_vc_null_func);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR, "FATAL ERROR: mpig_vc_null_func called.  aborting program"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_vc_null_func);
    MPID_Abort(NULL, MPI_SUCCESS, 13, "FATAL ERROR: mpig_vc_null_func called.  Aborting Program.");
}
