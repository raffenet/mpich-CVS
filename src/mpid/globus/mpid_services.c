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
int MPID_Comm_accept(char * const port_name, MPID_Info * const info, const int root, MPID_Comm * const comm,
    MPID_Comm ** const newcomm_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Comm * newcomm = NULL;
    mpig_vc_t * port_vc = NULL;
    char * local_vct_str = NULL;
    unsigned local_vct_len = 0;
    char * remote_vct_str = NULL;
    unsigned remote_vct_len = 0;
    int local_ctx = 0;
    int remote_ctx = 0;
    mpig_vcrt_t * vcrt = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_COMM_ACCEPT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_COMM_ACCEPT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering: port_name=%s, root=%d"
	", comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, port_name, root, comm->handle, MPIG_PTR_CAST(comm)));

    MPIR_Nest_incr();
    
    /* allocate a new communicator structure */
    mpi_errno = MPIR_Comm_create(&newcomm);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: unable to allocate an intercommunicator:"
	    "port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem",  "**nomem %s", "intercommunicator");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    /* allocate a new context id for the communicator.  if an error occurs while allocating a context id, all processes in the
       local communicator should be aware of the error. */
    local_ctx = MPIR_Get_contextid(comm);
    if (local_ctx == 0)
    {   /* --BEGIN ERROR HANDLING-- */
        MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: failed to allocate a new context "
            "id: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
        MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|ctxalloc", "**globus|connacc|ctxalloc %p", port_vc);
        goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DYNAMIC, "new context id allocated for the intercommunicator, port_vc=" MPIG_PTR_FMT
	",local_ctx=%d", MPIG_PTR_CAST(port_vc), local_ctx));

    if (comm->rank == root)
    {
	/* create an array containing the process group id, process group rank, and serialized business card for each VC in the
	   virtual connection reference table (VCRT) attached to the supplied communicator */
	mpi_errno = mpig_vcrt_serialize_object(comm->vcrt, &local_vct_str);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: serializing the VCRT failed: "
		"port_name=%s, comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, port_name, comm->handle, MPIG_PTR_CAST(comm)));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|vcrt_serialize",
		"**globus|connacc|vcrt_serialize %s %C %p", port_name, comm->handle, comm->vcrt);
	    goto endblk;
	}   /* --END ERROR HANDLING-- */

	local_vct_len = strlen(local_vct_str) + 1;

	/* accept a new connection for the port */
	mpi_errno = mpig_port_accept(port_name, &port_vc);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: accepting a new connection failed:"
		"port_name=%s", port_name));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_accept",
		"**globus|connacc|port_accept %s %i %C", port_name, root, comm->handle);
	    goto endblk;
	}   /* --END ERROR HANDLING-- */

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DYNAMIC, "port accept succeeded: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	
	/* exchange information about the length of the VCT string */
	{
	    char buf[sizeof(int32_t) + sizeof(u_int32_t)];
	    int32_t tmp_int32;
	    u_int32_t tmp_uint32;

	    tmp_int32 = local_ctx;
	    mpig_dc_put_int32(buf, tmp_int32);
	    tmp_uint32 = local_vct_len;
	    mpig_dc_put_uint32(buf + sizeof(int32_t), tmp_uint32);
	    
	    mpi_errno = mpig_port_vc_send(port_vc, buf, sizeof(buf));
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: sending local context id and "
                    "VCT string length failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
		MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_send",
		    "**globus|connacc|port_vc_send %p %p %d", port_vc, buf, sizeof(buf));
		goto endblk;
	    }   /* --END ERROR HANDLING-- */

	    mpi_errno = mpig_port_vc_recv(port_vc, buf, sizeof(buf));
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: receiving remote context id and "
                    "VCT string length failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
		MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_recv",
		    "**globus|connacc|port_vc_recv %p %p %d", port_vc, buf, sizeof(buf));
		goto endblk;
	    }   /* --END ERROR HANDLING-- */

	    mpig_dc_get_int32(mpig_port_vc_get_endian(port_vc), buf, &tmp_int32);
	    remote_ctx = tmp_int32;
	    mpig_dc_get_uint32(mpig_port_vc_get_endian(port_vc), buf + sizeof(int32_t), &tmp_uint32);
	    remote_vct_len = tmp_uint32;

	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DYNAMIC, "remote nprocs and VCT string length received: remote_ctx=%d, port_vc="
                MPIG_PTR_FMT ", remote_vct_len=%u", MPIG_PTR_CAST(port_vc), remote_ctx, remote_vct_len));
	}
	
	/* allocate a buffer for the remote VCT string */
	remote_vct_str = (char *) MPIU_Malloc(remote_vct_len);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: unable to allocate a buffer to hold the "
		"remote VCT string: port_vc=" MPIG_PTR_FMT ", len=%u", MPIG_PTR_CAST(port_vc), remote_vct_len));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem",  "**nomem %s", "receive buffer for remote VCT string");
	    goto endblk;
	}   /* --END ERROR HANDLING-- */
	
	/* send the local VCT string to the connecting process */
	mpi_errno = mpig_port_vc_send(port_vc, local_vct_str, local_vct_len);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: sending local VCT string failed: "
		"port_vc=" MPIG_PTR_FMT ",str=" MPIG_PTR_FMT ", len=%u", MPIG_PTR_CAST(port_vc), MPIG_PTR_CAST(local_vct_str),
		local_vct_len));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_send",
		"**globus|connacc|port_vc_send %p %p %d", port_vc, local_vct_str, (int) local_vct_len);
	    goto endblk;
	}   /* --END ERROR HANDLING-- */

	mpig_vcrt_free_serialized_object(local_vct_str);

	/* receive the remote VCT string from the connecting process */
	mpi_errno = mpig_port_vc_recv(port_vc, remote_vct_str, remote_vct_len);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: receiving remote VCT string failed: "
		"port_vc=" MPIG_PTR_FMT ", str=" MPIG_PTR_FMT ", len=%u", MPIG_PTR_CAST(port_vc),
		MPIG_PTR_CAST(remote_vct_str), remote_vct_len));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_recv",
		"**globus|connacc|port_vc_recv %p %p %d", port_vc, remote_vct_str, (int) remote_vct_len);
	    goto endblk;
	}   /* --END ERROR HANDLING-- */
	
      endblk:
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    int mrc;
	    
	    remote_vct_len = 0;
	    mrc = NMPI_Bcast(&remote_vct_len, 1, MPI_UNSIGNED, root, comm->handle);
	    if (mrc)
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: broadcast of an error condition "
		    "to processes in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
		MPIU_ERR_SETANDSTMT1(mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc);},
		    "**globus|connacc|bcast_vct_error_status", "**globus|connacc|bcast_vct_error_status %p", port_vc);
		goto fn_fail;
	    }
	    
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	/* send remote VC string to the other processes in the local communicator */
	mpi_errno = NMPI_Bcast(&remote_vct_len, 1, MPI_UNSIGNED, root, comm->handle);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: broadcast of remote VCT string length "
		"to processes in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|bcast_vct_len",
		"**globus|connacc|bcast_vct_len %p", port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	mpi_errno = NMPI_Bcast(remote_vct_str, (int) remote_vct_len, MPI_BYTE, root, comm->handle);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: broadcast of remote VCT string to "
		"processes in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|bcast_vct_str",
		"**globus|connacc|bcast_vct_str %p %p %d", port_vc, remote_vct_str, remote_vct_len);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
    }
    else /* not the root */
    {
	/* receive the remote VCT string length from the root of the local communicator */
	mpi_errno = NMPI_Bcast(&remote_vct_len, 1, MPI_UNSIGNED, root, comm->handle);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: reception of remote VCT string length "
		"from the root process in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|bcast_vct_len", "**globus|connacc|bcast_vct_len %p",
		port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	if (remote_vct_len == 0)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: communication between the two root "
		"processes failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|bcast_vct_error_status_recvd",
		"**globus|connacc|bcast_vct_error_status_recvd %p", port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
	
	/* allocate a buffer for the remote VCT string */
	remote_vct_str = (char *) MPIU_Malloc(remote_vct_len);
	if (remote_vct_str == NULL)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: unable to allocate a buffer to hold "
		"the remote string: port_vc=" MPIG_PTR_FMT ", len=%u", MPIG_PTR_CAST(port_vc), remote_vct_len));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem",  "**nomem %s", "receive buffer for remote VC objects");
	    remote_vct_str = 0;
	}   /* --END ERROR HANDLING-- */
	
	/* receive the remote VCT string length from the root of the local communicator */
	mpi_errno = NMPI_Bcast(remote_vct_str, (int) remote_vct_len, MPI_BYTE, root, comm->handle);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: reception of  remote VCT string from "
		"the root process in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|bcast_vct_str",
		"**globus|connacc|bcast_vct_str %p %p %d", port_vc, remote_vct_str, remote_vct_len);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
    }
    /* to be the root or not to be the root, that is the question */
    
    /* deserialize the remote VC string, creating additional process groups as necessary.  during this process, construct a remote
       VCRT for new intercommunicator */
    mpi_errno = mpig_vcrt_deserialize_object(remote_vct_str, &vcrt);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: serializing the VCRT failed: "
	    "port_name=%s, comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, port_name, comm->handle, MPIG_PTR_CAST(comm)));
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|vcrt_deserialize");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    
    /* FT-FIXME: gather status from local comm, exchange status with remote root, and broadcast status to local comm so that all
       processes know the creation of the intercommunciator was successful */

    /* complete the contruction of the intercommunicator */
    newcomm->comm_kind = MPID_INTERCOMM;
    newcomm->context_id = remote_ctx;
    newcomm->recvcontext_id = local_ctx;

    newcomm->local_size = comm->remote_size;
    newcomm->rank = comm->rank;
    MPID_VCRT_Add_ref(comm->vcrt);
    newcomm->local_vcrt = comm->vcrt;
    newcomm->local_vcr = comm->vcr;

    newcomm->remote_size = mpig_vcrt_size(vcrt);
    newcomm->vcrt = vcrt;
    MPID_VCRT_Get_ptr(newcomm->vcrt, &newcomm->vcr);

    newcomm->is_low_group = TRUE;
    newcomm->local_comm = NULL;

    /* Notify the device of this new communicator */
    MPID_Dev_comm_create_hook(newcomm);

    /* return the new communicator to the calling routine */
    *newcomm_p = newcomm;

  fn_return:
    if (port_vc)
    {
	mpig_port_vc_close(port_vc);
    }

    MPIR_Nest_decr();
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting: port_name=%s, root=%d"
	", comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT ", newcomm=" MPIG_PTR_FMT, port_name, root, comm->handle,
	MPIG_PTR_CAST(comm), MPIG_PTR_CAST(newcomm)));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_COMM_ACCEPT);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	if (newcomm->vcrt != NULL)
	{
	    MPID_VCRT_Release(newcomm->vcrt, TRUE);
	    MPID_VCRT_Release(newcomm->local_vcrt, TRUE);
	}
	
	if (local_ctx != 0)
	{
	    MPIR_Free_contextid(local_ctx);
	}
	
	if (newcomm != NULL)
	{
	    MPIU_Handle_obj_free(&MPID_Comm_mem, newcomm);
	    newcomm = NULL;
	}
	
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* MPID_Comm_accept() */


