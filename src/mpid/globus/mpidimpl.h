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
#undef free
#endif

/*
 * Function enter/exit macro, used primarily for logging, but can be used for other things.
 */
#define MPIG_STATE_DECL(a_) MPIDI_STATE_DECL(a_)
#define MPIG_FUNC_ENTER(a_) MPIDI_FUNC_ENTER(a_)
#define MPIG_FUNC_EXIT(a_) MPIDI_FUNC_EXIT(a_)
#define MPIG_RMA_FUNC_ENTER(a_) MPIDI_RMA_FUNC_ENTER(a_)
#define MPIG_RMA_FUNC_EXIT(a_) MPIDI_RMA_FUNC_EXIT(a_)


#define MPIG_QUOTE(a_) MPIG_QUOTE2(a_)
#define MPIG_QUOTE2(a_) #a_


#if defined(NDEBUG)
#define MPIG_STATIC static
#else
#define MPIG_STATIC
#endif


#if defined(HAVE_GETHOSTNAME) && defined(NEEDS_GETHOSTNAME_DECL) && !defined(gethostname)
int gethostname(char *name, size_t len);
# endif


/*>>>>>>>>>>>>>>>>>>>>
  PROCESS DATA SECTION
  >>>>>>>>>>>>>>>>>>>>*/
/*
 * The value 128 is returned by the echomaxprocname target in src/mpid/globus/Makefile.sm.  If the value is modified here, it
 * also needs to be modified in Makefile.sm.
 */
#if !defined(MPIG_PROCESSOR_NAME_SIZE)
#   define MPIG_PROCESSOR_NAME_SIZE 128
#endif

typedef struct mpig_process
{
    /* Pointer to the the process group to which this process belongs, the size of the process group, and the rank of the
       processs within the process group */
    struct mpig_pg * my_pg;
    const char * my_pg_id;
    int my_pg_size;
    int my_pg_rank;

    /* The sizeof of the subjob to which this process belongs, and the rank of the process within the subjob */
    int my_sj_size;
    int my_sj_rank;
    
    /* Local process ID counter for assigning a local ID to each virtual connection.  A local ID is necessary for the MPI_Group
       routines, implemented at the MPICH layer, to function properly.  MT: this requires a thread safe fetch-and-increment */
    int lpid_counter;

#if defined(MPIG_VMPI)
#define MPIG_VMPI_COMM_WORLD (&mpig_process.vmpi_cw)
    mpig_vmpi_comm_t vmpi_cw;
    int vmpi_cw_size;
    int vmpi_cw_rank;
#endif

    char hostname[MPIG_PROCESSOR_NAME_SIZE];
    pid_t pid;
}
mpig_process_t;

extern mpig_process_t mpig_process;

/* XXX: MT: needs to be made thread safe */
#define MPIDI_LPID_get_next(lpid_)		\
{						\
    *(lpid_) = mpig_process.lpid_counter++;	\
}
/*<<<<<<<<<<<<<<<<<<<<
  PROCESS DATA SECTION
  <<<<<<<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>>>>>>>>>
  BUSINESS CARD SECTION
  >>>>>>>>>>>>>>>>>>>>>*/
/*
 * NOTE: to insure that the memory associated with the strings returned by mpig_bc_get_contact() and mpig_bc_serialize_object()
 * is freed, the caller is responsible mpig_bc_free_contact() and mpig_bc_free_serialized_object() respectively.
 */
int mpig_bc_create(mpig_bc_t * bc);

int mpig_bc_add_contact(mpig_bc_t * bc, const char * key, char * value);

int mpig_bc_get_contact(mpig_bc_t * bc, const char * key, char ** value, int * flag);

void mpig_bc_free_contact(char * value);

int mpig_bc_serialize_object(mpig_bc_t * bc, char ** str);

void mpig_bc_free_serialized_object(char * str);

int mpig_bc_deserialize_object(const char *, mpig_bc_t * bc);

int mpig_bc_destroy(mpig_bc_t * bc);
/*<<<<<<<<<<<<<<<<<<<<<
  BUSINESS CARD SECTION
  <<<<<<<<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>>>>>>>>>
  PROCESS GROUP SECTION
  >>>>>>>>>>>>>>>>>>>>>*/
