/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIDPRE_H_INCLUDED)
#define MPICH2_MPIDPRE_H_INCLUDED

#include "mpidconf.h"
#include "mpid_datatype.h"

#define MPIG_AINT_FMT "%d"  /* XXX: get this from mpich2prereq */


struct MPID_Comm;
struct MPID_Request;

/*>>>>>>>>>>>>>>>>>>>>>>>>
  VENDOR MPI TYPES SECTION
  >>>>>>>>>>>>>>>>>>>>>>>>*/
#if defined(MPIG_VMPI)
typedef MPIG_ALIGNED_T mpig_vmpi_comm_t[(SIZEOF_VMPI_COMM + SIZEOF_MPIG_ALIGNED_T - 1) / SIZEOF_MPIG_ALIGNED_T];
typedef MPIG_ALIGNED_T mpig_vmpi_datatype_t[(SIZEOF_VMPI_DATATYPE + SIZEOF_MPIG_ALIGNED_T - 1) / SIZEOF_MPIG_ALIGNED_T];
typedef MPIG_ALIGNED_T mpig_vmpi_request_t[(SIZEOF_VMPI_REQUEST + SIZEOF_MPIG_ALIGNED_T - 1) / SIZEOF_MPIG_ALIGNED_T];
#endif
/*<<<<<<<<<<<<<<<<<<<<<<<<
  VENDOR MPI TYPES SECTION
  <<<<<<<<<<<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>>>>>>>>>
  BUSINESS CARD SECTION
  >>>>>>>>>>>>>>>>>>>>>*/
typedef struct mpig_bc
{
    char * str_begin;
    char * str_end;
    int str_size;
    int str_left;
}
mpig_bc_t;

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

/*>>>>>>>>>>>>>>>>>>>>>>>>
  MESSAGE ENVELOPE SECTION
  >>>>>>>>>>>>>>>>>>>>>>>>*/
/* NOTE: MPIG_TAG_UB should be set to the maximum value of a tag.  This is used in MPID_Init() to set the MPI_TAG_UB attribute
   on MPI_COMM_WORLD.  As specified by the MPI-1 standard, this value may not be less than 32767. */
#define MPIG_TAG_UB (0x7fffffff)

typedef int16_t mpig_rank_t;
typedef int32_t mpig_tag_t;
typedef int16_t mpig_ctx_t;

typedef struct mpig_envelope
{
    mpig_tag_t tag;
    mpig_rank_t rank;
    mpig_ctx_t ctx;
}
mpig_envelope_t;

#define mpig_envelope_equal(envl1_, envl2_) \
    ((envl1_)->ctx == (envl2_)->ctx && (envl1_)->rank == (envl2_)->rank && (envl1_)->tag == (envl2_)->tag)

#define mpig_envelope_equal_tuple(envl_, rank_, tag_, ctx_) \
    ((envl_)->ctx == (ctx_) && (envl_)->rank == (rank_) && (envl_)->tag == (tag_))

#define mpig_envelope_equal_masked(envl1_, envl2_, mask_)					\
    ((envl1_)->ctx == (envl2_)->ctx && ((envl1_)->rank & (mask_)->rank) == (envl2_)->rank &&	\
     ((envl1_)->tag & (mask_)->tag) == (envl2_)->tag)
/*<<<<<<<<<<<<<<<<<<<<<<<<
  MESSAGE ENVELOPE SECTION
  <<<<<<<<<<<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>>>>>>>>>
  PROCESS GROUP SECTION
  >>>>>>>>>>>>>>>>>>>>>*/
typedef struct mpig_pg
{
    volatile int ref_count;

    /* Number of processes in the process group */
    int size;

    /* VC table.  At present this is a pointer to an array of VC structures.  Someday we may want make this a pointer to an array
       of VC references.  Thus, it is important to use mpig_pg_get_vc() instead of directly referencing this field. */
    struct mpig_vc * vct;

    /* Unique ID for the process group ID.  This is required for MPI-2 dynamic process functionality. */
    char * id;
    
    /* Next pointer used to maintain a list of all process groups known to this process */
    struct mpig_pg * next;
}
mpig_pg_t;

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

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  CONNECTION MANAGEMENT SECTION
  >>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
#include "mpig_cm_self.h"
#include "mpig_cm_vmpi.h"
#include "mpig_cm_xio.h"
#include "mpig_cm_other.h"

typedef int (*mpig_cm_adi3_send_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
				      int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_cm_adi3_isend_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
				       int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_cm_adi3_rsend_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
				       int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_cm_adi3_irsend_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
					int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_cm_adi3_ssend_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
				       int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_cm_adi3_issend_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
					int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_cm_adi3_recv_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
				      int ctxoff, MPI_Status * status, struct MPID_Request ** reqp);

typedef int (*mpig_cm_adi3_irecv_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
				       int ctxoff, struct MPID_Request ** reqp);

