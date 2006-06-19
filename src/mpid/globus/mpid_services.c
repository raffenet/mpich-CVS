/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

int mpig_intercomm_exchange_contextid(int mask_size, unsigned * local_mask, unsigned * remote_mask, void * conn);

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

    mpi_errno = mpig_port_open(info, port_name);
    MPIU_ERR_CHKANDJUMP2((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|connacc|open_port",
	"**globus|connacc|open_port %p %p", info, port_name);
    
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

    mpi_errno = mpig_port_close(port_name);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|connacc|close_port",
	"**globus|connacc|close_port %s", port_name);
    
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
    mpig_connection_t * conn = NULL;
    char * local_vct_buf;
    int local_vct_size;
    char * remote_vct_buf;
    int remote_vct_size;
    int newctx = 0;
    int mrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_COMM_ACCEPT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_COMM_ACCEPT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering"));

    MPIR_Nest_incr();
    
    /* create a new communicator structure */

    if (root)
    {
	/* create an array of process containing the process group id, process group rank, and serialized business card for each
	   VC in the virtual connection reference table (VCRT) attached to the supplied communicator */

	/* accept a new connect for the port */
	mpi_errno = mpig_port_accept(port_name, comm->remote_size, local_vct_size, &remote_vct_size, &conn);
	MPIU_ERR_CHKANDSTMT((mpi_errno), mpi_errno, MPI_ERR_OTHER, {goto endblk;}, "**globus|connacc|conn_accept");

	/* send the serialized VC data to the connecting process */
	mpi_errno = mpig_connection_send(conn, local_vct_buf, local_vct_size);
	MPIU_ERR_CHKANDSTMT((mpi_errno), mpi_errno, MPI_ERR_OTHER, {goto endblk;}, "**globus|connacc|conn_send");

	/* receive serialized VC data from the connecting process */
	remote_vct_buf = (char *) MPIU_Malloc(remote_vct_size);
	MPIU_ERR_CHKANDSTMT1((remote_vct_buf == NULL), mpi_errno, MPI_ERR_OTHER, {goto endblk;}, "**nomem",
	    "**nomem %s", "receive buffer for remote VC objects");
	
	mpi_errno = mpig_connection_recv(conn, remote_vct_buf, remote_vct_size);
	MPIU_ERR_CHKANDSTMT((mpi_errno), mpi_errno, MPI_ERR_OTHER, {goto endblk;}, "**globus|connacc|conn_recv");
	
	/* allocate a new context id for the communicator */
	newctx = MPIR_Get_contextid(comm, mpig_intercomm_exchange_contextid, conn);
	MPIU_ERR_CHKANDSTMT((newctx == 0), mpi_errno, MPI_ERR_OTHER, {goto endblk;}, "**globus|connacc|ctxalloc");

      endblk:
	/* send remote VC data to the other processes in the local communicator */
	if (mpi_errno == MPI_SUCCESS)
	{
	    mpi_errno = NMPI_Bcast(&remote_vct_size, 1, MPI_INT, root, comm->handle);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|connacc|bcast_vct_size");
	    mpi_errno = NMPI_Bcast(remote_vct_buf, remote_vct_size, MPI_BYTE, root, comm->handle);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|connacc|bcast_vct_data");
	}
	else
	{
	    remote_vct_size = -1;
	    mrc = NMPI_Bcast(&remote_vct_size, 1, MPI_INT, root, comm->handle);
	    MPIU_ERR_CHKANDSTMT((mrc), mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc); goto fn_fail;},
		"**globus|connacc|bcast_vct_size_error");
	    goto fn_fail;
	}
    }
    else
    {
	/* allocate a new context id for the communicator */
	newctx = MPIR_Get_contextid(comm, mpig_intercomm_exchange_contextid, NULL);
	MPIU_ERR_CHKANDSTMT((newctx == 0), mpi_errno, MPI_ERR_OTHER, {goto endblk;}, "**globus|connacc|ctxalloc");
	    
	/* receive remote VC data from the root of the local communicator */
	mrc = NMPI_Bcast(&remote_vct_size, 1, MPI_INT, root, comm->handle);

	remote_vct_buf = (char *) MPIU_Malloc(remote_vct_size);
	if (remote_vct_buf)
	{
	    mrc = NMPI_Bcast(remote_vct_buf, remote_vct_size, MPI_BYTE, root, comm->handle);
	}
	else
	{
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive buffer for remote VC objects");
	    remote_vct_size = 0;
	    NMPI_Bcast(remote_vct_buf, remote_vct_size, MPI_BYTE, root, comm->handle);
	    /* FIXME: let root know that a error has occurred.  see status exchange below. */
	    goto fn_fail;
	}
    }
    
    /* deserialize the remote VC data, creating additional process groups as necessary.  during this process, construct a remote
       VCRT for new intercommunicator */

    
    /* FIXME: gather status from local comm, exchange status with remote root, and broadcast status to local comm so that all
       processes know the creation of the intercommunciator was successful */

  fn_return:
    if (conn)
    {
	mpig_connection_close(conn);
    }

    MPIR_Nest_decr();
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_COMM_ACCEPT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (newctx != 0)
    {
	MPIR_Free_contextid(newctx);
    }
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
    mpig_connection_t * conn = NULL;
    char * local_vct_buf;
    int local_vct_size;
    char * remote_vct_buf;
    int remote_vct_size;
    int newctx = 0;
    int mrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_COMM_CONNECT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_COMM_CONNECT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering"));

    MPIR_Nest_incr();
    
    /* create a new communicator structure */

    if (root)
    {
	/* create an array of process containing the process group id, process group rank, and serialized business card for each
	   VC in the virtual connection reference table (VCRT) attached to the supplied communicator */

	/* attempt to connect to the port */
	mpi_errno = mpig_port_connect(port_name, comm->remote_size, local_vct_size, &remote_vct_size, &conn);
	MPIU_ERR_CHKANDSTMT((mpi_errno), mpi_errno, MPI_ERR_OTHER, {goto endblk;}, "**globus|connacc|conn_accept");

	/* receive serialized VC data from the accepting process */
	remote_vct_buf = (char *) MPIU_Malloc(remote_vct_size);
	MPIU_ERR_CHKANDSTMT1((remote_vct_buf == NULL), mpi_errno, MPI_ERR_OTHER, {goto endblk;}, "**nomem",
	    "**nomem %s", "receive buffer for remote VC objects");
	
	/* send the serialized VC data to the accepting process */
	mpi_errno = mpig_connection_send(conn, local_vct_buf, local_vct_size);
	MPIU_ERR_CHKANDSTMT((mpi_errno), mpi_errno, MPI_ERR_OTHER, {goto endblk;}, "**globus|connacc|conn_send");

	mpi_errno = mpig_connection_recv(conn, remote_vct_buf, remote_vct_size);
	MPIU_ERR_CHKANDSTMT((mpi_errno), mpi_errno, MPI_ERR_OTHER, {goto endblk;}, "**globus|connacc|conn_recv");
	
	/* allocate a new context id for the communicator */
	newctx = MPIR_Get_contextid(comm, mpig_intercomm_exchange_contextid, conn);
	MPIU_ERR_CHKANDSTMT((newctx == 0), mpi_errno, MPI_ERR_OTHER, {goto endblk;}, "**globus|connacc|ctxalloc");

      endblk:
	/* send remote VC data to the other processes in the local communicator */
	if (mpi_errno == MPI_SUCCESS)
	{
	    mpi_errno = NMPI_Bcast(&remote_vct_size, 1, MPI_INT, root, comm->handle);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|connacc|bcast_vct_size");
	    mpi_errno = NMPI_Bcast(remote_vct_buf, remote_vct_size, MPI_BYTE, root, comm->handle);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|connacc|bcast_vct_data");
	}
	else
	{
	    remote_vct_size = -1;
	    mrc = NMPI_Bcast(&remote_vct_size, 1, MPI_INT, root, comm->handle);
	    MPIU_ERR_CHKANDSTMT((mrc), mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc); goto fn_fail;},
		"**globus|connacc|bcast_vct_size_error");
	    goto fn_fail;
	}
    }
    else
    {
	/* allocate a new context id for the communicator */
	newctx = MPIR_Get_contextid(comm, mpig_intercomm_exchange_contextid, NULL);
	MPIU_ERR_CHKANDSTMT((newctx == 0), mpi_errno, MPI_ERR_OTHER, {goto endblk;}, "**globus|connacc|ctxalloc");
	    
	/* receive remote VC data from the root of the local communicator */
	mrc = NMPI_Bcast(&remote_vct_size, 1, MPI_INT, root, comm->handle);

	remote_vct_buf = (char *) MPIU_Malloc(remote_vct_size);
	if (remote_vct_buf)
	{
	    mrc = NMPI_Bcast(remote_vct_buf, remote_vct_size, MPI_BYTE, root, comm->handle);
	}
	else
	{
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive buffer for remote VC objects");
	    remote_vct_size = 0;
	    NMPI_Bcast(remote_vct_buf, remote_vct_size, MPI_BYTE, root, comm->handle);
	    /* FIXME: let root know that a error has occurred.  see status exchange below. */
	    goto fn_fail;
	}
    }
    
    /* deserialize the remote VC data, creating additional process groups as necessary.  during this process, construct a remote
       VCRT for new intercommunicator */

    
    /* FIXME: gather status from local comm, exchange status with remote root, and broadcast status to local comm so that all
       processes know the creation of the intercommunciator was successful */

  fn_return:
    if (conn)
    {
	mpig_connection_close(conn);
    }

    MPIR_Nest_decr();
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_COMM_CONNECT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (newctx != 0)
    {
	MPIR_Free_contextid(newctx);
    }
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Comm_connect() */
