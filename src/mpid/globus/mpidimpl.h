/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIDIMPL_H_INCLUDED)
#define MPICH2_MPIDIMPL_H_INCLUDED


#include "mpiimpl.h"
#include "globus_module.h"
#include "globus_common.h"
#include "globus_dc.h"

/*
 * When memory tracing is enabled, mpimem.h redefines malloc(), calloc(), and free() to be invalid statements.  These
 * redefinitions have a negative interaction with the Globus heap routines which may be mapped (using C preprocessor defines)
 * directly to the heap routines supplied as part of the the operating system.  This is particularly a problem when a Globus
 * library routine returns an allocated object and expects to caller to free the object.  One cannot use MPIU_Free() to free the
 * object since MPIU_Free() is expecting a pointer to memory allocated by the memory tracing module.  Therefore, it is necessary
 * to remove the redefinitions of the heap routines, allowing the Globus heap routines to map directly to those provided by the
 * operating system.
 */
#if defined(USE_MEMORY_TRACING)
#undef malloc
#undef calloc
#undef realloc
#undef free
#endif

/*
 * Function enter/exit macro, used primarily for logging, but can be used for other things.
 */
#define FCNAME fcname

#if defined(MPIG_DEBUG)
#define MPIG_STATE_DECL(a_)
#define MPIG_FUNC_ENTER(a_) MPIG_FUNCNAME_CHECK()
#define MPIG_FUNC_EXIT(a_) MPIG_FUNCNAME_CHECK()
#define MPIG_RMA_FUNC_ENTER(a_) MPIG_FUNCNAME_CHECK()
#define MPIG_RMA_FUNC_EXIT(a_) MPIG_FUNCNAME_CHECK()
#else
#define MPIG_STATE_DECL(a_) MPIDI_STATE_DECL(a_)
#define MPIG_FUNC_ENTER(a_) MPIDI_FUNC_ENTER(a_)
#define MPIG_FUNC_EXIT(a_) MPIDI_FUNC_EXIT(a_)
#define MPIG_RMA_FUNC_ENTER(a_) MPIDI_RMA_FUNC_ENTER(a_)
#define MPIG_RMA_FUNC_EXIT(a_) MPIDI_RMA_FUNC_EXIT(a_)
#endif

#define MPIG_QUOTE(a_) MPIG_QUOTE2(a_)
#define MPIG_QUOTE2(a_) #a_


#define MPIG_BOOL_STR(b_) ((b_) ? "TRUE" : "FALSE")


#define MPIG_MIN(a_, b_) ((a_) <= (b_) ? (a_) : (b_))
#define MPIG_MAX(a_, b_) ((a_) >= (b_) ? (a_) : (b_))


#if defined(NDEBUG)
#define MPIG_STATIC static
#else
#define MPIG_STATIC
#endif


#define MPIG_UNUSED_ARG(arg_) {(void) arg_;}
#define MPIG_UNUSED_VAR(var_) {(void) var_;}


#if defined(HAVE_GETHOSTNAME) && defined(NEEDS_GETHOSTNAME_DECL) && !defined(gethostname)
int gethostname(char *name, size_t len);
# endif


/**********************************************************************************************************************************
						   BEGIN COMMUNICATOR SECTION
**********************************************************************************************************************************/
void mpig_comm_list_wait_empty(int * mpi_errno_p, bool_t * failed_p);

#define mpig_comm_construct(comm_)		\
{						\
    (comm_)->dev.user_ref = FALSE;		\
    (comm_)->dev.active_list_prev = NULL;	\
    (comm_)->dev.active_list_next = NULL;	\
}

#define mpig_comm_destruct(comm_)		\
{						\
    (comm_)->dev.user_ref = FALSE;		\
    (comm_)->dev.active_list_prev = NULL;	\
    (comm_)->dev.active_list_next = NULL;	\
}

#define mpig_comm_set_vc(comm_, rank_, vc_)	\
{						\
    (comm_)->vcr[(rank_)] = (vc_);		\
}
#define mpig_comm_get_vc(comm_, rank_, vc_p_)	\
{						\
    *(vc_p_) = (comm_)->vcr[(rank_)];		\
}

#define mpig_comm_get_my_vc(comm_, vc_p_)		\
{							\
    if ((comm_)->comm_kind == MPIR_INTRACOMM)		\
    {							\
	*(vc_p_) = (comm_)->vcr[(comm_)->rank];		\
    }							\
    else						\
    {							\
	*(vc_p_) = (comm_)->local_vcr[(comm_)->rank];	\
    }							\
}

/**********************************************************************************************************************************
						    END COMMUNICATOR SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						     BEGIN DATATYPE SECTION
**********************************************************************************************************************************/
void mpig_datatype_set_my_bc(mpig_bc_t * bc, int * mpi_errno_p, bool_t * failed_p);
void mpig_datatype_process_bc(const mpig_bc_t * bc, mpig_vc_t * vc, int * mpi_errno_p, bool_t * failed_p);
void mpig_datatype_get_src_ctype(const mpig_vc_t * vc, const MPID_Datatype dt, mpig_ctype_t * ctype);
void mpig_datatype_get_my_ctype(const MPI_Datatype dt, mpig_ctype_t * ctype);

#define mpig_datatype_get_src_ctype(vc_, dt_, ctype_)							\
{													\
    int dt_id__;											\
    													\
    MPIU_Assert(HANDLE_GET_KIND(dt_) == MPI_KIND_BUILTIN && HANDLE_GET_MPI_KIND(dt_) == MPID_DATATYPE);	\
    MPID_Datatype_get_basic_id(dt_, dt_id__);								\
    MPIU_Assert(dt_id__ < MPIG_DATATYPE_MAX_BASIC_TYPES);						\
    *(ctype_) = (mpig_ctype_t) (vc_)->dt_cmap[dt_id__];							\
}

#define mpig_datatype_get_my_ctype(dt_, ctype_)				\
{									\
    const mpig_vc_t * vc__;						\
									\
    mpig_pg_get_vc(mpig_process.my_pg, mpig_process.my_pg_rank, &vc__);	\
    mpig_datatype_get_src_ctype(vc__, (dt_), (ctype_));			\
}

#define mpig_datatype_get_info(/* MPI_Datatype */ dt_, /* bool_t */ dt_contig_, /* MPIU_Size_t */ dt_size_,		\
    /* MPIU_Size_t */ dt_nblks_, /* MPI_Aint */ dt_true_lb_)								\
{															\
    if (HANDLE_GET_KIND(dt_) == HANDLE_KIND_BUILTIN)									\
    {															\
	*(dt_contig_) = TRUE;												\
	*(dt_size_) = MPID_Datatype_get_basic_size(dt_);								\
        *(dt_nblks_) = 1;												\
        *(dt_true_lb_) = 0;												\
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "datatype - type=basic, dt_contig=%s, dt_size=" MPIG_SIZE_FMT		\
			   ", dt_nblks=" MPIG_SIZE_FMT ", dt_true_lb=" MPIG_AINT_FMT, MPIG_BOOL_STR(*(dt_contig_)),	\
			 (MPIU_Size_t) *(dt_size_), (MPIU_Size_t) *(dt_nblks_), (MPI_Aint) *(dt_true_lb_)));		\
    }															\
    else														\
    {															\
	MPID_Datatype * dtp__;												\
															\
	MPID_Datatype_get_ptr((dt_), dtp__);										\
	*(dt_contig_) = dtp__->is_contig ? TRUE : FALSE;								\
	*(dt_size_) = dtp__->size;											\
	*(dt_nblks_) = dtp__->n_contig_blocks;										\
        *(dt_true_lb_) = dtp__->true_lb;										\
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "datatype - type=user, dt_contig=%s, dt_size=" MPIG_SIZE_FMT		\
			   ", dt_nblks=" MPIG_SIZE_FMT ", dt_true_lb=" MPIG_AINT_FMT, MPIG_BOOL_STR(*(dt_contig_)),	\
			 (MPIU_Size_t) *(dt_size_), (MPIU_Size_t) *(dt_nblks_), (MPI_Aint) *(dt_true_lb_)));		\
    }															\
}
/**********************************************************************************************************************************
						      END DATATYPE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						      BEGIN REQUEST SECTION
**********************************************************************************************************************************/
/*
 * request allocation
 *
 * MT-NOTE: except for the the init and finalize routines, these request allocation routines are thread safe.
 */
extern globus_mutex_t mpig_request_alloc_mutex;

void mpig_request_alloc_init(void);

void mpig_request_alloc_finalize(void);