typedef struct mpig_cm_funcs
{
    mpig_cm_adi3_send_fn_t adi3_send;
    mpig_cm_adi3_isend_fn_t adi3_isend;
    mpig_cm_adi3_rsend_fn_t adi3_rsend;
    mpig_cm_adi3_irsend_fn_t adi3_irsend;
    mpig_cm_adi3_ssend_fn_t adi3_ssend;
    mpig_cm_adi3_issend_fn_t adi3_issend;
    mpig_cm_adi3_recv_fn_t adi3_recv;
    mpig_cm_adi3_irecv_fn_t adi3_irecv;
}
mpig_cm_funcs_t;
    
typedef enum mpig_cm_type
{
    MPIG_CM_TYPE_NONE = 0
    MPIG_CM_TYPE_SELF_LIST
    MPIG_CM_TYPE_VMPI_LIST
    MPIG_CM_TYPE_XIO_LIST
    MPIG_CM_TYPE_OTHER_LIST
}
mpig_cm_type_t;

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  CONNECTION MANAGEMENT SECTION
  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>
  VIRTUAL CONNECTION SECTION
  >>>>>>>>>>>>>>>>>>>>>>>>>>*/
/*
 * MPID_VCRT/MPID_VCR
 *
 * MPID_VCRT is the virtual connection reference table object.  MPID_VCR is an array of virtual connection references, allowing
 * the MPICH and device layers fast access to the items in the table.
 *
 * XXX: These structures should not be exposed as fields in the MPICH layer of MPID_Comm object.  A better technique would be to
 * define an interface that allows the MPICH layer to access the information without having any exposure to to the data
 * structures themselves.  We should work with the MPICH folks to define such an interface.
 */
typedef struct mpig_vcrt * MPID_VCRT;
typedef struct mpig_vc * MPID_VCR;

#define mpig_comm_get_vc(comm_, rank_)  (((rank_) >= 0) ? ((comm_)->vcr[(rank_)]) : (mpig_cm_other_vc))

typedef enum mpig_vc_state
{
    MPIG_VC_STATE_UNINITIALIZED = 0,
    MPIG_VC_STATE_UNCONNECTED,
    MPIG_VC_STATE_CONNECTING,
    MPIG_VC_STATE_CONNECTED, 
    MPIG_VC_STATE_DISCONNECTING,
    MPIG_VC_STATE_TEMPORARY,
}
mpig_vc_state_t;

typedef struct mpig_vc
{
    volatile int ref_count;

    mpig_vc_state_t state;

    /* Process group to which the process associated with this VC belongs, and the rank of that process in the process group */
    struct mpig_pg * pg;
    int pg_rank;

    /* Local process ID */
    int lpid;

    mpig_cm_type_t cm_type;
    mpig_cm_funcs_t cm_funcs;
    union
    {
	MPIG_VC_CM_SELF_DECL
	MPIG_VC_CM_VMPI_DECL
	MPIG_VC_CM_XIO_DECL
	MPIG_VC_CM_OTHER_DECL
    }
    cm;
}
mpig_vc_t;

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
#define MPIG_REQUEST_DEV_DECL													 \
struct mpig_request_dev														 \
{																 \
    /* Message envelope data (rank, tag, context id) */										 \
    mpig_envelope_t envl;													 \
																 \
    /* Application buffer */													 \
    void * buf;															 \
    int cnt;															 \
    MPI_Datatype dt;														 \
																 \
    /* Pointer to datatype for reference counting purposes.  The datatype must be kept alive until the request is complete, even \
       if the user were to free it. */												 \
    struct MPID_Datatype * dtp;													 \
																 \
    /* Various device state information.  Bits and access macros are defined in mpidimpl.h. */					 \
    u_int32_t state;														 \
																 \
    /* The send request id (handle) is stored with all requests on the unexpected queue.  Among other things, this information	 \
       allows a remote cancel send handler to identify the correct request to remove from the unexpected queue.  This field is	 \
       not used by the VMPI communication module, but is required by the receive queue code, and thus is part of the general	 \
       request structure. */													 \
    int sreq_id;														 \
																 \
    /* Pointer allowing the request to be inserted into any number of lists/queues */						 \
    struct MPID_Request * next;													 \
} dev;

#define mpig_request_set_envelope(req_, rank_, tag_, ctx_)	\
{								\
    (req_)->dev.envl.rank = (rank_);				\
    (req_)->dev.envl.tag = (tag_);				\
    (req_)->dev.envl.ctx = (ctx_);				\
}

#define mpig_request_set_buffer(req_, buf_, cnt_, dt_)	\
{							\
    (req_)->dev.buf = (buf_);				\
    (req_)->dev.cnt = (cnt_);				\
    (req_)->dev.dt = (dt_);				\
}

#define MPID_DEV_REQUEST_DECL			\
MPIG_REQUEST_DEV_DECL				\
union						\
{						\
    MPIG_REQUEST_CM_SELF_DECL			\
    MPIG_REQUEST_CM_VMPI_DECL			\
    MPIG_REQUEST_CM_XIO_DECL			\
    MPIG_REQUEST_CM_OTHER_DECL			\
} cm;

