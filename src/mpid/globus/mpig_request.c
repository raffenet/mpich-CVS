/*
 * Globus device code:       Copyright 2005 Northern Illinois University
 * Borrowed CH3 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

#ifndef MPID_REQUEST_PREALLOC
#define MPID_REQUEST_PREALLOC 32
#endif

MPID_Request MPID_Request_direct[MPID_REQUEST_PREALLOC];
MPIU_Object_alloc_t MPID_Request_mem = {
    0, 0, 0, 0, MPID_REQUEST, sizeof(MPID_Request), MPID_Request_direct,
    MPID_REQUEST_PREALLOC };


#undef FUNCNAME
#define FUNCNAME mpig_request_create
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPID_Request * mpig_request_create()
{
    MPID_Request * req;
    MPIG_STATE_DECL(MPID_STATE_mpig_request_create);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_request_create);
    
    req = MPIU_Handle_obj_alloc(&MPID_Request_mem);
    if (req != NULL)
    {
	MPIU_Assert(HANDLE_GET_MPI_KIND((req)->handle) == MPID_REQUEST);

	/*
	 * Initialize critical fields
	 */
	mpig_request_lock_create(rreq);
	req->kind = MPID_REQUEST_UNDEFINED;
	req->comm = NULL;
	req->dev.dtp = NULL;
	req->dev.next = NULL;
    }
    else
    {
	/* --BEGIN ERROR HANDLING-- */
	MPIG_DBG_PRINTF((1, FCNAME, "unable to allocate a request"));
	goto fn_exit;
	/* --END ERROR HANDLING-- */
    }

  fn_exit:
    MPIG_FUNC_EXIT(MPID_STATE_mpig_request_create);
    return req;
}
/* mpig_request_create() */


#undef FUNCNAME
#define FUNCNAME mpig_request_destroy
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
void mpig_request_destroy(MPID_Request * req)
{
    MPIG_STATE_DECL(MPID_STATE_MPID_REQUEST_DESTROY);
    
    MPIG_FUNC_ENTER(MPID_STATE_MPID_REQUEST_DESTROY);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIG_DBG_PRINTF((30, FCNAME, "destroying request, handle=0x%08x, ptr=%p", req->handle, req));
    MPIU_Assert(HANDLE_GET_MPI_KIND((req)->handle) == MPID_REQUEST);
    MPIU_Assert((req)->ref_count == 0);

    /*
     * Free any resources used by the request
     */
    if (req->comm != NULL)
    {
	MPIR_Comm_release(req->comm);
    }
    
    if (req->dev.dtp != NULL)
    {
	MPID_Datatype_release(req->dev.dtp);
    }

    mpig_request_lock_destroy(rreq);

    
    MPIU_Handle_obj_free(&MPID_Request_mem, req);
    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_REQUEST_DESTROY);
}
/* mpig_request_destroy() */
