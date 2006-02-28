/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

volatile mpig_progress_cc_t mpig_progress_cc = 0;


/*
 * MPID_Progress_start()
 */
#if !defined (MPID_Progress_start)
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
#if !defined (MPID_Progress_end)
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

    while (state->dev.cc == mpig_progress_cc)
    { 
	/* mpig_cm_vmpi_progress_wait(state); */
	mpig_cm_xio_progress_wait(state, &mpi_errno, &failed);
	MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|pe_wait");
    }
    
    state->dev.cc = mpig_progress_cc;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_PROGRESS_WAIT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
/* MPID_Progress_wait() */
}


/*
 * MPID_Progress_test()
 */
#if !defined (MPID_Progress_test)
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

    /* mpig_cm_vmpi_progress_test(); */
    mpig_cm_xio_progress_test(&mpi_errno, &failed);
    MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|pe_test");
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PROGRESS, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_PROGRESS_TEST);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Progress_test() */
#endif


/*
 * MPID_Progress_poke()
 */
#if !defined (MPID_Progress_poke)
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
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Progress_poke() */
#endif