/* Masks and flags for channel device state in an MPID_Request */
#define mpig_request_state_init(req_)		\
{						\
    (req_)->dev.state = 0;			\
}

#define MPIG_REQUEST_TYPE_SHIFT 0
#define MPIG_REQUEST_TYPE_MASK (0xF << MPIG_REQUEST_TYPE_SHIFT)
#define MPIG_REQUEST_TYPE_RECV 0
#define MPIG_REQUEST_TYPE_SEND 1
#define MPIG_REQUEST_TYPE_RSEND 2
#define MPIG_REQUEST_TYPE_SSEND 3
#define MPIG_REQUEST_TYPE_BSEND 4

#define mpig_request_get_type(req_) \
(((req_)->dev.state & MPIG_REQUEST_TYPE_MASK) >> MPIG_REQUEST_TYPE_SHIFT)

#define mpig_request_set_type(req_, type_)							\
{												\
    (req_)->dev.state &= ~MPIG_REQUEST_TYPE_MASK;						\
    (req_)->dev.state |= ((type_) << MPIG_REQUEST_TYPE_SHIFT) & MPIG_REQUEST_TYPE_MASK;	\
}

#define MPIG_REQUEST_PROTO_SHIFT 4
#define MPIG_REQUEST_PROTO_MASK (0x3 << MPIG_REQUEST_PROTO_SHIFT)
#define MPIG_REQUEST_PROTO_SELF 0
#define MPIG_REQUEST_PROTO_VMPI 3
#define MPIG_REQUEST_PROTO_XIO 1

#define mpig_request_get_proto(req_) \
(((req_)->dev.state & MPIG_REQUEST_PROTO_MASK) >> MPIG_REQUEST_PROTO_SHIFT)

#define mpig_request_set_proto(req_, proto_)							\
{												\
    (req_)->dev.state &= ~MPIG_REQUEST_PROTO_MASK;						\
    (req_)->dev.state |= ((proto_) << MPIG_REQUEST_PROTO_SHIFT) & MPIG_REQUEST_PROTO_MASK;	\
}

#define mpig_request_get_cancel_pending(req_) \
(((req_)->dev.state & MPIG_REQUEST_CANCEL_MASK) >> MPIG_REQUEST_CANCEL_SHIFT)

#define mpig_request_set_cancel_pending(req_)			\
{								\
    (req_)->dev.state |= 1 << MPIG_REQUEST_CANCEL_SHIFT;	\
}

#define mpig_request_clear_cancel_pending(req_)	\
{							\
    (req_)->dev.state &= ~MPIG_REQUEST_CANCEL_MASK;	\
}

/* XXX: MT: define */
#define mpig_request_lock(req)
#define mpig_request_unlock(req)
/*<<<<<<<<<<<<<<<
  REQUEST SECTION
  <<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>>>>>>>>>
  RECEIVE QUEUE SECTION
  >>>>>>>>>>>>>>>>>>>>>*/
int mpig_recvq_init(void);

struct MPID_Request * mpig_recvq_find_unexp(mpig_rank_t rank, mpig_tag_t tag, mpig_ctx_t ctx);

struct MPID_Request * mpig_recvq_deq_unexp_sreq(mpig_envelope_t * envl, MPI_Request sreq_id);

struct MPID_Request * mpig_recvq_deq_unexp_or_enq_posted(mpig_rank_t rank, mpig_tag_t tag, mpig_ctx_t ctx, int * found);

int mpig_recvq_deq_posted_rreq(struct MPID_Request * rreq);

struct MPID_Request * mpig_recvq_deq_posted(mpig_envelope_t * envl);

struct MPID_Request * mpig_recvq_deq_posted_or_enq_unexp(mpig_envelope_t * envl, int * found);
/*<<<<<<<<<<<<<<<<<<<<<
  RECEIVE QUEUE SECTION
  <<<<<<<<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>>>>>>>>>>>
  PROGRESS ENGINE SECTION
  >>>>>>>>>>>>>>>>>>>>>>>*/
/*
 * MPID_PROGRESS_STATE_DECL
 *
 * This state object is used to prevent MPID_Progress_wait() from erroneous blocking if progress has occurred between
 * MPID_Progress_start() and MPID_Progress_wait().  The state object is allocated on the stack, and initialized by
 * MPID_Progress_start().  MPID_Progress_end() is only called if MPID_Progress_wait() is not, so any cleanup of data structures
 * in the state object must occur in both routines.
 */
struct MPIDI_Progress_state
{
    /* snapshot of the progress engine completion counter */
    int count;
};

#define MPID_PROGRESS_STATE_DECL struct MPIDI_Progress_state dev;
/*<<<<<<<<<<<<<<<<<<<<<<<
  PROGRESS ENGINE SECTION
  <<<<<<<<<<<<<<<<<<<<<<<*/


#endif /* MPICH2_MPIDPRE_H_INCLUDED */