int mpig_pg_init(void);
int mpig_pg_finalize(void);
int mpig_pg_create(int vct_sz, mpig_pg_t ** pgp);
int mpig_pg_destroy(mpig_pg_t * pg);
void mpig_pg_add_ref(mpig_pg_t * pg);
void mpig_pg_release_ref(mpig_pg_t * pg, int * inuse);
int mpig_pg_find(char * id, mpig_pg_t ** pgp);
int mpig_pg_get_next(mpig_pg_t ** pgp);
int mpig_pg_get_size(mpig_pg_t * pg);
void mpig_pg_get_vc(mpig_pg_t * pg, int rank, struct mpig_vc ** vc);
void mpig_pg_id_set(mpig_pg_t * pg, const char * id);
void mpig_pg_id_clear(mpig_pg_t * pg);
int mpig_pg_compare_ids(const char * id1, const char * id2);


#define mpig_pg_add_ref(pg_)			\
{						\
    (pg_)->ref_count++;				\
}

#define mpig_pg_release_ref(pg_, inuse_)	\
{						\
    *(inuse_) = --(pg_)->ref_count;		\
}

#define mpig_pg_get_vc(pg_, rank_, vcp_)		\
{							\
    *(vcp_) = &(pg_)->vct[rank_];			\
}

#define mpig_pg_get_size(pg_) ((pg_)->size)

#define mpig_pg_id_set(pg_, id_) {(pg_)->id = MPIU_Strdup(id_);}

#define mpig_pg_id_clear(pg_)			\
{						\
    MPIU_Free((char *) (pg_)->id);		\
    (pg_)->id = NULL;				\
}

#define mpig_pg_compare_ids(id1_, id2_) (strcmp((id1_), (id2_)))
/*<<<<<<<<<<<<<<<<<<<<<
  PROCESS GROUP SECTION
  <<<<<<<<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>
  VIRTUAL CONNECTION SECTION
  >>>>>>>>>>>>>>>>>>>>>>>>>>*/
#define mpig_vc_create(vc_)					\
{								\
    (vc_)->ref_count = 0;					\
    mpig_vc_set_state((vc_), MPIG_VC_STATE_UNINITIALIZED);	\
    mpig_vc_set_cm_type((vc_), MPIG_CM_TYPE_NONE);		\
    (vc_)->pg = NULL;						\
    (vc_)->lpid = -1;						\
}

#define mpig_vc_destroy(vc_) {;}

#define mpig_vc_add_ref(vc_)			\
{						\
    (vc_)->ref_count++;				\
}

#define mpig_vc_release_ref(vc_, inuse_)	\
{						\
    *(inuse_) = --(vc_)->ref_count;		\
}

#define mpig_vc_set_state(vc_, state_) {(vc_)->state = (state_);}
#define mpig_vc_get_state(vc_) ((vc_)->state)

#define mpig_vc_set_cm_type(vc_, cm_type_) {(vc_)->cm_type = (cm_type_);}
#define mpig_vc_get_cm_type(vc_) ((vc_)->cm_type)

#define mpig_vc_set_cm_funcs(vc_, cm_funcs_) {(vc_)->cm_funcs = (cm_funcs_);}

/* XXX: MT: define */
#define mpig_vc_lock(vc)
#define mpig_vc_unlock(vc)
/*<<<<<<<<<<<<<<<<<<<<<<<<<<
  VIRTUAL CONNECTION SECTION
  <<<<<<<<<<<<<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>>>
  REQUEST SECTION
  >>>>>>>>>>>>>>>*/
#define mpig_request_set_envelope(req_, rank_, tag_, ctx_)	\
{								\
    (req_)->dev.rank = (rank_);					\
    (req_)->dev.tag = (tag_);					\
    (req_)->dev.ctx = (ctx_);					\
}

#define mpig_request_get_envelope(req_, rank_, tag_, ctx_)	\
{								\
    *(rank_) = (req_)->dev.rank;				\
    *(tag_) = (req_)->dev.tag;					\
    *(ctx_) = (req_)->dev.ctx;					\
}

