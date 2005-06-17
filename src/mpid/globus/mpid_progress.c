/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

/*
 * MPID_Progress_start()
 */
#if !defined (MPID_Progress_start)
#undef FUNCNAME
#define FUNCNAME MPID_Progress_start
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
void MPID_Progress_start(MPID_Progress_state * state)
{
    MPIG_STATE_DECL(MPID_STATE_MPID_PROGRESS_START);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_PROGRESS_START);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    /* XXX: ... */
    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
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
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
void MPID_Progress_end(MPID_Progress_state * state)
{
    MPIG_STATE_DECL(MPID_STATE_MPID_PROGRESS_END);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_PROGRESS_END);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    /* XXX: ... */
    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_PROGRESS_END);
}
/* MPID_Progress_end() */
#endif


/*
 * MPID_Progress_wait()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Progress_wait
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Progress_wait(MPID_Progress_state * state)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_PROGRESS_WAIT);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_PROGRESS_WAIT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
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
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Progress_test(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_PROGRESS_TEST);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_PROGRESS_TEST);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
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
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Progress_poke(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_PROGRESS_POKE);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_PROGRESS_POKE);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_PROGRESS_POKE);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Progress_poke() */
#endif
