/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

mpig_pe_count_t mpig_pe_count = 0;
int mpig_pe_num_cm_must_be_polled = 0;
int mpig_pe_active_ops_count = 0;


/*
 * MPID_Progress_start()
 */
#if !defined (HAVE_MPID_PROGRESS_START_MACRO)
#undef FUNCNAME
#define FUNCNAME MPID_Progress_start
void MPID_Progress_start(MPID_Progress_state * state)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_MPID_PROGRESS_START);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_PROGRESS_START);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));
    
#   error MPID_Progress_start() should be a macro
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_PROGRESS_START);
}
/* MPID_Progress_start() */
#endif


/*
 * MPID_Progress_end()
 */
#if !defined (HAVE_MPID_PROGRESS_END_MACRO)
#undef FUNCNAME
#define FUNCNAME MPID_Progress_end
void MPID_Progress_end(MPID_Progress_state * state)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_MPID_PROGRESS_END);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_PROGRESS_END);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));

#   error MPID_Progress_end() should be a macro
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_PROGRESS_END);
}
/* MPID_Progress_end() */
#endif


/*
 * MPID_Progress_wait()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Progress_wait
int MPID_Progress_wait(MPID_Progress_state * state)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_PROGRESS_WAIT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_PROGRESS_WAIT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));

    while (TRUE)
    {
	/* NOTE: a CM pe_wait() routine must not block if the total number of active operations is greater than the number of
	   active operations in that module or if more than one CM requires polling in order to make progress.  a CM can
	   determine if it safe to block by calling mpig_pe_cm_can_block(). */

	/* XXX: should we loop some number of times here to reduce average latency, or does that belong in
	   mpig_cm_vmpi_pe_wait()? */
	mpig_cm_vmpi_pe_wait(state, &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**globus|pe_wait", "**globus|pe_wait %s", "CM_VMPI");

	/* XXX: likewise, should we perform a thread yield here if no progress was made, allowing the XIO CM threads some CPU
	   time in which to make progress, or should that decision be in the XIO CM pe_wait() routine? */
	mpig_cm_xio_pe_wait(state, &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**globus|pe_wait", "**globus|pe_wait %s", "CM_XIO");

        if (state->dev.count != mpig_pe_count) break;
    }
    
    state->dev.count = mpig_pe_count;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_PROGRESS_WAIT);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
/* MPID_Progress_wait() */
}


/*
 * MPID_Progress_test()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Progress_test
int MPID_Progress_test(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_PROGRESS_TEST);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_PROGRESS_TEST);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));

    mpig_cm_vmpi_pe_test(&mpi_errno, &failed);
    MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**globus|pe_test", "**globus|pe_test %s", "CM_VMPI");
    mpig_cm_xio_pe_test(&mpi_errno, &failed);
    MPIU_ERR_CHKANDJUMP1((failed), mpi_errno, MPI_ERR_OTHER, "**globus|pe_test", "**globus|pe_test %s", "CM_XIO");
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_PROGRESS_TEST);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* MPID_Progress_test() */


/*
 * MPID_Progress_poke()
 */
#if !defined (HAVE_MPID_PROGRESS_POKE_MACRO)
#undef FUNCNAME
#define FUNCNAME MPID_Progress_poke
int MPID_Progress_poke(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_PROGRESS_POKE);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_PROGRESS_POKE);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));

#   error MPID_Progress_poke() should be a macro
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_PROGRESS_POKE);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* MPID_Progress_poke() */
#endif
