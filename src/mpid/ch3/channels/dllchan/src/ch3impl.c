/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * This file implements the CH3 Channel interface in terms of routines
 * loaded from a dynamically-loadable library.  As such, it is a 
 * complete example of the functions required in a channel.
 */

static void *dllhandle = 0;

/* Here is the home for the table of channel functions; initialized to
   keep it from being a "common" symbol */
struct MPIDI_CH3_Funcs MPIU_CALL_MPIDI_CH3 = { 0 };
int *MPIDI_CH3I_progress_completion_count_ptr = 0;

#undef FUNCNAME
#define FUNCNAME MPIDH_CH3_PreInit
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_PreInit( )
{
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t * pg_p, int pg_rank )
{
    int mpi_errno = MPI_SUCCESS;
    char *dllname = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_CH3_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CH3_INIT);

    /* Determine the channel to load.  There is a default (selected
       at configure time and an override (provided by an environment
       variable (we could use the PMI interface as well).

       Make sure that the selected DLLs are consistent

       Load the function table, and start by invoking the
       channel's version of this routine 
    */
    
    dllname = getenv( "MPICH_CH3CHANNEL" );
    if (!dllname) {
	dllname = MPICH_DEFAULT_CH3_CHANNEL;
    }
    
    mpi_errno = MPIU_DLL_Open( dllname, &dllhandle );
    if (mpi_errno) {
	MPIU_ERR_POP(mpi_errno);
    }
    /* We should check the version numbers here for consistency */

    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_Init", 
				  &MPIU_CALL_MPIDI_CH3.Init );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_Finalize", 
				  &MPIU_CALL_MPIDI_CH3.Finalize );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_iSend", 
				  &MPIU_CALL_MPIDI_CH3.iSend );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_iSendv", 
				  &MPIU_CALL_MPIDI_CH3.iSendv );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_iStartMsg", 
				  &MPIU_CALL_MPIDI_CH3.iStartMsg );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_iStartMsgv", 
				  &MPIU_CALL_MPIDI_CH3.iStartMsgv );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_Progress_test", 
				  &MPIU_CALL_MPIDI_CH3.Progress_test );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_Progress_wait", 
				  &MPIU_CALL_MPIDI_CH3.Progress_wait );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_Connection_terminate", 
				  &MPIU_CALL_MPIDI_CH3.Connection_terminate );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_VC_Init", 
				  &MPIU_CALL_MPIDI_CH3.VC_Init );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_PG_Init", 
				  &MPIU_CALL_MPIDI_CH3.PG_Init );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_VC_GetStateString", 
				  &MPIU_CALL_MPIDI_CH3.VC_GetStateString );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_PortFnsInit", 
				  &MPIU_CALL_MPIDI_CH3.PortFnsInit );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_Connect_to_root", 
				  &MPIU_CALL_MPIDI_CH3.Connect_to_root );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_Get_business_card", 
				  &MPIU_CALL_MPIDI_CH3.Get_business_card );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    mpi_errno = MPIU_DLL_FindSym( dllhandle, "MPIDI_CH3_progress_completion_count_ptr", 
				  &MPIDI_CH3I_progress_completion_count_ptr );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CH3_INIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

int MPIDI_CH3_Finalize( )
{
    MPIU_DLL_Close( dllhandle );
    return MPI_SUCCESS;
}

/* This routine must be exported beyond ch3 device because ch3 defines
   MPID_Progress_wait as this routine.  We may want to change how this
   is done so that the export is used only in the top-level (above ch3) 
   routines */
int MPIDI_CH3_Progress_wait(MPID_Progress_state *progress_state)
{
    return MPIU_CALL_MPIDI_CH3.Progress_wait( progress_state );
}
