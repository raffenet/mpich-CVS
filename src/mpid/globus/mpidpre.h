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

struct MPID_Comm;
struct MPID_Request;
struct mpig_bc;
struct mpig_pg;
struct mpig_vc;

#include "mpig_cm_self.h"
#include "mpig_cm_vmpi.h"
#if XXX
#define MPIG_CM_TYPE_VMPI_LIST
#define MPIG_VC_CM_VMPI_DECL
#define MPIG_REQUEST_CM_VMPI_DECL
#else
#include "mpig_cm_xio.h"
#endif
#include "mpig_cm_other.h"

/* NOTE: MPIG_TAG_UB should be set to the maximum value of a tag.  This is used in MPID_Init() to set the MPI_TAG_UB attribute
   on MPI_COMM_WORLD.  As specified by the MPI-1 standard, this value may not be less than 32767. */
#define MPIG_TAG_UB (0x7fffffff)

#define MPIG_AINT_FMT "%d"  /* XXX: get this from mpich2prereq */


/**********************************************************************************************************************************
						 BEGIN VENDOR MPI TYPES SECTION
**********************************************************************************************************************************/
#if defined(MPIG_VMPI)
typedef MPIG_ALIGNED_T mpig_vmpi_comm_t[(SIZEOF_VMPI_COMM + SIZEOF_MPIG_ALIGNED_T - 1) / SIZEOF_MPIG_ALIGNED_T];
typedef MPIG_ALIGNED_T mpig_vmpi_datatype_t[(SIZEOF_VMPI_DATATYPE + SIZEOF_MPIG_ALIGNED_T - 1) / SIZEOF_MPIG_ALIGNED_T];
typedef MPIG_ALIGNED_T mpig_vmpi_request_t[(SIZEOF_VMPI_REQUEST + SIZEOF_MPIG_ALIGNED_T - 1) / SIZEOF_MPIG_ALIGNED_T];
#endif
/**********************************************************************************************************************************
						  END VENDOR MPI TYPES SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						     BEGIN DATATYPE SECTION
**********************************************************************************************************************************/
typedef enum mpig_ftype
{
    MPIG_FTYPE_CHARACTER,
    MPIG_FTYPE_LOGICAL,
    MPIG_FTYPE_INTEGER,
    MPIG_FTYPE_INTEGER1,
    MPIG_FTYPE_INTEGER2,
    MPIG_FTYPE_INTEGER4,
    MPIG_FTYPE_INTEGER8,
    MPIG_FTYPE_INTEGER16,
    MPIG_FTYPE_REAL,
    MPIG_FTYPE_REAL2,
    MPIG_FTYPE_REAL4,
    MPIG_FTYPE_REAL8,
    MPIG_FTYPE_REAL16,
    MPIG_FTYPE_DOBULE_PRECISION,
    MPIG_FTYPE_COMPLEX,
    MPIG_FTYPE_COMPLEX4,
    MPIG_FTYPE_COMPLEX8,
    MPIG_FTYPE_COMPLEX16,
    MPIG_FTYPE_COMPLEX32,
    MPIG_FTYPE_DOUBLE_COMPLEX
}
mpig_ftype_t;

typedef enum mpig_ctype
{
    MPIG_CTYPE_NONE,
    MPIG_CTYPE_CHAR,
    MPIG_CTYPE_SHORT,
    MPIG_CTYPE_INT,
    MPIG_CTYPE_LONG,
    MPIG_CTYPE_LONG_LONG,
    MPIG_CTYPE_FLOAT,
    MPIG_CTYPE_DOUBLE,
    MPIG_CTYPE_LONG_DOUBLE
}
mpig_ctype_t;
/**********************************************************************************************************************************
						      END DATATYPE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN BUSINESS CARD SECTION
**********************************************************************************************************************************/
typedef struct mpig_bc
{
    char * str_begin;
    char * str_end;
    int str_size;
    int str_left;
}
mpig_bc_t;
/**********************************************************************************************************************************
						    END BUSINESS CARD SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					       BEGIN CONNECTION MANAGEMENT SECTION
**********************************************************************************************************************************/
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

