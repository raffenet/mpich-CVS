/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_init
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_init(void)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_INIT);

    /* MT: needs to be atomically incremented? */
    MPIDU_Socki_initialized++;

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_INIT);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_INIT);
    return MPI_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_Sock_finalize
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDU_Sock_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_SOCK_FINALIZE);

    /* MT: needs to be atomically tested and decremented? */
    MPIDU_SOCKI_VERIFY_INIT(mpi_errno);
    MPIDU_Socki_initialized--;
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_SOCK_FINALIZE);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_SOCK_FINALIZE);
    return mpi_errno;
}