#define mpig_request_alloc(req_p_)				\
{								\
    /* acquire a request from the object allocation pool */	\
    globus_mutex_lock(&mpig_request_alloc_mutex);		\
    {								\
	*(req_p_) = MPIU_Handle_obj_alloc(&MPID_Request_mem);	\
    }								\
    globus_mutex_unlock(&mpig_request_alloc_mutex);		\
								\
    mpig_request_mutex_create(*(req_p_));			\
}

#define mpig_request_free(req_)						\
{									\
    mpig_request_mutex_destroy(req_);					\
									\
    /* release the request back to the object allocation pool */	\
    globus_mutex_lock(&mpig_request_alloc_mutex);			\
    {									\
	MPIU_Handle_obj_free(&MPID_Request_mem, (req_));		\
    }									\
    globus_mutex_unlock(&mpig_request_alloc_mutex);			\
}

/*
 * request functions
 */
const char * mpig_request_type_get_string(mpig_request_types_t req_type);

/*
 * request accessor macros
 *
 * MT-NOTE: these routines are not thread safe.  the calling routing is responsible for performing the necessary operations to
 * insure atomicity.
 *
 * MT-RC-NOTE: neither do these routines provide any coherence guarantees when a request is shared between multiple threads.  the
 * calling routine is responsible for performing the necessary mutex lock/unlock or RC acquire/release operations to insure that
 * changes made to the request object are made visible to other threads accessing the object.
 */
#define mpig_request_i_set_envelope(req_, rank_, tag_, ctx_)	\
{								\
    (req_)->dev.rank = (rank_);					\
    (req_)->dev.tag = (tag_);					\
    (req_)->dev.ctx = (ctx_);					\
}

#define mpig_request_set_envelope(req_, rank_, tag_, ctx_)								\
{															\
    mpig_request_i_set_envelope((req_), (rank_), (tag_), (ctx_));							\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request - set envelope: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT	\
		     ", rank=%d, tag=%d, ctx=%d", (req_)->handle, (MPIG_PTR_CAST) (req_), (req_)->dev.rank,		\
		     (req_)->dev.tag, (req_)->dev.ctx));								\
}

#define mpig_request_i_get_envelope(req_, rank_p_, tag_p_, ctx_p_)	\
{									\
    *(rank_p_) = (req_)->dev.rank;					\
    *(tag_p_) = (req_)->dev.tag;					\
    *(ctx_p_) = (req_)->dev.ctx;					\
}

#define mpig_request_get_envelope(req_, rank_p_, tag_p_, ctx_p_)							\
{															\
    mpig_request_i_get_envelope((req_), (rank_p_), (tag_p_), (ctx_p_));							\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request - get envelope: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT	\
		     ", rank=%d, tag=%d, ctx=%d", (req_)->handle, (MPIG_PTR_CAST) (req_), (req_)->dev.rank,		\
		     (req_)->dev.tag, (req_)->dev.ctx));								\
}

#define mpig_request_i_set_buffer(req_, buf_, cnt_, dt_)	\
{								\
    (req_)->dev.buf = (buf_);					\
    (req_)->dev.cnt = (cnt_);					\
    (req_)->dev.dt = (dt_);					\
}

#define mpig_request_set_buffer(req_, buf_, cnt_, dt_)									\
{															\
    mpig_request_i_set_buffer((req_), (buf_), (cnt_), (dt_));								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request - set buffer: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT	\
		       ", buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT, (req_)->handle, (MPIG_PTR_CAST) (req_),	\
		      (MPIG_PTR_CAST) (req_)->dev.buf, (req_)->dev.cnt, (req_)->dev.dt));				\
}

#define mpig_request_i_get_buffer(req_, buf_p_, cnt_p_, dt_p_)	\
{								\
    *(buf_p_) = (req_)->dev.buf;				\
    *(cnt_p_) = (req_)->dev.cnt;				\
    *(dt_p_) = (req_)->dev.dt;					\
}

#define mpig_request_get_buffer(req_, buf_p_, cnt_p_, dt_p_)								\
{															\
    mpig_request_i_get_buffer((req_), (buf_p_), (cnt_p_), (dt_p_));							\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request - get buffer: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT	\
		       ", buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT, (req_)->handle, (MPIG_PTR_CAST) (req_),	\
		     (MPIG_PTR_CAST) (req_)->dev.buf, (req_)->dev.cnt, (req_)->dev.dt));				\
}

#define mpig_request_get_dt(req_) ((req_)->dev.dt)

#define mpig_request_i_set_remote_req_id(req_, remote_req_id_)	\
{								\
    (req_)->dev.remote_req_id = (remote_req_id_);		\
}

#define mpig_request_set_remote_req_id(req_, remote_req_id_)								\
{															\
    mpig_request_i_set_remote_req_id((req_), (remote_req_id_));								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request - set remote req id: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT	\
		     ", id=" MPIG_HANDLE_FMT, (req_)->handle, (MPIG_PTR_CAST) (req_), (req_)->dev.remote_req_id));	\
}

#define mpig_request_get_remote_req_id(req_) ((req_)->dev.remote_req_id)

#define mpig_request_add_comm_ref(req_, comm_)	\
{						\
    if ((req_)->comm == NULL)			\
    {						\
	(req_)->comm = (comm_);			\
	MPIR_Comm_add_ref(comm_);		\
    }						\
}

#define mpig_request_release_comm_ref(req_)	\
{						\
    if (req->comm != NULL)			\
    {						\
	MPIR_Comm_release(req->comm);		\
        req->comm = NULL;			\
    }						\
}    

#define mpig_request_add_dt_ref(req_, dt_)											\
{																\
    if ((req_)->dev.dtp == NULL && HANDLE_GET_KIND(dt_) != HANDLE_KIND_BUILTIN && HANDLE_GET_KIND(dt_) != HANDLE_KIND_INVALID)	\
    {																\
	MPID_Datatype_get_ptr((dt_), (req_)->dev.dtp);										\
	MPID_Datatype_add_ref((req_)->dev.dtp);											\
    }																\
}

#define mpig_request_release_dt_ref(req_)	\
{						\
    if (req->dev.dtp != NULL)			\
    {						\
	MPID_Datatype_release(req->dev.dtp);	\
        req->dev.dtp = NULL;			\
    }						\
}

#define mpig_request_get_type(req_) ((req_)->dev.type)

#define mpig_request_i_set_type(req_, type_)	\
{						\
    (req_)->dev.type = (type_);			\
}

#define mpig_request_set_type(req_, type_)											\
{																\
    mpig_request_i_set_type((req_), (type_));											\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request - set request type: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT		\
		       ", type=%s",(req_)->handle, (MPIG_PTR_CAST) (req_), mpig_request_type_get_string((req_)->dev.type)));	\
}

#define mpig_request_i_set_vc(req_, vc_)	\
{						\
    (req_)->dev.vc = (vc_);			\
}

#define mpig_request_set_vc(req_, vc_)										\
{														\
    mpig_request_i_set_vc((req_), (vc_));									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request - set vc: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT	\
		       ", vc=" MPIG_PTR_FMT, (req_)->handle, (MPIG_PTR_CAST) (req_), (MPIG_PTR_CAST) (vc_)));	\
}

#define mpig_request_get_vc(req_) ((req_)->dev.vc)

#define mpig_request_set_cm_destruct_fn(req_, fn_)	\
{							\
    (req_)->dev.cm_destruct = (fn_);			\
}

/*
 * thread safety and release consistency macros
 */
#define mpig_request_mutex_create(req_)		globus_mutex_init(&(req_)->dev.mutex, NULL)
#define mpig_request_mutex_destroy(req_)	globus_mutex_destroy(&(req_)->dev.mutex)
#define mpig_request_mutex_lock(req_)										\
{														\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_REQ, "request - acquiring mutex: req="	\
		       MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT, (req_)->handle, (MPIG_PTR_CAST) (req_)));	\
    globus_mutex_lock(&(req_)->dev.mutex);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_REQ, "request - mutex acquired: req="	\
		       MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT, (req_)->handle, (MPIG_PTR_CAST) (req_)));	\
}
#define mpig_request_mutex_unlock(req_)										\
{														\
    globus_mutex_unlock(&(req_)->dev.mutex);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_REQ, "request - mutex released: req="	\
		       MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT, (req_)->handle, (MPIG_PTR_CAST) (req_)));	\
}

#define mpig_request_mutex_lock_conditional(req_, cond_)	{if (cond_) mpig_request_mutex_lock(req_);}
#define mpig_request_mutex_unlock_conditional(req_, cond_)	{if (cond_) mpig_request_mutex_unlock(req_);}

#define mpig_request_rc_acq(req_, needed_)	mpig_request_mutex_lock(req_)
#define mpig_request_rc_rel(req_, needed_)	mpig_request_mutex_unlock(req_)