#define mpig_request_set_buffer(req_, buf_, cnt_, dt_)	\
{							\
    (req_)->dev.buf = (buf_);				\
    (req_)->dev.cnt = (cnt_);				\
    (req_)->dev.dt = (dt_);				\
}

#define mpig_request_get_buffer(req_, buf_, cnt_, dt_)	\
{							\
    *(buf_) = (req_)->dev.buf;				\
    *(cnt_) = (req_)->dev.cnt;				\
    *(dt_) = (req_)->dev.dt;				\
}

#define mpig_request_set_sreq_id(req_, sreq_id_)	\
{							\
    (req_)->dev.sreq_id = (sreq_id_);			\
}

#define mpig_request_get_sreq_id(req_, sreq_id_)	\
{							\
    *(sreq_id_) = (req_)->dev.sreq_id;			\
}

#define mpig_request_add_comm(req_, comm_)					\
{										\
    /* XXX: MT: atomicity for MPIG and all of MPICH2???  Talk to Darius. */	\
    (req_)->comm = (comm_);							\
    MPIR_Comm_add_ref(comm_);							\
}

#define mpig_request_add_dt(req_, dt_)									\
{													\
    /* XXX: MT: atomicity for MPIG and all of MPICH2???  Talk to Darius. */				\
    if (HANDLE_GET_KIND(dt) != HANDLE_KIND_BUILTIN && HANDLE_GET_KIND(dt) != HANDLE_KIND_INVALID)	\
    {													\
	MPID_Datatype_get_ptr(dt, (req_)->dev.dtp);							\
	MPID_Datatype_add_ref((req_)->dev.dtp);								\
    }													\
}

#define mpig_request_get_type(req_) ((req_)->dev.type)

#define mpig_request_set_type(req_, type_)	\
{						\
    (req_)->dev.type = (type_);			\
}

/* XXX: MT: define */
#define mpig_request_lock_create(req_)
#define mpig_request_lock_destroy(req_)
#define mpig_request_lock(req_)
#define mpig_request_unlock(req_)


#define mpig_request_init(req_, kind_, type_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_)		\
{														\
    /* set MPICH fields */											\
    mpig_request_set_ref((req_), (ref_cnt_));									\
    (req_)->kind = (kind_);											\
    (req_)->cc_ptr = &(req_)->cc;										\
    mpig_request_set_cc((req_), (cc_));										\
    /* (req_)->comm = NULL; preset in create, use mpig_request_add_comm() to change */				\
    (req_)->partner_request = NULL;										\
    MPIR_Status_set_empty(&(req_)->status);									\
    (req_)->status.mpig_dc_format = GLOBUS_DC_FORMAT_LOCAL;							\
														\
    /* set device fields */											\
    mpig_request_set_type((req_), (type_));									\
    mpig_request_set_buffer((req_), (buf_), (cnt_), (dt_));							\
    mpig_request_set_envelope((req_), (rank_), (tag_), (ctx_));							\
    /* (req_)->dev.dtp = NULL; preset in create, use mpig_request_add_dt() to change */				\
    mpig_request_set_sreq_id((req_), MPI_REQUEST_NULL);								\
}

