/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

/*
 * MPID_Open_port
 */
#undef FUNCNAME
#define FUNCNAME MPID_Open_port
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Open_port(MPID_Info * info_ptr, char * port_name)
{
    int mpi_errno = MPI_SUCCESS;

    MPIG_STATE_DECL(MPID_STATE_MPID_OPEN_PORT);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_OPEN_PORT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_OPEN_PORT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Open_port */


/*
 * MPID_Comm_Close_port()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Close_port
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Close_port(char *port_name)
{
    int mpi_errno=MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_CLOSE_PORT);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_CLOSE_PORT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_CLOSE_PORT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Comm_Close_port() */


/*
 * MPID_Comm_accept()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Comm_accept
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Comm_accept(char * port_name, MPID_Info * info_ptr, int root, MPID_Comm * comm_ptr, MPID_Comm ** newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_COMM_ACCEPT);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_COMM_ACCEPT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_COMM_ACCEPT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Comm_accept() */


/*
 * MPID_Comm_connect()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Comm_connect
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Comm_connect(char * port_name, MPID_Info * info, int root, MPID_Comm * comm, MPID_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_COMM_CONNECT);
    
    MPIG_FUNC_ENTER(MPID_STATE_MPID_COMM_CONNECT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_COMM_CONNECT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Comm_connect() */


/*
 * MPID_Comm_disconnect()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Comm_disconnect
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int MPID_Comm_disconnect(MPID_Comm *comm_ptr)
{
    int mpi_errno = mpi_errno;
    MPIG_STATE_DECL(MPID_STATE_MPID_COMM_DISCONNECT);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_COMM_DISCONNECT);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", FCNAME);
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_COMM_DISCONNECT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Comm_disconnect() */