/*
 * request construction and destruction
 *
 * MT-RC-NOTE: on release consistent systems, if the request object is to be used by any thread other than the one in which it
 * was created, then the construction of the object must be followed by a RC release or mutex unlock.  for systems using super
 * lazy release consistency models, such as the one used in Treadmarks, it would be necessary to perform an acquire/lock in the
 * mpig_vc_construct() immediately after creating the mutex.  The calling routine would need to perform a release after the temp
 * VC was completely initialized.  we have chosen to assume that none of the the systems upon which MPIG will run have that lazy
 * of a RC model.
 */
#define mpig_request_construct(req_)						\
{										\
    /* set MPICH fields */							\
    mpig_request_i_set_ref_count((req_), 1);					\
    (req_)->kind = MPID_REQUEST_UNDEFINED;					\
    (req_)->cc_ptr = &(req_)->cc;						\
    mpig_request_i_set_cc((req_), 0);						\
    (req_)->comm = NULL;							\
    (req_)->partner_request = NULL;						\
    MPIR_Status_set_empty(&(req_)->status);					\
    (req_)->status.MPI_ERROR = MPI_SUCCESS;					\
    (req_)->status.mpig_dc_format = NEXUS_DC_FORMAT_UNKNOWN;			\
										\
    /* set device fields */							\
    mpig_request_i_set_type((req_), MPIG_REQUEST_TYPE_UNDEFINED);		\
    mpig_request_i_set_buffer((req_), NULL, 0, MPI_DATATYPE_NULL);		\
    mpig_request_i_set_envelope((req_), MPI_PROC_NULL, MPI_ANY_TAG, -1);	\
    (req_)->dev.dtp = NULL;							\
    mpig_request_i_set_remote_req_id((req_), MPI_REQUEST_NULL);			\
    (req_)->dev.cm_destruct = NULL;						\
    mpig_request_i_set_vc((req_), NULL);					\
    (req_)->dev.next = NULL;							\
}

#define mpig_request_destruct(req_)							\
{											\
    /* call communication module's request destruct function */				\
    if ((req_)->dev.cm_destruct != NULL)						\
    {											\
	(req_)->dev.cm_destruct(req_);							\
	(req_)->dev.cm_destruct = NULL;							\
    }											\
											\
    /* release and communicator and datatype objects associated with the request */	\
    mpig_request_release_comm_ref(req_);						\
    mpig_request_release_dt_ref(req_);							\
											\
    /* resset MPICH fields */								\
    mpig_request_i_set_ref_count((req_), 0);						\
    (req_)->kind = MPID_REQUEST_UNDEFINED;						\
    (req_)->cc_ptr = NULL;								\
    mpig_request_i_set_cc((req_), 0);							\
    /* (req_)->comm = NULL; -- cleared by mpig_request_release_comm() */		\
    (req_)->partner_request = NULL;							\
    MPIR_Status_set_empty(&(req_)->status);						\
    (req_)->status.MPI_ERROR = MPI_ERR_INTERN;						\
    (req_)->status.mpig_dc_format = NEXUS_DC_FORMAT_UNKNOWN;				\
											\
    /* reset device fields */								\
    mpig_request_i_set_type((req_), MPIG_REQUEST_TYPE_UNDEFINED);			\
    mpig_request_i_set_buffer((req_), NULL, 0, MPI_DATATYPE_NULL);			\
    mpig_request_i_set_envelope((req_), MPI_PROC_NULL, MPI_ANY_TAG, -1);		\
    /* (req_)->dev.dtp = NULL; -- cleared by mpig_request_release_dt() */		\
    mpig_request_i_set_remote_req_id((req_), MPI_REQUEST_NULL);				\
    mpig_request_i_set_vc((req_), NULL);						\
    (req_)->dev.next = NULL;								\
}

#define mpig_request_set_params(req_, kind_, type_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, vc_)	\
{														\
    /* set MPICH fields */											\
    mpig_request_set_ref_count((req_), (ref_cnt_));								\
    (req_)->kind = (kind_);											\
    (req_)->cc_ptr = &(req_)->cc;										\
    mpig_request_set_cc((req_), (cc_));										\
														\
    /* set device fields */											\
    mpig_request_set_type((req_), (type_));									\
    mpig_request_set_buffer((req_), (buf_), (cnt_), (dt_));							\
    mpig_request_set_envelope((req_), (rank_), (tag_), (ctx_));							\
    mpig_request_set_vc(req_, (vc_));										\
}