/*
 * mpid_comm_connect()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Comm_connect
int MPID_Comm_connect(const char * const port_name, MPID_Info * const info, const int root, MPID_Comm * const comm,
    MPID_Comm ** const newcomm_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Comm * newcomm = NULL;
    mpig_vc_t * port_vc = NULL;
    char * local_vct_str = NULL;
    unsigned local_vct_len = 0;
    char * remote_vct_str = NULL;
    unsigned remote_vct_len = 0;
    int local_ctx = 0;
    int remote_ctx = 0;
    mpig_vcrt_t * vcrt = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_COMM_CONNECT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_COMM_CONNECT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering: port_name=%s, root=%d"
	", comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, port_name, root, comm->handle, MPIG_PTR_CAST(comm)));

    MPIR_Nest_incr();
    
    /* allocate a new communicator structure */
    mpi_errno = MPIR_Comm_create(&newcomm);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: unable to allocate an intercommunicator:"
	    "port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem",  "**nomem %s", "intercommunicator");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    
    /* allocate a new context id for the communicator.  if an error occurs while allocating a context id (and the
       MPIR_Get_contextid routine returns), all processes should be aware of the error and thus it should be safe for all
       processes to exit the routine without any further communication to verify the error. */
    local_ctx = MPIR_Get_contextid(comm);
    if (local_ctx == 0)
    {   /* --BEGIN ERROR HANDLING-- */
        MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: failed to allocate a new context id: "
            "port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
        MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|ctxalloc", "**globus|connacc|ctxalloc %p", port_vc);
        goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DYNAMIC, "new context id allocated for the intercommunicator, port_vc=" MPIG_PTR_FMT
	", local_ctx=%d", MPIG_PTR_CAST(port_vc), local_ctx));

    if (comm->rank == root)
    {
	/* create an array containing the process group id, process group rank, and serialized business card for each VC in the
	   virtual connection reference table (VCRT) attached to the supplied communicator */
	mpi_errno = mpig_vcrt_serialize_object(comm->vcrt, &local_vct_str);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: serializing the VCRT failed: "
		"port_name=%s, comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, port_name, comm->handle, MPIG_PTR_CAST(comm)));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|vcrt_serialize",
		"**globus|connacc|vcrt_serialize %s %C %p", port_name, comm->handle, comm->vcrt);
	    goto endblk;
	}   /* --END ERROR HANDLING-- */

	local_vct_len = strlen(local_vct_str) + 1;

	/* connect to the provided port */
	mpi_errno = mpig_port_connect(port_name, &port_vc);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: connecting to the port failed:"
		"port_name=%s", port_name));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_connect",
		"**globus|connacc|port_connect %s %i %C", port_name, root, comm->handle);
	    goto endblk;
	}   /* --END ERROR HANDLING-- */

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DYNAMIC, "port connect succeeded: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	
	/* exchange information about the length of the VCT string */
	{
	    char buf[sizeof(int32_t) + sizeof(u_int32_t)];
	    int32_t tmp_int32;
	    u_int32_t tmp_uint32;

	    mpi_errno = mpig_port_vc_recv(port_vc, buf, sizeof(buf));
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: receiving remote context id and "
                    "VCT string length failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
		MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_recv",
		    "**globus|connacc|port_vc_recv %p %p %d", port_vc, buf, sizeof(buf));
		goto endblk;
	    }   /* --END ERROR HANDLING-- */

	    mpig_dc_get_int32(mpig_port_vc_get_endian(port_vc), buf, &tmp_int32);
	    remote_ctx = tmp_int32;
	    mpig_dc_get_uint32(mpig_port_vc_get_endian(port_vc), buf + sizeof(int32_t), &tmp_uint32);
	    remote_vct_len = tmp_uint32;

	    tmp_int32 = local_ctx;
	    mpig_dc_put_int32(buf, tmp_int32);
	    tmp_uint32 = local_vct_len;
	    mpig_dc_put_uint32(buf + sizeof(int32_t), tmp_uint32);
	    
	    mpi_errno = mpig_port_vc_send(port_vc, buf, sizeof(buf));
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: sending local context id and "
                    "VCT string length failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
		MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_send",
		    "**globus|connacc|port_vc_send %p %p %d", port_vc, buf, sizeof(buf));
		goto endblk;
	    }   /* --END ERROR HANDLING-- */

	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DYNAMIC, "remote nprocs and VCT string length received: remote_ctx=%d, port_vc="
                MPIG_PTR_FMT ", remote_vct_len=%u", MPIG_PTR_CAST(port_vc), remote_ctx, remote_vct_len));
	}
	
	/* allocate a buffer for the remote VCT string */
	remote_vct_str = (char *) MPIU_Malloc(remote_vct_len);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: unable to allocate a buffer to hold the "
		"remote VCT string: port_vc=" MPIG_PTR_FMT ", len=%u", MPIG_PTR_CAST(port_vc), remote_vct_len));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem",  "**nomem %s", "receive buffer for remote VCT string");
	    goto endblk;
	}   /* --END ERROR HANDLING-- */
	
	/* receive the remote VCT string from the accepting process */
	mpi_errno = mpig_port_vc_recv(port_vc, remote_vct_str, remote_vct_len);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: receiving remote VCT string failed: "
		"port_vc=" MPIG_PTR_FMT ", str=" MPIG_PTR_FMT ", len=%u", MPIG_PTR_CAST(port_vc),
		MPIG_PTR_CAST(remote_vct_str), remote_vct_len));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_recv",
		"**globus|connacc|port_vc_recv %p %p %d", port_vc, remote_vct_str, (int) remote_vct_len);
	    goto endblk;
	}   /* --END ERROR HANDLING-- */
	
	/* send the local VCT string to the accepting process */
	mpi_errno = mpig_port_vc_send(port_vc, local_vct_str, local_vct_len);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: sending local VCT string failed: "
		"port_vc=" MPIG_PTR_FMT ",str=" MPIG_PTR_FMT ", len=%u", MPIG_PTR_CAST(port_vc), MPIG_PTR_CAST(local_vct_str),
		local_vct_len));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_send",
		"**globus|connacc|port_vc_send %p %p %d", port_vc, local_vct_str, (int) local_vct_len);
	    goto endblk;
	}   /* --END ERROR HANDLING-- */

	mpig_vcrt_free_serialized_object(local_vct_str);
	
      endblk:
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    int mrc;
	    
	    remote_vct_len = 0;
	    mrc = NMPI_Bcast(&remote_vct_len, 1, MPI_UNSIGNED, root, comm->handle);
	    if (mrc)
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: broadcast of an error condition "
		    "to processes in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
		MPIU_ERR_SETANDSTMT1(mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc);},
		    "**globus|connacc|bcast_vct_error_status", "**globus|connacc|bcast_vct_error_status %p", port_vc);
		goto fn_fail;
	    }
	    
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	/* send remote VC string to the other processes in the local communicator */
	mpi_errno = NMPI_Bcast(&remote_vct_len, 1, MPI_UNSIGNED, root, comm->handle);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: broadcast of remote VCT string length "
		"to processes in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|bcast_vct_len",
		"**globus|connacc|bcast_vct_len %p", port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	mpi_errno = NMPI_Bcast(remote_vct_str, (int) remote_vct_len, MPI_BYTE, root, comm->handle);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: broadcast of remote VCT string to "
		"processes in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|bcast_vct_str",
		"**globus|connacc|bcast_vct_str %p %p %d", port_vc, remote_vct_str, remote_vct_len);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
    }
    else /* not the root */
    {
	/* receive the remote VCT string length from the root of the local communicator */
	mpi_errno = NMPI_Bcast(&remote_vct_len, 1, MPI_UNSIGNED, root, comm->handle);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: reception of remote VCT string length "
		"from the root process in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|bcast_vct_len", "**globus|connacc|bcast_vct_len %p",
		port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	if (remote_vct_len == 0)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: communication between the two root "
		"processes failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|bcast_vct_error_status_recvd",
		"**globus|connacc|bcast_vct_error_status_recvd %p", port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
	
	/* allocate a buffer for the remote VCT string */
	remote_vct_str = (char *) MPIU_Malloc(remote_vct_len);
	if (remote_vct_str == NULL)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: unable to allocate a buffer to hold "
		"the remote string: port_vc=" MPIG_PTR_FMT ", len=%u", MPIG_PTR_CAST(port_vc), remote_vct_len));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem",  "**nomem %s", "receive buffer for remote VC objects");
	    remote_vct_str = 0;
	}   /* --END ERROR HANDLING-- */
	
	/* receive the remote VCT string length from the root of the local communicator */
	mpi_errno = NMPI_Bcast(remote_vct_str, (int) remote_vct_len, MPI_BYTE, root, comm->handle);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: reception of  remote VCT string from "
		"the root process in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|bcast_vct_str",
		"**globus|connacc|bcast_vct_str %p %p %d", port_vc, remote_vct_str, remote_vct_len);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

    }
    /* to be the root or not to be the root, that is the question */
    
    /* deserialize the remote VC string, creating additional process groups as necessary.  during this process, construct a remote
       VCRT for new intercommunicator */
    mpi_errno = mpig_vcrt_deserialize_object(remote_vct_str, &vcrt);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: serializing the VCRT failed: "
	    "port_name=%s, comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, port_name, comm->handle, MPIG_PTR_CAST(comm)));
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|vcrt_deserialize");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    
    /* FT-FIXME: gather status from local comm, exchange status with remote root, and broadcast status to local comm so that all
       processes know the creation of the intercommunciator was successful */

    /* complete the contruction of the intercommunicator */
    newcomm->comm_kind = MPID_INTERCOMM;
    newcomm->context_id = remote_ctx;
    newcomm->recvcontext_id = local_ctx;

    newcomm->local_size = comm->remote_size;
    newcomm->rank = comm->rank;
    MPID_VCRT_Add_ref(comm->vcrt);
    newcomm->local_vcrt = comm->vcrt;
    newcomm->local_vcr = comm->vcr;

    newcomm->remote_size = mpig_vcrt_size(vcrt);
    newcomm->vcrt = vcrt;
    MPID_VCRT_Get_ptr(newcomm->vcrt, &newcomm->vcr);

    newcomm->is_low_group = FALSE;
    newcomm->local_comm = NULL;

    /* Notify the device of this new communicator */
    MPID_Dev_comm_create_hook(newcomm);

    /* return the new communicator to the calling routine */
    *newcomm_p = newcomm;

  fn_return:
    if (port_vc)
    {
	mpig_port_vc_close(port_vc);
    }

    MPIR_Nest_decr();
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting: port_name=%s, root=%d"
	", comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT ", newcomm=" MPIG_PTR_FMT, port_name, root, comm->handle,
	MPIG_PTR_CAST(comm), MPIG_PTR_CAST(newcomm)));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_COMM_CONNECT);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	if (newcomm->vcrt != NULL)
	{
	    MPID_VCRT_Release(newcomm->vcrt, TRUE);
	    MPID_VCRT_Release(newcomm->local_vcrt, TRUE);
	}
	
	if (local_ctx != 0)
	{
	    MPIR_Free_contextid(local_ctx);
	}
	
	if (newcomm != NULL)
	{
	    MPIU_Handle_obj_free(&MPID_Comm_mem, newcomm);
	    newcomm = NULL;
	}
	
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* MPID_Comm_connect() */


/*
 * mpid_Comm_disconnect()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Comm_connect
int MPID_Comm_disconnect(MPID_Comm * const comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_COMM_DISCONNECT);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_VAR(comm);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_COMM_DISCONNECT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering: comm=" MPIG_HANDLE_FMT
        ", commp=" MPIG_PTR_FMT, comm->handle, MPIG_PTR_CAST(comm)));

    /* ... nothing to do right now.  may need to worry about disconnecting stdout/err forwarding later. ... */

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting: comm=" MPIG_HANDLE_FMT
        ", commp=" MPIG_PTR_FMT, comm->handle, MPIG_PTR_CAST(comm)));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_COMM_DISCONNECT);
    return mpi_errno;
}
/* MPID_Comm_disconnect() */