#define mpig_request_create_sreq(type_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, sreqp_)				 \
{																 \
    *(sreqp_) = mpig_request_create();												 \
    MPIU_ERR_CHKANDJUMP1((*(sreqp_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "send request");		 \
    mpig_request_init(*(sreqp_), MPID_REQUEST_SEND, (type_), (ref_cnt_), (cc_), (buf_), (cnt_), (dt_), (rank_), (tag_), (ctx_)); \
}

#define mpig_request_create_isreq(type_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, comm_, sreqp_)			 \
{																 \
    *(sreqp_) = mpig_request_create();												 \
    MPIU_ERR_CHKANDJUMP1((*(sreqp_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "send request");		 \
    mpig_request_init(*(sreqp_), MPID_REQUEST_SEND, (type_), (ref_cnt_), (cc_), (buf_), (cnt_), (dt_), (rank_), (tag_), (ctx_)); \
    mpig_request_add_comm(*(sreqp_), (comm_));											 \
    mpig_request_add_dt(*(sreqp_), (dt_));											 \
}

#define mpig_request_create_psreq(type_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, comm_, sreqp_)			  \
{																  \
    *(sreqp_) = mpig_request_create();												  \
    MPIU_ERR_CHKANDJUMP1((*(sreqp_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "persistent send request");	  \
    mpig_request_init(*(sreqp_), MPID_PREQUEST_SEND, (type_), (ref_cnt_), (cc_), (buf_), (cnt_), (dt_), (rank_), (tag_), (ctx_)); \
    mpig_request_add_comm(*(sreqp_), (comm_));											  \
    mpig_request_add_dt(*(sreqp_), (dt_));											  \
}

#define mpig_request_init_rreq(rreq_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_)					\
{																\
    mpig_request_init((rreq_), MPID_PREQUEST_RECV, MPIG_REQUEST_TYPE_RECV, (ref_cnt_), (cc_), (buf_), (cnt_), (dt_), (rank_),	\
		      (tag_), (ctx_));												\
}

#define mpig_request_create_rreq(ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, rreqp_)					\
{																\
    *(rreqp_) = mpig_request_create();												\
    MPIU_ERR_CHKANDJUMP1((*(rreqp_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "persistent receive request");	\
    mpig_request_init_rreq(*(rreqp_), (ref_cnt_), (cc_), (buf_), (cnt_), (dt_), (rank_), (tag_), (ctx_));			\
}

#define mpig_request_init_irreq(rreq_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, comm_)				\
{																\
    mpig_request_init((rreq_), MPID_REQUEST_RECV, MPIG_REQUEST_TYPE_RECV, (ref_cnt_), (cc_), (buf_), (cnt_), (dt_), (rank_),	\
		      (tag_), (ctx_));												\
    mpig_request_add_comm((rreq_), (comm_));											\
    mpig_request_add_dt((rreq_), (dt_));											\
}

#define mpig_request_create_irreq(ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, comm_, rreqp_)				\
{																\
    *(rreqp_) = mpig_request_create();												\
    MPIU_ERR_CHKANDJUMP1((*(rreqp_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "persistent receive request");	\
    mpig_request_init_rreq(*(rreqp_), (ref_cnt_), (cc_), (buf_), (cnt_), (dt_), (rank_), (tag_), (ctx_));			\
}

#define mpig_request_create_prreq(ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, comm_, rreqp_)				\
{																\
    *(rreqp_) = mpig_request_create();												\
    MPIU_ERR_CHKANDJUMP1((*(rreqp_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "persistent receive request");	\
    mpig_request_init(*(rreqp_), MPID_PREQUEST_RECV, MPIG_REQUEST_TYPE_RECV, (ref_cnt_), (cc_), (buf_), (cnt_), (dt_), (rank_),	\
		      (tag_), (ctx_));												\
    mpig_request_add_comm(*(rreqp_), (comm_));											\
    mpig_request_add_dt(*(rreqp_), (dt_));											\
}
/*<<<<<<<<<<<<<<<
  REQUEST SECTION
  <<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>>>>
  DATATYPE SECTION
  >>>>>>>>>>>>>>>>*/
#define mpig_datatype_get_info(count_, dt_, dt_contig_, data_sz_, dt_ptr_, dt_true_lb_)			\
{													\
    if (HANDLE_GET_KIND(dt_) == HANDLE_KIND_BUILTIN)							\
    {													\
	*(dt_ptr_) = NULL;										\
	*(dt_contig_) = TRUE;										\
        *(dt_true_lb_) = 0;										\
	*(data_sz_) = (count_) * MPID_Datatype_get_basic_size(dt_);					\
	MPIG_DBG_PRINTF((15, FCNAME, "basic datatype: dt_contig=%d, dt_sz=%d, data_sz=" MPIG_AINT_FMT,	\
			  *(dt_contig_), MPID_Datatype_get_basic_size(dt_), *(data_sz_)));		\
    }													\
    else												\
    {													\
	MPID_Datatype_get_ptr((dt_), *(dt_ptr_));							\
	*(dt_contig_) = (*(dt_ptr_))->is_contig;							\
	*(data_sz_) = (count_) * (*(dt_ptr_))->size;							\
        *(dt_true_lb_) = (*(dt_ptr_))->true_lb;								\
	MPIG_DBG_PRINTF((15, FCNAME, "user defined dt: dt_contig=%d, dt_sz=%d, data_sz=" MPIG_AINT_FMT,	\
			  *(dt_contig_), (*(dt_ptr_))->size, *(data_sz_)));				\
    }													\
}
/*<<<<<<<<<<<<<<<<
  DATATYPE SECTION
  <<<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>>>>>>>>>
  RECEIVE QUEUE SECTION
  >>>>>>>>>>>>>>>>>>>>>*/
int mpig_recvq_init(void);

struct MPID_Request * mpig_recvq_find_unexp(int rank, int tag, int ctx);

struct MPID_Request * mpig_recvq_deq_unexp_sreq(int rank, int tag, int ctx, MPI_Request sreq_id);

struct MPID_Request * mpig_recvq_deq_unexp_or_enq_posted(int rank, int tag, int ctx, int * found);

int mpig_recvq_deq_posted_rreq(struct MPID_Request * rreq);

struct MPID_Request * mpig_recvq_deq_posted(int rank, int tag, int ctx);

struct MPID_Request * mpig_recvq_deq_posted_or_enq_unexp(int rank, int tag, int ctx, int * found);
/*<<<<<<<<<<<<<<<<<<<<<
  RECEIVE QUEUE SECTION
  <<<<<<<<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>
  PROCESS MANAGEMENT SECTION
  >>>>>>>>>>>>>>>>>>>>>>>>>>*/
int mpig_pm_init(void);

int mpig_pm_finalize(void);

int mpig_pm_exchange_business_cards(mpig_bc_t * bc, mpig_bc_t ** bcs);

int mpig_pm_free_business_cards(mpig_bc_t * bcs);

int mpig_pm_get_pg_size(int * pg_size);

int mpig_pm_get_pg_rank(int * pg_rank);

int mpig_pm_get_pg_id(const char ** pg_id);
/*<<<<<<<<<<<<<<<<<<<<<<<<<<
  PROCESS MANAGEMENT SECTION
  <<<<<<<<<<<<<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>>>>>>>>>>>>
  DEBUGGING OUTPUT SECTION
  >>>>>>>>>>>>>>>>>>>>>>>>*/
void mpig_dbg_printf(int level, const char * fcname, const char * fmt, ...);
void mpig_dbg_vprintf(int level, const char * fcname, const char * fmt, va_list ap);

#if defined(MPICH_DBG_OUTPUT)
#define MPIG_DBG_PRINTF(e_)			\
{						\
    if (MPIUI_dbg_state != MPIU_DBG_STATE_NONE)	\
    {						\
        mpig_dbg_printf e_;			\
    }						\
}
#define MPIG_DBG_VPRINTF(level_, fcname_, fmt_, ap_)		\
{								\
    if (MPIUI_dbg_state != MPIU_DBG_STATE_NONE)			\
    {								\
        mpig_dbg_vprintf((level_), (fcname_), (fmt_), (ap_));	\
    }								\
}
#else
#define MPIG_DBG_PRINTF(e_)
#define MPIG_DBG_VPRINTF(e_)
#endif

#if defined(HAVE_CPP_VARARGS)
#define mpig_dbg_printf(level_, func_, fmt_, args_...)										  \
{																  \
    MPIU_dbglog_printf("[%s:%d:%d] %s():%d: " fmt_ "\n", mpig_process.my_pg_id, mpig_process.my_pg_rank, 0 /* MT: thread id */,   \
		       (func_), (level_), ## args_);										  \
}
#endif
/*<<<<<<<<<<<<<<<<<<<<<<<<
  DEBUGGING OUTPUT SECTION
  <<<<<<<<<<<<<<<<<<<<<<<<*/

#endif /* MPICH2_MPIDIMPL_H_INCLUDED */