#define mpig_request_create_sreq(type_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, vc_, sreq_p_)		\
{															\
    mpig_request_alloc(sreq_p_);											\
    MPIU_ERR_CHKANDJUMP1((*(sreq_p_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "send request");	\
    mpig_request_construct(*(sreq_p_));											\
    mpig_request_set_params(*(sreq_p_), MPID_REQUEST_SEND, (type_), (ref_cnt_), (cc_),					\
			    (buf_), (cnt_), (dt_), (rank_), (tag_), (ctx_), (vc_));					\
}

#define mpig_request_create_isreq(type_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, comm_, vc_, sreq_p_)	\
{															\
    mpig_request_alloc(sreq_p_);											\
    MPIU_ERR_CHKANDJUMP1((*(sreq_p_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "send request");	\
    mpig_request_construct(*(sreq_p_));											\
    mpig_request_set_params(*(sreq_p_), MPID_REQUEST_SEND, (type_), (ref_cnt_), (cc_),					\
			    (buf_), (cnt_), (dt_), (rank_), (tag_), (ctx_), (vc_));					\
    mpig_request_add_comm_ref(*(sreq_p_), (comm_));									\
    mpig_request_add_dt_ref(*(sreq_p_), (dt_));										\
}

#define mpig_request_create_psreq(type_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, comm_, sreq_p_)			\
{																\
    mpig_request_alloc(sreq_p_);												\
    MPIU_ERR_CHKANDJUMP1((*(sreq_p_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "persistent send request");	\
    mpig_request_construct(*(sreq_p_));												\
    mpig_request_set_params(*(sreq_p_), MPID_PREQUEST_SEND, (type_), (ref_cnt_), (cc_),						\
			    (buf_), (cnt_), (dt_),(rank_), (tag_), (ctx_), NULL);						\
    mpig_request_add_comm_ref(*(sreq_p_), (comm_));										\
    mpig_request_add_dt_ref(*(sreq_p_), (dt_));											\
}

#define mpig_request_construct_rreq(rreq_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, vc_)	\
{													\
    mpig_request_set_params((rreq_), MPID_REQUEST_RECV, MPIG_REQUEST_TYPE_RECV, (ref_cnt_), (cc_),	\
			    (buf_), (cnt_), (dt_), (rank_), (tag_), (ctx_), (vc_));			\
}

#define mpig_request_create_rreq(ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, rreq_p_, vc_)			\
{															\
    mpig_request_alloc(rreq_p_);											\
    MPIU_ERR_CHKANDJUMP1((*(rreq_p_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive request");	\
    mpig_request_construct(*(rreq_p_));											\
    mpig_request_construct_rreq(*(rreq_p_), (ref_cnt_), (cc_), (buf_), (cnt_), (dt_), (rank_), (tag_), (ctx_), (vc_));	\
}

#define mpig_request_construct_irreq(rreq_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, comm_, vc_)	\
{														\
    mpig_request_set_params((rreq_), MPID_REQUEST_RECV, MPIG_REQUEST_TYPE_RECV, (ref_cnt_), (cc_),		\
			    (buf_), (cnt_), (dt_), (rank_), (tag_), (ctx_), (vc_));				\
    mpig_request_add_comm_ref((rreq_), (comm_));								\
    mpig_request_add_dt_ref((rreq_), (dt_));									\
}

#define mpig_request_create_irreq(ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, comm_, vc_, rreq_p_)		\
{															\
    mpig_request_alloc(rreq_p_);											\
    MPIU_ERR_CHKANDJUMP1((*(rreq_p_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive request");	\
    mpig_request_construct(*(rreq_p_));											\
    mpig_request_construct_rreq(*(rreq_p_), (ref_cnt_), (cc_), (buf_), (cnt_), (dt_), (rank_), (tag_), (ctx_), (vc_));	\
}

#define mpig_request_create_prreq(ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, comm_, rreq_p_)				 \
{																 \
    mpig_request_alloc(rreq_p_);												 \
    MPIU_ERR_CHKANDJUMP1((*(rreq_p_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "persistent receive request"); \
    mpig_request_construct(*(rreq_p_));												 \
    mpig_request_set_params(*(rreq_p_), MPID_PREQUEST_RECV, MPIG_REQUEST_TYPE_RECV, (ref_cnt_), (cc_),				 \
			    (buf_), (cnt_), (dt), (rank_), (tag_), (ctx_), NULL);						 \
    mpig_request_add_comm_ref(*(rreq_p_), (comm_));										 \
    mpig_request_add_dt_ref(*(rreq_p_), (dt_));											 \
}
/**********************************************************************************************************************************
						       END REQUEST SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN PROCESS DATA SECTION
**********************************************************************************************************************************/
#define mpig_process_mutex_create()	globus_mutex_init(&mpig_process.mutex, NULL)
#define mpig_process_mutex_destroy()	globus_mutex_destroy(&mpig_process.mutex)
#define mpig_process_mutex_lock()					\
{									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS,			\
		       "process local data - acquiring mutex"));	\
    globus_mutex_lock(&mpig_process.mutex);				\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS,			\
		       "process local data - mutex acquired"));		\
}
#define mpig_process_mutex_unlock()					\
{									\
    globus_mutex_unlock(&mpig_process.mutex);				\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS,			\
		       "process local data - mutex released"));		\
}

#define mpig_process_rc_acq(needed_)	mpig_process_mutex_lock()
#define mpig_process_rc_rel(needed_)	mpig_process_mutex_unlock()
/**********************************************************************************************************************************
						    END PROCESS DATA SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						      BEGIN DATA CONVERSION
**********************************************************************************************************************************/
/*
 * MT-RC-NOTE: the data conversion do not make any effort to insure that the data being converted has been acquired or that the
 * converted data is released.  it is the responsibility of the calling routine to perform any mutex lock/unlock or RC
 * acquire/release operations that are necessary to insure that data manipulated by these routines are visible by the necessary
 * threads.
 */

#define mpig_dc_put_int32(buf_, data_) mpig_dc_put_uint32((buf_), (data_))
#define mpig_dc_putn_int32(buf_, data_, num_) mpig_dc_put_uint32((buf_), (data_), (num_))
#define mpig_dc_get_int32(endian_, buf_, data_p_) mpig_dc_get_uint32((endian_), (buf_), (data_p_))
#define mpig_dc_getn_int32(endian_, buf_, data_, num_) mpig_dc_get_uint32((endian_), (buf_, (data_), (num_))
#define mpig_dc_put_int64(buf_, data_) mpig_dc_put_uint64((buf_), (data_))
#define mpig_dc_putn_int64(buf_, data_, num_) mpig_dc_put_uint64((buf_), (data_), (num_))
#define mpig_dc_get_int64(endian_, buf_, data_p_) mpig_dc_get_uint64((endian_), (buf_), (data_p_))
#define mpig_dc_getn_int64(endian_, buf_, data_, num_) mpig_dc_get_uint64((endian_), (buf_), (data_), (num_))

#define mpig_dc_put_uint32(buf_, data_)				\
{								\
    unsigned char * buf__ = (unsigned char *)(buf_);		\
    unsigned char * data__ = (unsigned char *) &(data_);	\
    *(buf__)++ = *(data__)++;					\
    *(buf__)++ = *(data__)++;					\
    *(buf__)++ = *(data__)++;					\
    *(buf__)++ = *(data__)++;					\
}

#define mpig_dc_putn_uint32(buf_, data_, num_)			\
{								\
    memcpy((void *)(buf_), (void *)(data_), (num_) * 4);	\
}

#define mpig_dc_get_uint32(endian_, buf_, data_p_)			\
{									\
    if ((endian_) == MPIG_MY_ENDIAN)					\
    {									\
	unsigned char * buf__ = (unsigned char *)(buf_);		\
	unsigned char * data_p__ = (unsigned char *)(data_p_);		\
	*(data_p__)++ = *(buf__)++;					\
	*(data_p__)++ = *(buf__)++;					\
	*(data_p__)++ = *(buf__)++;					\
	*(data_p__)++ = *(buf__)++;					\
    }									\
    else								\
    {									\
	unsigned char * buf__ = (unsigned char *)(buf_);		\
	unsigned char * data_p__ = (unsigned char *)(data_p_) + 3;	\
	*(data_p__)-- = *(buf__)++;					\
	*(data_p__)-- = *(buf__)++;					\
	*(data_p__)-- = *(buf__)++;					\
	*(data_p__)-- = *(buf__)++;					\
    }									\
}

#define mpig_dc_getn_uint32(endian_, buf_, data_, num_)		\
{								\
    if ((endian_) == MPIG_MY_ENDIAN)				\
    {								\
	memcpy((void *)(buf_), (void *)(data_), (num_) * 4);	\
    }								\
    else							\
    {								\
	unsigned char * buf__ = (unsigned char *)(buf_);	\
	unsigned char * data__ = (unsigned char *)(data_) + 3;	\
								\
	for (n__ = 0; n__ < (num_); n__++)			\
	{							\
	    *(data__)-- = *(buf__)++;				\
	    *(data__)-- = *(buf__)++;				\
	    *(data__)-- = *(buf__)++;				\
	    *(data__)-- = *(buf__)++;				\
	    data__ += 8;					\
	}							\
    }								\
}

#define mpig_dc_put_uint64(buf_, data_)				\
{								\
    unsigned char * buf__ = (unsigned char *)(buf_);		\
    unsigned char * data__ = (unsigned char *) &(data_);	\
    *(buf__)++ = *(data__)++;					\
    *(buf__)++ = *(data__)++;					\
    *(buf__)++ = *(data__)++;					\
    *(buf__)++ = *(data__)++;					\
    *(buf__)++ = *(data__)++;					\
    *(buf__)++ = *(data__)++;					\
    *(buf__)++ = *(data__)++;					\
    *(buf__)++ = *(data__)++;					\
}

#define mpig_dc_putn_uint64(buf_, data_, num_)			\
{								\
    memcpy((void *)(buf_), (void *)(data_), (num_) * 8);	\
}

#define mpig_dc_get_uint64(endian_, buf_, data_p_)			\
{									\
    if ((endian_) == MPIG_MY_ENDIAN)					\
    {									\
	unsigned char * buf__ = (unsigned char *)(buf_);		\
	unsigned char * data_p__ = (unsigned char *)(data_p_);		\
	*(data_p__)++ = *(buf__)++;					\
	*(data_p__)++ = *(buf__)++;					\
	*(data_p__)++ = *(buf__)++;					\
	*(data_p__)++ = *(buf__)++;					\
	*(data_p__)++ = *(buf__)++;					\
	*(data_p__)++ = *(buf__)++;					\
	*(data_p__)++ = *(buf__)++;					\
	*(data_p__)++ = *(buf__)++;					\
    }									\
    else								\
    {									\
	unsigned char * buf__ = (unsigned char *)(buf_);		\
	unsigned char * data_p__ = (unsigned char *)(data_p_) + 7;	\
	*(data_p__)-- = *(buf__)++;					\
	*(data_p__)-- = *(buf__)++;					\
	*(data_p__)-- = *(buf__)++;					\
	*(data_p__)-- = *(buf__)++;					\
	*(data_p__)-- = *(buf__)++;					\
	*(data_p__)-- = *(buf__)++;					\
	*(data_p__)-- = *(buf__)++;					\
	*(data_p__)-- = *(buf__)++;					\
    }									\
}

#define mpig_dc_getn_uint64(endian_, buf_, data_, num_)		\
{								\
    if ((endian_) == MPIG_MY_ENDIAN)				\
    {								\
	memcpy((void *)(buf_), (void *)(data_), (num_) * 8);	\
    }								\
    else							\
    {								\
	unsigned char * buf__ = (unsigned char *)(buf_);	\
	unsigned char * data__ = (unsigned char *)(data_p) + 7;	\
								\
	for (n__ = 0; n__ < (num_); n__++)			\
	{							\
	    *(data__)-- = *(buf__)++;				\
	    *(data__)-- = *(buf__)++;				\
	    *(data__)-- = *(buf__)++;				\
	    *(data__)-- = *(buf__)++;				\
	    *(data__)-- = *(buf__)++;				\
	    *(data__)-- = *(buf__)++;				\
	    *(data__)-- = *(buf__)++;				\
	    *(data__)-- = *(buf__)++;				\
	    data__ += 16;					\
	}							\
    }								\
}

/**********************************************************************************************************************************
						       END DATA CONVERSION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						    BEGIN I/O VECTOR SECTION
**********************************************************************************************************************************/
/*
 * MT-NOTE: the i/o vector routines are not thread safe.  it is the responsibility of the calling routine to insure that a i/o
 * vector object is accessed atomically.  this is typically handled by performing a mutex lock/unlock on the data structure
 * containing i/o vector object.
 *
 * MT-RC-NOTE: on release consistent systems, if the i/o vector object is to be used by any thread other than the one in which it
 * was created, then the construction of the object must be followed by a RC release or mutex unlock.  furthermore, if a mutex is
 * not used as the means of insuring atomic access to the i/o vector, it will also be necessary to perform RC acquires and
 * releases to guarantee that up-to-date data and internal state is visible at the appropriate times.  (see additional notes in
 * sections for other objects concerning issues with super lazy release consistent systems like Treadmarks.)
 */
MPIU_Size_t mpig_iov_unpack_fn(const void * buf, MPIU_Size_t buf_size, mpig_iov_t * iov);

#define mpig_iov_construct(iov_, max_entries_)			\
{								\
    ((mpig_iov_t *)(iov_))->max_entries = (max_entries_);	\
    mpig_iov_reset(iov_, 0);					\
}

#define mpig_iov_destruct(iov_)	\
{				\
    mpig_iov_nullify(iov_);	\
}

#define mpig_iov_reset(iov_, num_prealloc_entries_)			\
{									\
    ((mpig_iov_t *)(iov_))->num_bytes = 0;				\
    ((mpig_iov_t *)(iov_))->free_entry = (num_prealloc_entries_);	\
    ((mpig_iov_t *)(iov_))->cur_entry = 0;				\
}

#define mpig_iov_nullify(iov_)			\
{						\
    ((mpig_iov_t *)(iov_))->max_entries = 0;	\
}

#define mpig_iov_is_null(iov_) ((((mpig_iov_t *)(iov_))->max_entries == 0) ? TRUE : FALSE)

#define mpig_iov_set_entry(iov_, entry_, buf_, bytes_)					\
{											\
    MPIU_Assert((entry_) < ((mpig_iov_t *)(iov_))->max_entries);			\
    ((mpig_iov_t *)(iov_))->iov[entry_].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)(buf_);	\
    ((mpig_iov_t *)(iov_))->iov[entry_].MPID_IOV_LEN = (bytes_);			\
    ((mpig_iov_t *)(iov_))->num_bytes += (bytes_);					\
}

#define mpig_iov_add_entry(iov_, buf_, bytes_)									\
{														\
    MPIU_Assert(((mpig_iov_t *)(iov_))->free_entry < ((mpig_iov_t *)(iov_))->max_entries);			\
    ((mpig_iov_t *)(iov_))->iov[((mpig_iov_t *)(iov_))->free_entry].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)(buf_);	\
    ((mpig_iov_t *)(iov_))->iov[((mpig_iov_t *)(iov_))->free_entry].MPID_IOV_LEN = (bytes_);			\
    ((mpig_iov_t *)(iov_))->num_bytes += (bytes_);								\
    ((mpig_iov_t *)(iov_))->free_entry += 1;									\
}

#define mpig_iov_inc_num_inuse_entries(iov_, n_)						\
{												\
    ((mpig_iov_t *)(iov_))->free_entry += (n_);							\
    MPIU_Assert(((mpig_iov_t *)(iov_))->free_entry <= ((mpig_iov_t *)(iov_))->max_entries);	\
}

#define mpig_iov_inc_current_entry(iov_, n_)	\
{						\
    ((mpig_iov_t *)(iov_))->cur_entry += (n_);	\
}

#define mpig_iov_inc_num_bytes(iov_, bytes_)		\
{							\
    ((mpig_iov_t *)(iov_))->num_bytes += (bytes_);	\
}

#define mpig_iov_dec_num_bytes(iov_, bytes_)		\
{							\
    ((mpig_iov_t *)(iov_))->num_bytes -= (bytes_);	\
}

#define mpig_iov_get_base_entry_ptr(iov_) (&((mpig_iov_t *)(iov_))->iov[0])

#define mpig_iov_get_current_entry_ptr(iov_) (&((mpig_iov_t *)(iov_))->iov[((mpig_iov_t *)(iov_))->cur_entry])

#define mpig_iov_get_next_free_entry_ptr(iov_) (&((mpig_iov_t *)(iov_))->iov[((mpig_iov_t *)(iov_))->free_entry])

#define mpig_iov_get_num_entries(iov_) (((mpig_iov_t *)(iov_))->max_entries)

#define mpig_iov_get_num_inuse_entries(iov_) (((mpig_iov_t *)(iov_))->free_entry - ((mpig_iov_t *)(iov_))-> cur_entry)

#define mpig_iov_get_num_free_entries(iov_) (((mpig_iov_t *)(iov_))->max_entries - ((mpig_iov_t *)(iov_))->free_entry)

#define mpig_iov_get_num_bytes(iov_) (((mpig_iov_t *)(iov_))->num_bytes)

#define mpig_iov_unpack(buf_, nbytes_, iov_) mpig_iov_unpack_fn((buf_), (nbytes_), (mpig_iov_t *)(iov_));
/**********************************************************************************************************************************
						     END I/O VECTOR SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						    BEGIN DATA BUFFER SECTION
**********************************************************************************************************************************/
/*
 * MT-NOTE: the data buffer routines are not thread safe.  it is the responsibility of the calling routine to insure that a data
 * buffer object is accessed atomically.  this is typically handled by performing a mutex lock/unlock on the data structure
 * containing data buffer object.
 *
 * MT-RC-NOTE: on release consistent systems, if the data buffer object is to be used by any thread other than the one in which
 * it was created, then the construction of the object must be followed by a RC release or mutex unlock.  furthermore, if a mutex
 * is not used as the means of insuring atomic access to the data bufffer, it will also be necessary to perform RC acquires and
 * releases to guarantee that up-to-date data and internal state is visible at the appropriate times.  (see additional notes in
 * sections for other objects concerning issues with super lazy release consistent systems like Treadmarks.)
 */

void mpig_databuf_create(MPIU_Size_t size, mpig_databuf_t ** dbufp, int * mpi_errno_p, bool_t * failed_p);

void mpig_databuf_destroy(mpig_databuf_t * dbuf);

#define mpig_databuf_construct(dbuf_, size_)		\
{							\
    ((mpig_databuf_t *)(dbuf_))->size = (size_);	\
    ((mpig_databuf_t *)(dbuf_))->eod = 0;		\
    ((mpig_databuf_t *)(dbuf_))->pos = 0;		\
}

#define mpig_databuf_destruct(dbuf_)		\
{						\
    ((mpig_databuf_t *)(dbuf_))->size = 0;	\
    ((mpig_databuf_t *)(dbuf_))->eod = 0;	\
    ((mpig_databuf_t *)(dbuf_))->pos = 0;	\
}
								
#define mpig_databuf_reset(dbuf_)		\
{						\
    ((mpig_databuf_t *)(dbuf_))->eod = 0;	\
    ((mpig_databuf_t *)(dbuf_))->pos = 0;	\
}

#define mpig_databuf_get_ptr(dbuf_) ((char *)(dbuf_) + sizeof(mpig_databuf_t))

#define mpig_databuf_get_pos_ptr(dbuf_) ((char *)(dbuf_) + sizeof(mpig_databuf_t) + ((mpig_databuf_t *)(dbuf_))->pos)

#define mpig_databuf_get_eod_ptr(dbuf_) ((char *)(dbuf_) + sizeof(mpig_databuf_t) + ((mpig_databuf_t *)(dbuf_))->eod)

#define mpig_databuf_get_size(dbuf_) (((mpig_databuf_t *)(dbuf_))->size)

#define mpig_databuf_get_eod(dbuf_) (((mpig_databuf_t *)(dbuf_))->eod)

#define mpig_databuf_set_eod(dbuf_, val_)											\
{																\
    MPIU_Assert(((mpig_databuf_t *)(dbuf_))->size - (val_) <= ((mpig_databuf_t *)(dbuf_))->size);				\
    ((mpig_databuf_t *)(dbuf_))->eod = (MPIU_Size_t)(val_);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATABUF,										\
	"databuf - set eod: databuf=" MPIG_PTR_FMT ", val=" MPIG_SIZE_FMT ", size=" MPIG_SIZE_FMT ", pos=" MPIG_SIZE_FMT	\
	", eod=" MPIG_SIZE_FMT ", eodp=" MPIG_PTR_FMT ", posp=" MPIG_PTR_FMT, (MPIG_PTR_CAST) (dbuf_), (MPIU_Size_t)(val_),	\
	((mpig_databuf_t *)(dbuf_))->size, ((mpig_databuf_t *)(dbuf_))->pos, ((mpig_databuf_t *)(dbuf_))->eod,			\
	(MPIG_PTR_CAST) mpig_databuf_get_eod_ptr(dbuf_), (MPIG_PTR_CAST) mpig_databuf_get_pos_ptr(dbuf_)));			\
}

#define mpig_databuf_inc_eod(dbuf_, val_)											\
{																\
    MPIU_Assert(((mpig_databuf_t *)(dbuf_))->eod + (val_) <= ((mpig_databuf_t *)(dbuf_))->size);				\
    ((mpig_databuf_t *)(dbuf_))->eod += (MPIU_Size_t)(val_);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATABUF,										\
	"databuf - inc eod: databuf=" MPIG_PTR_FMT ", val=" MPIG_SIZE_FMT ", size=" MPIG_SIZE_FMT ", pos=" MPIG_SIZE_FMT	\
	", eod=" MPIG_SIZE_FMT ", eodp=" MPIG_PTR_FMT ", posp=" MPIG_PTR_FMT, (MPIG_PTR_CAST) (dbuf_), (MPIU_Size_t)(val_),	\
	((mpig_databuf_t *)(dbuf_))->size, ((mpig_databuf_t *)(dbuf_))->pos, ((mpig_databuf_t *)(dbuf_))->eod,			\
	(MPIG_PTR_CAST) mpig_databuf_get_eod_ptr(dbuf_), (MPIG_PTR_CAST) mpig_databuf_get_pos_ptr(dbuf_)));			\
}

#define mpig_databuf_dec_eod(dbuf_, val_)											\
{																\
    MPIU_Assert(((mpig_databuf_t *)(dbuf_))->eod - (val_) <= ((mpig_databuf_t *)(dbuf_))->eod);					\
    ((mpig_databuf_t *)(dbuf_))->eod -= (MPIU_Size_t)(val_);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATABUF,										\
	"databuf - dec eod: databuf=" MPIG_PTR_FMT ", val=" MPIG_SIZE_FMT ", size=" MPIG_SIZE_FMT ", pos=" MPIG_SIZE_FMT	\
	", eod=" MPIG_SIZE_FMT ", eodp=" MPIG_PTR_FMT ", posp=" MPIG_PTR_FMT, (MPIG_PTR_CAST) (dbuf_), (MPIU_Size_t)(val_),	\
	((mpig_databuf_t *)(dbuf_))->size, ((mpig_databuf_t *)(dbuf_))->pos, ((mpig_databuf_t *)(dbuf_))->eod,			\
	(MPIG_PTR_CAST) mpig_databuf_get_eod_ptr(dbuf_), (MPIG_PTR_CAST) mpig_databuf_get_pos_ptr(dbuf_)));			\
}

#define mpig_databuf_get_pos(dbuf_) (((mpig_databuf_t *)(dbuf_))->pos)

#define mpig_databuf_set_pos(dbuf_, val_)											\
{																\
    MPIU_Assert(((mpig_databuf_t *)(dbuf_))->eod - (val_) <= ((mpig_databuf_t *)(dbuf_))->eod);					\
    ((mpig_databuf_t *)(dbuf_))->pos = (MPIU_Size_t)(val_);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATABUF,										\
	"databuf - set pos: databuf=" MPIG_PTR_FMT ", val=" MPIG_SIZE_FMT ", size=" MPIG_SIZE_FMT ", pos=" MPIG_SIZE_FMT	\
	", eod=" MPIG_SIZE_FMT ", eodp=" MPIG_PTR_FMT ", posp=" MPIG_PTR_FMT, (MPIG_PTR_CAST) (dbuf_), (MPIU_Size_t)(val_),	\
	((mpig_databuf_t *)(dbuf_))->size, ((mpig_databuf_t *)(dbuf_))->pos, ((mpig_databuf_t *)(dbuf_))->eod,			\
	(MPIG_PTR_CAST) mpig_databuf_get_eod_ptr(dbuf_), (MPIG_PTR_CAST) mpig_databuf_get_pos_ptr(dbuf_)));			\
}

