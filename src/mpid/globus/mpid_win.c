/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

/*
 * MPID_Win_create()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Win_create
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Win_create(void * base, MPI_Aint size, int disp_unit, MPID_Info * info, MPID_Comm * comm_ptr, MPID_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_WIN_CREATE);
    
    MPIG_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_CREATE);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
 fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_CREATE);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Win_create() */


/*
 * MPID_Win_free()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Win_free
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Win_free(MPID_Win ** win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_WIN_FREE);
        
    MPIG_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_FREE);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
 
 fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FREE);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Win_free() */


/*
 * MPID_Win_fence()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Win_fence
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Win_fence(int assert, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_WIN_FENCE);

    MPIG_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_FENCE);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);

 fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    MPIG_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Win_fence() */


/*
 * MPID_Win_start()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Win_start
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Win_start(MPID_Group * group_ptr, int assert, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_WIN_START);

    MPIG_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_START);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_START);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Win_start() */



/*
 * MPID_Win_complete()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Win_complete
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Win_complete(MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_WIN_COMPLETE);

    MPIG_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_COMPLETE);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Win_complete() */


/*
 * MPID_Win_post()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Win_post
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Win_post(MPID_Group * group_ptr, int assert, MPID_Win * win_ptr)
{
    int mpi_errno=MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_WIN_POST);

    MPIG_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_POST);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
 fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_POST);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Win_post() */


/*
 * MPID_Win_wait()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Win_wait
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Win_wait(MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIG_STATE_DECL(MPID_STATE_MPID_WIN_WAIT);

    MPIG_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_WAIT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_WAIT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Win_wait() */


/*
 * MPID_Win_test()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Win_test
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Win_test(MPID_Win * win_ptr, int * flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_WIN_TEST);

    MPIG_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_TEST);
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    *flag = FALSE;

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    MPIG_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_TEST);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Win_test() */


/*
 * MPID_Win_lock()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Win_lock
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Win_lock(int lock_type, int dest, int assert, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_WIN_LOCK);

    MPIG_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_LOCK);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);

 fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_LOCK);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Win_lock() */


/*
 * MPID_Win_unlock()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Win_unlock
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Win_unlock(int dest, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_WIN_UNLOCK);

    MPIG_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_UNLOCK);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
 fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Win_unlock() */


/*
 * MPID_Accumulate()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Accumulate
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Accumulate(void * origin_addr, int origin_count, MPI_Datatype origin_datatype,
		    int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
		    MPI_Op op, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_ACCUMULATE);
    
    MPIG_RMA_FUNC_ENTER(MPID_STATE_MPID_ACCUMULATE);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_RMA_FUNC_EXIT(MPID_STATE_MPID_ACCUMULATE);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Accumulate() */


/*
 * MPID_Get()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Get
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
	     int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_GET);
        
    MPIG_RMA_FUNC_ENTER(MPID_STATE_MPID_GET);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_RMA_FUNC_EXIT(MPID_STATE_MPID_GET);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Get() */


#undef FUNCNAME
#define FUNCNAME MPID_Put
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Put(void * origin_addr, int origin_count, MPI_Datatype origin_datatype,
	     int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_PUT);
        
    MPIG_RMA_FUNC_ENTER(MPID_STATE_MPID_PUT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_RMA_FUNC_EXIT(MPID_STATE_MPID_PUT);    
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Put() */


/*
 * MPID_Alloc_mem()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Alloc_mem
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
void * MPID_Alloc_mem(size_t size, MPID_Info * info_ptr)
{
    void * mem_ptr;
    MPIG_STATE_DECL(MPID_STATE_MPID_ALLOC_MEM);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_ALLOC_MEM);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    mem_ptr = MPIU_Malloc(size);
    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_ALLOC_MEM);
    return mem_ptr;
}
/* MPID_Alloc_mem() */


/*
 * MPID_Free_mem()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Free_mem
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Free_mem(void * mem_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_FREE_MEM);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_FREE_MEM);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_Free(mem_ptr);

    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_FREE_MEM);
    return mpi_errno;
}
/* MPID_Free_mem() */
