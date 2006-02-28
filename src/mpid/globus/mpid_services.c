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
int MPID_Open_port(MPID_Info * info, char * port_name)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;

    MPIG_STATE_DECL(MPID_STATE_MPID_OPEN_PORT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_OPEN_PORT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", fcname);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting: port_name=%s",
		       port_name));
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
int MPID_Close_port(const char * port_name)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno=MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_CLOSE_PORT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_CLOSE_PORT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering: port_name=%s",
		       port_name));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", fcname);
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting"));
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
int MPID_Comm_accept(char * port_name, MPID_Info * info, int root, MPID_Comm * comm, MPID_Comm ** newcommp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_COMM_ACCEPT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_COMM_ACCEPT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", fcname);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting"));
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
int MPID_Comm_connect(const char * port_name, MPID_Info * info, int root, MPID_Comm * comm, MPID_Comm ** newcommp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_COMM_CONNECT);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_MPID_COMM_CONNECT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering"));
    
    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", fcname);
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting"));
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
int MPID_Comm_disconnect(MPID_Comm *comm_ptr)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = mpi_errno;
    MPIG_STATE_DECL(MPID_STATE_MPID_COMM_DISCONNECT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_COMM_DISCONNECT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering"));

    MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_INTERN, {goto fn_fail;}, "**notimpl", "**notimpl %s", fcname);
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_COMM_DISCONNECT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Comm_disconnect() */