#define mpig_databuf_inc_pos(dbuf_, val_)											\
{																\
    MPIU_Assert(((mpig_databuf_t *)(dbuf_))->pos + (val_) <= ((mpig_databuf_t *)(dbuf_))->eod);					\
    ((mpig_databuf_t *)(dbuf_))->pos += (MPIU_Size_t)(val_);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATABUF,										\
	"databuf - inc pos: databuf=" MPIG_PTR_FMT ", val=" MPIG_SIZE_FMT ", size=" MPIG_SIZE_FMT ", pos=" MPIG_SIZE_FMT	\
	", eod=" MPIG_SIZE_FMT ", eodp=" MPIG_PTR_FMT ", posp=" MPIG_PTR_FMT, (MPIG_PTR_CAST) (dbuf_), (MPIU_Size_t)(val_),	\
	((mpig_databuf_t *)(dbuf_))->size, ((mpig_databuf_t *)(dbuf_))->pos, ((mpig_databuf_t *)(dbuf_))->eod,			\
	(MPIG_PTR_CAST) mpig_databuf_get_eod_ptr(dbuf_), (MPIG_PTR_CAST) mpig_databuf_get_pos_ptr(dbuf_)));			\
}

#define mpig_databuf_dec_pos(dbuf_, val_)											\
{																\
    MPIU_Assert(((mpig_databuf_t *)(dbuf_))->pos - (val_) <= ((mpig_databuf_t *)(dbuf_))->pos);					\
    ((mpig_databuf_t *)(dbuf_))->pos -= (MPIU_Size_t)(val_);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATABUF,										\
	"databuf - dec pos: databuf=" MPIG_PTR_FMT ", val=" MPIG_SIZE_FMT ", size=" MPIG_SIZE_FMT ", pos=" MPIG_SIZE_FMT	\
	", eod=" MPIG_SIZE_FMT ", eodp=" MPIG_PTR_FMT ", posp=" MPIG_PTR_FMT, (MPIG_PTR_CAST) (dbuf_), (MPIU_Size_t)(val_),	\
	((mpig_databuf_t *)(dbuf_))->size, ((mpig_databuf_t *)(dbuf_))->pos, ((mpig_databuf_t *)(dbuf_))->eod,			\
	(MPIG_PTR_CAST) mpig_databuf_get_eod_ptr(dbuf_), (MPIG_PTR_CAST) mpig_databuf_get_pos_ptr(dbuf_)));			\
}