typedef int (*mpig_cm_adi3_recv_fn_t)(void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
				      int ctxoff, MPI_Status * status, struct MPID_Request ** reqp);

typedef int (*mpig_cm_adi3_irecv_fn_t)(void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
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
/**********************************************************************************************************************************
						END CONNECTION MANAGEMENT SECTION
**********************************************************************************************************************************/

/**********************************************************************************************************************************
						   BEGIN PROCESS GROUP SECTION
**********************************************************************************************************************************/
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
/*<<<<<<<<<<<<<<<<<<<<<
  PROCESS GROUP SECTION
  <<<<<<<<<<<<<<<<<<<<<*/

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
    /* mutex tp protect data and insure RC systems see updates */
    globus_mutex_t mutex;
    
    volatile int ref_count;

    /* connection state */
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
/**********************************************************************************************************************************
						 END VIRTUAL CONNECTION SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						      BEGIN REQUEST SECTION
**********************************************************************************************************************************/
typedef enum mpig_request_type
{
    MPIG_REQUEST_TYPE_RECV = 1,
    MPIG_REQUEST_TYPE_SEND,
    MPIG_REQUEST_TYPE_RSEND,
    MPIG_REQUEST_TYPE_SSEND,
    MPIG_REQUEST_TYPE_BSEND
}
mpig_request_type_t;

#define MPIG_REQUEST_DEV_DECL													 \
struct mpig_request_dev														 \
{																 \
    /* Request type (combine with the top-level 'kind' field to determine exact nature of the request) */			 \
    mpig_request_type_t type;													 \
 																 \
    /* Message envelope data (rank, tag, context id) */										 \
    int rank;															 \
    int tag;															 \
    int ctx;															 \
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
    /* The send request id (handle) is stored with all requests on the unexpected queue.  Among other things, this information	 \
       allows a remote cancel send handler to identify the correct request to remove from the unexpected queue.  This field is	 \
       not used by the VMPI communication module, but is required by the receive queue code, and thus is part of the general	 \
       request structure. */													 \
    int sreq_id;														 \
																 \
    /* Request cancelled flag */												 \
    int cancelled;														 \
																 \
    /* Pointer allowing the request to be inserted into any number of lists/queues */						 \
    struct MPID_Request * next;													 \
} dev;

#define MPID_DEV_REQUEST_DECL			\
MPIG_REQUEST_DEV_DECL				\
union						\
{						\
    MPIG_REQUEST_CM_SELF_DECL			\
    MPIG_REQUEST_CM_VMPI_DECL			\
    MPIG_REQUEST_CM_XIO_DECL			\
    MPIG_REQUEST_CM_OTHER_DECL			\
} cm;
/**********************************************************************************************************************************
						       END REQUEST SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						  BEGIN PROGRESS ENGINE SECTION
**********************************************************************************************************************************/
/*
 * MPID_PROGRESS_STATE_DECL
 *
 * This state object is used to prevent MPID_Progress_wait() from erroneous blocking if progress has occurred between
 * MPID_Progress_start() and MPID_Progress_wait().  The state object is allocated on the stack, and initialized by
 * MPID_Progress_start().  MPID_Progress_end() is only called if MPID_Progress_wait() is not, so any cleanup of data structures
 * in the state object must occur in both routines.
 */
struct mpig_Progress_state
{
    /* snapshot of the progress engine completion counter */
    int count;
};

#define MPID_PROGRESS_STATE_DECL struct mpig_Progress_state dev;
/**********************************************************************************************************************************
END PROGRESS ENGINE SECTION
**********************************************************************************************************************************/

#endif /* MPICH2_MPIDPRE_H_INCLUDED */