#define mpig_databuf_get_remaining_bytes(dbuf_) (mpig_databuf_get_eod(dbuf_) - mpig_databuf_get_pos(dbuf_))

#define mpig_databuf_get_free_bytes(dbuf_) (mpig_databuf_get_size(dbuf_) - mpig_databuf_get_eod(dbuf_))
/**********************************************************************************************************************************
						     END DATA BUFFER SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN BUSINESS CARD SECTION
**********************************************************************************************************************************/
/*
 * NOTE: to insure that the memory associated with the strings returned by mpig_bc_get_contact() and mpig_bc_serialize_object()
 * is freed, the caller is responsible mpig_bc_free_contact() and mpig_bc_free_serialized_object() respectively.
 *
 * MT-NOTE: the business card routines are not thread safe.  it is the responsibility of the calling routine to insure that a
 * business card object is accessed atomically.  this is typically handled by performing a mutex lock/unlock on the data
 * structure containing business card object.
 *
 * MT-RC-NOTE: on release consistent systems, if the business card object is to be used by any thread other than the one in which
 * it was created, then the creation of the object must be followed by a RC release or mutex unlock.  furthermore, if a mutex is
 * not used as the means of insuring atomic access to the business card, it will also be necessary to perform RC acquires and
 * releases to guarantee that up-to-date data and internal state is visible at the appropriate times.  (see additional notes in
 * sections for other objects concerning issues with super lazy release consistent systems like Treadmarks.)
 */

void mpig_bc_create(mpig_bc_t * bc, int * mpi_errno_p, bool_t * failed_p);

void mpig_bc_add_contact(mpig_bc_t * bc, const char * key, const char * value, int * mpi_errno_p, bool_t * failed_p);

void mpig_bc_get_contact(const mpig_bc_t * bc, const char * key, char ** value, bool_t * found_p,
			 int * mpi_errno_p, bool_t * failed_p);

void mpig_bc_free_contact(char * value);

void mpig_bc_serialize_object(mpig_bc_t * bc, char ** str, int * mpi_errno_p, bool_t * failed_p);

void mpig_bc_free_serialized_object(char * str);

void mpig_bc_deserialize_object(const char *, mpig_bc_t * bc, int * mpi_errno_p, bool_t * failed_p);

void mpig_bc_destroy(mpig_bc_t * bc, int * mpi_errno_p, bool_t * failed_p);
/**********************************************************************************************************************************
						    END BUSINESS CARD SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						BEGIN VIRTUAL CONNECTION SECTION
**********************************************************************************************************************************/
/* MT-NOTE: the following routine locks the VC and PG mutexes.  previous routines on the call stack must not be holding those
   mutexes. */
void mpig_vc_release_ref(mpig_vc_t * vc, int * mpi_errno_p, bool_t * failed_p);

void mpig_vc_null_func(void);

/*
 * constructor/destructor macros
 *
 * MT-RC-NOTE: on release consistent systems, if the virtual connection object is to be used by any thread other than the one in
 * which it was created, then the construction of the object must be followed by a RC release or mutex unlock.  for systems using
 * super lazy release consistency models, such as the one used in Treadmarks, it would be necessary to perform an acquire/lock in
 * the mpig_vc_construct() immediately after creating the mutex.  The calling routine would need to perform a release after the
 * temp VC was completely initialized.  we have chosen to assume that none of the the systems upon which MPIG will run have that
 * lazy of a RC model.
 */
#define mpig_vc_construct(vc_)				\
{							\
    mpig_vc_mutex_create(vc_);				\
    mpig_vc_i_set_ref_count((vc_), 0);			\
    mpig_vc_set_cm_type((vc_), MPIG_CM_TYPE_UNDEFINED);	\
    mpig_vc_set_cm_funcs((vc_), NULL);			\
    mpig_vc_set_pg_info((vc_), NULL, 0);		\
    mpig_vc_set_pg_id((vc_), "(pg id not set)");	\
    (vc_)->lpid = -1;					\
}

#define mpig_vc_destruct(vc_)							\
{										\
    if ((vc_)->cm_funcs != NULL && (vc_)->cm_funcs->vc_destruct != NULL)	\
    {										\
	(vc_)->cm_funcs->vc_destruct(vc_);					\
    }										\
    mpig_vc_i_set_ref_count((vc_), 0);						\
    mpig_vc_set_cm_type((vc_), MPIG_CM_TYPE_UNDEFINED);				\
    mpig_vc_set_cm_funcs((vc_), NULL);						\
    mpig_vc_set_pg_info((vc_), NULL, 0);					\
    mpig_vc_set_pg_id((vc_), "");						\
    (vc_)->lpid = -1;								\
    mpig_vc_mutex_destroy(vc_);							\
}

/*
 * reference counting macros
 *
 * MT-NOTE: these macros are not thread safe.  their use will likely require locking the PG mutex, although performing a RC
 * acquire/release may prove sufficient, depending on the level of synchronization required.
 */
#define mpig_vc_i_set_ref_count(vc_, ref_count_)						\
{												\
    (vc_)->ref_count = (ref_count_);								\
}

#define mpig_vc_set_ref_count(vc_, ref_count_)									\
{														\
    mpig_vc_i_set_ref_count((vc_), (ref_count_));								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_VC,						\
	"VC - set ref count: vc=" MPIG_PTR_FMT ", ref_count=%d", (MPIG_PTR_CAST) (vc_), (vc_)->ref_count));	\
}

#define mpig_vc_inc_ref_count(vc_, was_inuse_p_, mpi_errno_p_, failed_p_)						\
{															\
    if ((vc_)->cm_funcs->vc_inc_ref_count == NULL)									\
    {															\
	*(was_inuse_p_) = ((vc_)->ref_count++) ? TRUE : FALSE;								\
	*(failed_p_) = FALSE;												\
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_VC,						\
	    "VC - increment ref count: vc=" MPIG_PTR_FMT ", ref_count=%d", (MPIG_PTR_CAST) (vc_), (vc_)->ref_count));	\
    }															\
    else														\
    {															\
	(vc_)->cm_funcs->vc_inc_ref_count((vc_), (was_inuse_p_), (mpi_errno_p_), (failed_p_));				\
    }															\
}

#define mpig_vc_dec_ref_count(vc_, is_inuse_p_, mpi_errno_p_, failed_p_)						\
{															\
    if ((vc_)->cm_funcs->vc_dec_ref_count == NULL)									\
    {															\
	*(is_inuse_p_) = (--(vc_)->ref_count) ? TRUE : FALSE;								\
	*(failed_p_) = FALSE;												\
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_VC,						\
	    "VC - decrement ref count: vc=" MPIG_PTR_FMT ", ref_count=%d", (MPIG_PTR_CAST) (vc_), (vc_)->ref_count));	\
    }															\
    else														\
    {															\
	(vc_)->cm_funcs->vc_dec_ref_count((vc_), (is_inuse_p_), (mpi_errno_p_), (failed_p_));				\
    }															\
}

/*
 * miscellaneous accessor macros
 *
 * MT-NOTE: these macros are not thread safe.  their use may require locking the PG mutex.  alternatively, it may be sufficient
 * for the get routines to be preceeded by a RC acquire and the set routines to be followed a RC release.  the specific method
 * depends on the level of synchronization required.
 */
#define mpig_vc_set_cm_type(vc_, cm_type_)	\
{						\
    (vc_)->cm_type = (cm_type_);		\
}

#define mpig_vc_get_cm_type(vc_) ((vc_)->cm_type)

#define mpig_vc_set_cm_funcs(vc_, cm_funcs_)	\
{						\
    (vc_)->cm_funcs = (cm_funcs_);		\
}

#define mpig_vc_set_pg_info(vc_, pg_, pg_rank_)	\
{						\
    (vc_)->pg = (pg_);				\
    (vc_)->pg_rank = (pg_rank_);		\
}

#define mpig_vc_set_pg_id(vc_, pg_id_)	\
{					\
    (vc_)->pg_id = (pg_id_);		\
}

#define mpig_vc_get_pg(vc_) ((vc_)->pg)

#define mpig_vc_get_pg_rank(vc_) ((vc_)->pg_rank)

#define mpig_vc_get_pg_id(vc_) ((vc_)->pg_id)

/* Thread safety and release consistency macros */
#define mpig_vc_mutex_create(vc_)	globus_mutex_init(&(vc_)->mutex, NULL)
#define mpig_vc_mutex_destroy(vc_)	globus_mutex_destroy(&(vc_)->mutex)
#define mpig_vc_mutex_lock(vc_)									\
{												\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_VC,				\
		       "VC - acquiring mutex: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) (vc_)));	\
    globus_mutex_lock(&(vc_)->mutex);								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_VC,				\
		       "VC - mutex acquired: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) (vc_)));	\
}
#define mpig_vc_mutex_unlock(vc_)								\
{												\
    globus_mutex_unlock(&(vc_)->mutex);								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_VC,				\
		       "VC - mutex released: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) (vc_)));	\
}

#define mpig_vc_mutex_lock_conditional(vc_, cond_)	{if (cond_) mpig_vc_mutex_lock(vc_);}
#define mpig_vc_mutex_unlock_conditional(vc_, cond_)	{if (cond_) mpig_vc_mutex_unlock(vc_);}

#define mpig_vc_rc_acq(vc_, needed_)	mpig_vc_mutex_lock(vc_)
#define mpig_vc_rc_rel(vc_, needed_)	mpig_vc_mutex_unlock(vc_)
/**********************************************************************************************************************************
						 END VIRTUAL CONNECTION SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					BEGIN VIRTUAL CONNECTION REFERENCE TABLE SECTION
**********************************************************************************************************************************/
/* MT-NOTE: atomicity and data conherence is presently guaranteed by the MPI layer */
#define mpig_vcrt_mutex_create(vcrt_)
#define mpig_vcrt_mutex_lock(vcrt_)
#define mpig_vcrt_mutex_unlock(vcrt_)
#define mpig_vcrt_mutex_destroy(vcrt_)
#define mpig_vcrt_rc_acq(vcrt_, needed_)
#define mpig_vcrt_rc_rel(vcrt_, needed_)


/*
 * mpig_vcrt object definition
 */
typedef struct mpig_vcrt
{
    /* globus_mutex_t mutex; -- not need right now; see note above */

    /* number of references to this object */
    int ref_count;

    /* number of entries in the table */
    int size;

    /* array of virtual connection references (pointers to VCs) */
    mpig_vc_t * vcr_table[1];
}
mpig_vcrt_t;
/**********************************************************************************************************************************
					END VIRTUAL CONNECTION REFERENCE TABLE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN PROCESS GROUP SECTION
**********************************************************************************************************************************/
void mpig_pg_init(int * mpi_errno_p, bool_t * failed_p);

void mpig_pg_finalize(int * mpi_errno_p, bool_t * failed_p);

void mpig_pg_acquire_ref_locked(const char * pg_id, int pg_size, mpig_pg_t ** pg_p, int * mpi_errno_p, bool_t * failed_p);

void mpig_pg_commit(mpig_pg_t * pg);

void mpig_pg_release_ref(mpig_pg_t * pg);

/*
 *reference counting macros
 *
 * MT-NOTE: these macros are not thread safe.  their use will likely require locking the PG mutex, although performing a RC
 * acquire/release may prove sufficient, depending on the level of synchronization required.
 */
#define mpig_pg_inc_ref_count(pg_)											\
{															\
    (pg_)->ref_count++;													\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_PG,							\
	"PG - increment ref count: vc=" MPIG_PTR_FMT ", ref_count=%d", (MPIG_PTR_CAST) (pg_), (pg_)->ref_count));	\
}


#define mpig_pg_dec_ref_count(pg_, is_inuse_p_)										\
{															\
    *(is_inuse_p_) = (--(pg_)->ref_count) ? TRUE : FALSE;								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_PG,							\
	"PG - decrement ref count: vc=" MPIG_PTR_FMT ", ref_count=%d", (MPIG_PTR_CAST) (pg_), (pg_)->ref_count));	\
}

/*
 * miscellaneous accessor macros
 *
 * MT-NOTE: these macros are not thread safe.  their use may require locking the PG mutex.  alternatively, it may be sufficient
 * for the get routines to be preceeded by a RC acquire and the set routines to be followed a RC release.  the specific method
 * depends on the level of synchronization required.
 */
#define mpig_pg_get_size(pg_) ((pg_)->size)

#define mpig_pg_get_rank(pg_) ((pg_)->rank)

/* MT-NOTE: this routine should only be called if the VC is known to have a reference count greater than zero.  if the reference
   count could be zero indicating the object is being or has been destroyed, then use mpig_pg_get_vc_ref() */
#define mpig_pg_get_vc(pg_, rank_, vc_p_)	\
{						\
    *(vc_p_) = &(pg_)->vct[rank_];		\
}

/*
 * process group ID accessor macros
 *
 * MT-NOTE: these macros are not thread safe.  their use may require locking the PG mutex or performing a RC release.
 */
#define mpig_pg_get_id(pg_) ((pg_)->id)

#define mpig_pg_compare_ids(id1_, id2_) (strcmp((id1_), (id2_)))

/*
 * thread saftey and release consistency macros
 */
extern globus_mutex_t mpig_pg_global_mutex;

#define mpig_pg_global_mutex_create()	globus_mutex_init(&mpig_pg_global_mutex, NULL)
#define mpig_pg_global_mutex_destroy()	globus_mutex_destroy(&mpig_pg_global_mutex)
#define mpig_pg_global_mutex_lock()									\
{													\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_PG, "PG - acquiring global mutex"));	\
    globus_mutex_lock(&mpig_pg_global_mutex);								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_PG, "PG - global mutex acquired"));	\
}
#define mpig_pg_global_mutex_unlock()									\
{													\
    globus_mutex_unlock(&mpig_pg_global_mutex);								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_PG, "PG - global mutex released"));	\
}
 
#define mpig_pg_mutex_create(pg_)
#define mpig_pg_mutex_destroy(pg_)
#define mpig_pg_mutex_lock(pg_)		mpig_pg_global_mutex_lock()
#define mpig_pg_mutex_unlock(pg_)	mpig_pg_global_mutex_unlock()

#define mpig_pg_mutex_lock_conditional(pg_, cond_)	{if (cond_) mpig_pg_mutex_lock(pg_);}
#define mpig_pg_mutex_unlock_conditional(pg_, cond_)	{if (cond_) mpig_pg_mutex_unlock(pg_);}

#define mpig_pg_rc_acq(pg_, needed_)	mpig_pg_mutex_lock(pg_)
#define mpig_pg_rc_rel(pg_, needed_)	mpig_pg_mutex_unlock(pg_)
/**********************************************************************************************************************************
						    END PROCESS GROUP SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN RECEIVE QUEUE SECTION
**********************************************************************************************************************************/
/*
 * MT-NOTE: with the exception of the init and finalize routines, the receive queue routines are thread safe.
 *
 * the routines that return a request, return it with its mutex locked by the current context.  this allows the calling routine
 * an opportunity to initialize an enqueued request before it is accessible to a thread which has dequeued it.  likewise, it
 * allows another thread still initializing a request to complete before the request is made available to other routines.
 *
 * the routines that take a request as a parameter expect that the calling routine will acquire the request mutex either before
 * calling the recvq routine or directly after calling the recvq routine.  In the latter case, acquiring the mutex is only
 * necessary if the request be found and dequeued.  Acquiring the mutex guarantees that no other thread is in the process of
 * initializing the request.
 */
void mpig_recvq_init(int * mpi_errno_p, bool_t * failed_p);

void mpig_recvq_finalize(int * mpi_errno_p, bool_t * failed_p);

struct MPID_Request * mpig_recvq_find_unexp(int rank, int tag, int ctx);

struct MPID_Request * mpig_recvq_deq_unexp(int rank, int tag, int ctx, MPI_Request sreq_id);

bool_t mpig_recvq_deq_unexp_rreq(struct MPID_Request * rreq);

bool_t mpig_recvq_deq_posted_rreq(struct MPID_Request * rreq);

struct MPID_Request * mpig_recvq_deq_unexp_or_enq_posted(int rank, int tag, int ctx, int * found_p);

struct MPID_Request * mpig_recvq_deq_posted_or_enq_unexp(int rank, int tag, int ctx, int * found_p);
/**********************************************************************************************************************************
						    END RECEIVE QUEUE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						BEGIN PROCESS MANAGEMENT SECTION
**********************************************************************************************************************************/
int mpig_pm_init(void);

int mpig_pm_finalize(void);

int mpig_pm_abort(int exit_code);

int mpig_pm_exchange_business_cards(mpig_bc_t * bc, mpig_bc_t ** bcs_p);

int mpig_pm_free_business_cards(mpig_bc_t * bcs);

int mpig_pm_get_pg_size(int * pg_size);

int mpig_pm_get_pg_rank(int * pg_rank);

int mpig_pm_get_pg_id(const char ** pg_id_p);

int mpig_pm_get_app_num(int * app_num);


int mpig_pm_gk_init(void);

int mpig_pm_gk_finalize(void);

int mpig_pm_gk_abort(int exit_code);

int mpig_pm_gk_exchange_business_cards(mpig_bc_t * bc, mpig_bc_t ** bcs_p);

int mpig_pm_gk_free_business_cards(mpig_bc_t * bcs);

int mpig_pm_gk_get_pg_size(int * pg_size);

int mpig_pm_gk_get_pg_rank(int * pg_rank);

int mpig_pm_gk_get_pg_id(const char ** pg_id_p);

int mpig_pm_gk_get_app_num(int * app_num);


int mpig_pm_ws_init(void);

int mpig_pm_ws_finalize(void);

int mpig_pm_ws_abort(int exit_code);

int mpig_pm_ws_exchange_business_cards(mpig_bc_t * bc, mpig_bc_t ** bcs_p);

int mpig_pm_ws_free_business_cards(mpig_bc_t * bcs);

int mpig_pm_ws_get_pg_size(int * pg_size);

int mpig_pm_ws_get_pg_rank(int * pg_rank);

int mpig_pm_ws_get_pg_id(const char ** pg_id_p);

int mpig_pm_ws_get_app_num(int * app_num);

#if 1
#define mpig_pm_init mpig_pm_gk_init
#define mpig_pm_finalize mpig_pm_gk_finalize
#define mpig_pm_abort mpig_pm_gk_abort
#define mpig_pm_exchange_business_cards mpig_pm_gk_exchange_business_cards
#define mpig_pm_free_business_cards mpig_pm_gk_free_business_cards
#define mpig_pm_get_pg_size mpig_pm_gk_get_pg_size
#define mpig_pm_get_pg_rank mpig_pm_gk_get_pg_rank
#define mpig_pm_get_pg_id mpig_pm_gk_get_pg_id
#define mpig_pm_get_app_num mpig_pm_gk_get_app_num
#else
#define mpig_pm_init mpig_pm_ws_init
#define mpig_pm_finalize mpig_pm_ws_finalize
#define mpig_pm_abort mpig_pm_ws_abort
#define mpig_pm_exchange_business_cards mpig_pm_ws_exchange_business_cards
#define mpig_pm_free_business_cards mpig_pm_ws_free_business_cards
#define mpig_pm_get_pg_size mpig_pm_ws_get_pg_size
#define mpig_pm_get_pg_rank mpig_pm_ws_get_pg_rank
#define mpig_pm_get_pg_id mpig_pm_ws_get_pg_id
#define mpig_pm_get_app_num mpig_pm_ws_get_app_num
#endif
/**********************************************************************************************************************************
						 END PROCESS MANAGEMENT SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						       BEGIN PT2PT SECTION
**********************************************************************************************************************************/
int mpig_adi3_cancel_recv(struct MPID_Request * rreq);
/**********************************************************************************************************************************
							END PT2PT SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						       BEGIN VENDOR MPI SECTION
**********************************************************************************************************************************/
#if defined(MPIG_VMPI)
void mpig_vmpi_error_to_mpich2_error(int vendor_errno, int * mpi_errno_p);
#endif
/**********************************************************************************************************************************
							END VENDOR MPI SECTION
**********************************************************************************************************************************/

#endif /* MPICH2_MPIDIMPL_H_INCLUDED */
