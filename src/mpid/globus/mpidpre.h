/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIDPRE_H_INCLUDED)
#define MPICH2_MPIDPRE_H_INCLUDED

/* compile time contants */
#if !defined(MPIG_ERR_STRING_SIZE)
#define MPIG_ERR_STRING_SIZE 32768
#endif

#include "mpidconf.h"

/* include common tools provided by Globus */
#include "globus_common.h"

/* miscellaneous core types */
#if !defined(HAVE_BOOL_T)
typedef int bool_t;
#endif
typedef TYPEOF_MPIG_ALIGNED_T mpig_aligned_t;

/* NOTE: the values ofTRUE and FALSE must match the values set in mpig_vmpi.c */
#undef FALSE
#define FALSE 0
#undef TRUE
#define TRUE 1

/* declare the existence of MPICH2 and MPIG structures that cannot be defined util later */
struct MPID_Comm;
struct MPID_Request;
struct MPID_Progress_state;
struct mpig_bc;
struct mpig_pg;
struct mpig_vc;

/* macros to make printing values of types that vary with the platform a little easier */
#define MPIG_HANDLE_FMT "0x%08x"    /* format of an MPICH2 object handle; assumed to be at most 32-bits */
#define MPIG_ERRNO_FMT "0x%08x"	    /* format of an MPI error code; assumed to be at most 32-bits */
#define MPIG_HANDLE_VAL(object_) (((object_) != NULL) ? (object_)->handle : 0)
#define MPIG_STR_VAL(str_) ((str_) != NULL ? (str_) : "(null)")


/**********************************************************************************************************************************
					       BEGIN COMMUNICATION MODULE SECTION
**********************************************************************************************************************************/
/* FIXME: this list should be generated at configure time, perhaps by scanning files with a pattern of mpig_cm_*.h. */
typedef enum mpig_cm_type
{
    MPIG_CM_TYPE_FIRST = 0,
    MPIG_CM_TYPE_UNDEFINED,
    MPIG_CM_TYPE_SELF,
    MPIG_CM_TYPE_VMPI,
    MPIG_CM_TYPE_XIO,
    MPIG_CM_TYPE_OTHER,
    MPIG_CM_TYPE_LAST
}
mpig_cm_type_t;

#define MPIG_CM_NUM_TYPES (MPIG_CM_TYPE_LAST - MPIG_CM_TYPE_FIRST - 2)

/*
 * mpig_cm_init([IN/OUT] argc, [IN/OUT] argv, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * Parameters:
 *
 * argc [IN/OUT] - number of arguments in the argv array.  this may be modified communication module.
 *
 * argv [IN/OUT] - array of command line arguments.  arguments may be added or removed as appropriate.
 *
 * mpi_errno [IN/OUT] - MPI error code
 *
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 */
typedef void (*mpig_cm_init_fn_t)(int * argc, char *** argv, int * mpi_errno_p, bool_t * failed_p);

/*
 * mpig_cm_finalize([IN/OUT] mpi_errno, [OUT] failed)
 *
 * shutdown down the communication module.  all pending communication using this module must complete before the routine returns.
 *
 * Parameters:
 *
 * mpi_errno [IN/OUT] - MPI error code
 *
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 */
typedef void (*mpig_cm_finalize_fn_t)(int * mpi_errno_p, bool_t * failed_p);

/*
 * mpig_cm_add_contact_info([IN/MOD] bc, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * Add contact information for the current process to the supplied business card.
 *
 * Parameters:
 *
 * bc [IN/MOD] - business card object in which to store contact information for this process
 *
 * mpi_errno [IN/OUT] - MPI error code
 *
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 */
typedef void (*mpig_cm_add_contact_info_fn_t)(struct mpig_bc * bc, int * mpi_errno_p, bool_t * failed_p);

/*
 * mpig_cm_extract_contact_info([IN/MOD] vc, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * extract contact information from the business card and store it in machine processable form
 *
 * NOTE: this routine assumes the that business card has been attached to the VC
 *
 * Parameters:
 *
 * vc [IN/MOD] - vc object in which to place contact information
 *
 * mpi_errno [IN/OUT] - MPI error code
 *
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 */
typedef void (*mpig_cm_extract_contact_info_fn_t)(struct mpig_vc * vc, int * mpi_errno_p, bool_t * failed_p);

/*
 * void mpig_cm_select_module([IN/MOD] vc, [OUT] selected, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * determine if the current process can use the communication module to communicate with the process associated with the supplied
 * VC.  if it can, then the VC will be initialized accordingly.  if the VC has already been selected by another communication
 * module, the routine will return with 'selected' equal to FALSE.
 *
 * NOTE: this routine assumes the that mpig_cm_extract_contact_info() has be called
 *
 * Parameters:
 *
 * vc [IN] - vc object to initialize if the communication module is capable of performing communication with the process
 *
 * selected [OUT] - TRUE if the communication module can communicate with the remote process; otherwise FALSE
 *
 * mpi_errno [IN/OUT] - MPI error code
 *
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 */
typedef void (*mpig_cm_select_module_fn_t)(struct mpig_vc * vc, bool_t * selected, int * mpi_errno_p, bool_t * failed_p);

/*
 * void mpig_cm_get_vc_compatability([IN] vc1, [IN] vc2, [IN] levels_in, [OUT] levels_out, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * determine if the two VCs supplied to the routine are capable of communicating at any of the predefined topology levels using
 * the communication module.
 *
 * NOTE: this routine assumes the that mpig_cm_extract_contact_info() has be called for both VCs
 *
 * Parameters:
 *
 * vc1 [IN] - first VC
 *
 * vc2 [IN[ - second VC
 *
 * levels_in [IN] - a bitmask defining the topology levels of interest
 *
 * levels_out [OUT] - a bitmask containing the topology levels are shared by the two VCs (a subset of levels_in)
 *
 * mpi_errno [IN/OUT] - MPI error code
 *
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 */
typedef void (*mpig_cm_get_vc_compatability_fn_t)(const struct mpig_vc * vc1, const struct mpig_vc * vc2,
    unsigned levels_in, unsigned * levels_out, int * mpi_errno_p, bool_t * failed_p);

typedef char * (*mpig_cm_vtable_last_entry_fn_t)(int foo, float bar, const short * baz, char bif);

typedef struct mpig_cm_vtable
{
    mpig_cm_type_t type;
    const char * name;
    mpig_cm_init_fn_t			init;
    mpig_cm_finalize_fn_t		finalize;
    mpig_cm_add_contact_info_fn_t	add_contact_info;
    mpig_cm_extract_contact_info_fn_t	extract_contact_info;
    mpig_cm_select_module_fn_t		select_module;
    mpig_cm_get_vc_compatability_fn_t	get_vc_compatability;
    mpig_cm_vtable_last_entry_fn_t	vtable_last_entry;
}
mpig_cm_vtable_t;

/*
 * include communication module defintions and declarations
 *
 * FIXME: the list of configuration module header files should be generated at configure time, perhaps by scanning for files who
 * names match the pattern mpig_cm_*.h.
 */
#include "mpig_cm_self.h"
#include "mpig_cm_vmpi.h"
#include "mpig_cm_xio.h"
#include "mpig_cm_other.h"
/**********************************************************************************************************************************
						END COMMUNICATION MODULE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN COMMUNICATOR SECTION
**********************************************************************************************************************************/
#define MPIG_COMM_NUM_CONTEXTS 2

/* set of processes which will have to talk together at a given level */
typedef struct mpig_comm_set
{
    int size;               /* number of processes in the set */
    int root_index;         /* position of the root process in the set */
    int my_rank_index;      /* position of the current process in the set */
    int * set;               /* array of process ids in the set */
}
mpig_comm_set_t;

typedef struct mpig_comm
{
    /* topology information */
    int * topology_depths;
    int ** topology_colors;
    int ** topology_ranks;
    int ** topology_cluster_ids;
    int ** topology_cluster_sizes;
    mpig_comm_set_t * topology_comm_sets;
    int topology_max_depth;
    
    /* data structures for tracking active communicators to prevent MPID_Finalize from returning before all outstanding
       communication has completed. */
    bool_t user_ref;
    struct MPID_Comm * active_list_prev;
    struct MPID_Comm * active_list_next;
}
mpig_comm_t;

/* FIXME: the defintion of MPIG_COMM_CM_DECL should be generated by configure based on a list of communication modules. */
#if !defined(MPIG_COMM_CM_SELF_DECL)
#define MPIG_COMM_CM_SELF_DECL
#else
#undef MPIG_COMM_CM_DECL_STRUCT_DEFINED
#define MPIG_COMM_CM_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_COMM_CM_VMPI_DECL)
#define MPIG_COMM_CM_VMPI_DECL
#else
#undef MPIG_COMM_CM_DECL_STRUCT_DEFINED
#define MPIG_COMM_CM_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_COMM_CM_XIO_DECL)
#define MPIG_COMM_CM_XIO_DECL
#else
#undef MPIG_COMM_CM_DECL_STRUCT_DEFINED
#define MPIG_COMM_CM_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_COMM_CM_OTHER_DECL)
#define MPIG_COMM_CM_OTHER_DECL
#else
#undef MPIG_COMM_CM_DECL_STRUCT_DEFINED
#define MPIG_COMM_CM_DECL_STRUCT_DEFINED
#endif

#if defined(MPIG_COMM_CM_DECL_STRUCT_DEFINED)
#define MPIG_COMM_CM_DECL	\
    struct			\
    {				\
	MPIG_COMM_CM_SELF_DECL	\
	MPIG_COMM_CM_VMPI_DECL	\
	MPIG_COMM_CM_XIO_DECL	\
	MPIG_COMM_CM_OTHER_DECL	\
    } cm;
#else
#define MPIG_COMM_CM_DECL
#endif

#define MPID_DEV_COMM_DECL \
    mpig_comm_t dev;	   \
    MPIG_COMM_CM_DECL

#define HAVE_DEV_COMM_HOOK
/**********************************************************************************************************************************
						    END COMMUNICATOR SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						     BEGIN DATATYPE SECTION
**********************************************************************************************************************************/
#define MPIG_DATATYPE_MAX_BASIC_TYPES 64

typedef enum mpig_ctype
{
    MPIG_CTYPE_NONE = 0,
    MPIG_CTYPE_CHAR,
    MPIG_CTYPE_SHORT,
    MPIG_CTYPE_INT,
    MPIG_CTYPE_LONG,
    MPIG_CTYPE_LONG_LONG,
    MPIG_CTYPE_FLOAT,
    MPIG_CTYPE_DOUBLE,
    MPIG_CTYPE_LONG_DOUBLE,
    MPIG_CTYPE_LAST
}
mpig_ctype_t;

/* FIXME: the defintion of MPIG_DATATYPE_CM_DECL should be generated by configure based on a list of communication modules. */
#if !defined(MPIG_DATATYPE_CM_SELF_DECL)
#define MPIG_DATATYPE_CM_SELF_DECL
#else
#undef MPIG_DATATYPE_CM_DECL_STRUCT_DEFINED
#define MPIG_DATATYPE_CM_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_DATATYPE_CM_VMPI_DECL)
#define MPIG_DATATYPE_CM_VMPI_DECL
#else
#undef MPIG_DATATYPE_CM_DECL_STRUCT_DEFINED
#define MPIG_DATATYPE_CM_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_DATATYPE_CM_XIO_DECL)
#define MPIG_DATATYPE_CM_XIO_DECL
#else
#undef MPIG_DATATYPE_CM_DECL_STRUCT_DEFINED
#define MPIG_DATATYPE_CM_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_DATATYPE_CM_OTHER_DECL)
#define MPIG_DATATYPE_CM_OTHER_DECL
#else
#undef MPIG_DATATYPE_CM_DECL_STRUCT_DEFINED
#define MPIG_DATATYPE_CM_DECL_STRUCT_DEFINED
#endif

#if defined(MPIG_DATATYPE_CM_DECL_STRUCT_DEFINED)
#define MPIG_DATATYPE_CM_DECL		\
    struct				\
    {					\
	MPIG_DATATYPE_CM_SELF_DECL	\
	MPIG_DATATYPE_CM_VMPI_DECL	\
	MPIG_DATATYPE_CM_XIO_DECL	\
	MPIG_DATATYPE_CM_OTHER_DECL	\
    } cm;
#else
#define MPIG_DATATYPE_CM_DECL
#endif

#define MPID_DEV_DATATYPE_DECL \
    MPIG_DATATYPE_CM_DECL

/* the inclusion of the datatype header file must come after the defintion of MPID_DEV_DATATYPE_DECL.  if it does not, then the
   device and communication module data structures will not be included in the MPID_Datatype object. */
#include "mpid_datatype.h"
/**********************************************************************************************************************************
						      END DATATYPE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						      BEGIN REQUEST SECTION
**********************************************************************************************************************************/
typedef enum mpig_request_type
{
    MPIG_REQUEST_TYPE_UNDEFINED = 0,
    MPIG_REQUEST_TYPE_INTERNAL,
    MPIG_REQUEST_TYPE_RECV,
    MPIG_REQUEST_TYPE_SEND,
    MPIG_REQUEST_TYPE_RSEND,
    MPIG_REQUEST_TYPE_SSEND,
    MPIG_REQUEST_TYPE_BSEND
}
mpig_request_type_t;

typedef void (*mpig_request_cm_destruct_fn_t)(struct MPID_Request * req);

typedef globus_mutex_t mpig_request_mutex_t;

#define MPIG_REQUEST_DEV_DECL													\
    struct mpig_request														\
    {																\
	/* mutex tp protect data and insure RC systems see updates */								\
	mpig_request_mutex_t mutex;												\
																\
	/* request type (combine with the top-level 'kind' field to determine exact nature of the request) */			\
	mpig_request_type_t type;												\
																\
	/* message envelope data (rank, tag, context id) */									\
	int rank;														\
	int tag;														\
	int ctx;														\
																\
	/* user buffer */													\
	void * buf;														\
	int cnt;														\
	MPI_Datatype dt;													\
																\
	/* pointer to datatype for reference counting purposes.  the datatype must be kept alive until the request is complete, \
	   even if the user were to free it. */											\
	struct MPID_Datatype * dtp;												\
																\
	/* handle of an associated remote request.  among other things, this information allows a remote cancel send handler to \
	   identify the correct request to remove from the unexpected queue.  this field is not used by the VMPI communication	\
	   module, but is required by the receive queue code, and thus is part of the general request structure. */		\
	int remote_req_id;													\
																\
	/* pointer to the communication module's request destruct function */							\
	mpig_request_cm_destruct_fn_t cm_destruct;										\
																\
	/* the virtual connection used to satisfy this request */								\
	mpig_vc_t * vc;														\
																\
	/* pointer allowing the request to be inserted into any number of lists/queues */					\
	struct MPID_Request * next;												\
    } dev;

/* FIXME: the defintion of MPIG_REQUEST_CM_DECL should be generated by configure based on a list of communication modules. */
#if !defined(MPIG_REQUEST_CM_SELF_DECL)
#define MPIG_REQUEST_CM_SELF_DECL
#else
#undef MPIG_REQUEST_CM_DECL_UNION_DEFINED
#define MPIG_REQUEST_CM_DECL_UNION_DEFINED
#endif
#if !defined(MPIG_REQUEST_CM_VMPI_DECL)
#define MPIG_REQUEST_CM_VMPI_DECL
#else
#undef MPIG_REQUEST_CM_DECL_UNION_DEFINED
#define MPIG_REQUEST_CM_DECL_UNION_DEFINED
#endif
#if !defined(MPIG_REQUEST_CM_XIO_DECL)
#define MPIG_REQUEST_CM_XIO_DECL
#else
#undef MPIG_REQUEST_CM_DECL_UNION_DEFINED
#define MPIG_REQUEST_CM_DECL_UNION_DEFINED
#endif
#if !defined(MPIG_REQUEST_CM_OTHER_DECL)
#define MPIG_REQUEST_CM_OTHER_DECL
#else
#undef MPIG_REQUEST_CM_DECL_UNION_DEFINED
#define MPIG_REQUEST_CM_DECL_UNION_DEFINED
#endif

#if defined(MPIG_REQUEST_CM_DECL_UNION_DEFINED)
#define MPIG_REQUEST_CM_DECL		\
    union				\
    {					\
	MPIG_REQUEST_CM_SELF_DECL	\
	MPIG_REQUEST_CM_VMPI_DECL	\
	MPIG_REQUEST_CM_XIO_DECL	\
	MPIG_REQUEST_CM_OTHER_DECL	\
    } cm;
#else
#define MPIG_REQUEST_CM_DECL
#endif

#define MPID_DEV_REQUEST_DECL	\
    MPIG_REQUEST_DEV_DECL	\
    MPIG_REQUEST_CM_DECL
/**********************************************************************************************************************************
						       END REQUEST SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN PROCESS DATA SECTION
**********************************************************************************************************************************/
/*
 * The value 128 is returned by the echomaxprocname target in src/mpid/globus/Makefile.sm.  If the value is modified here, it
 * also needs to be modified in Makefile.sm.
 */
#if !defined(MPIG_PROCESSOR_NAME_SIZE)
#define MPIG_PROCESSOR_NAME_SIZE 128
#endif

/* FIXME: the defintion of MPIG_PROCESS_CM_DECL should be generated by configure based on a list of communication modules. */
#if !defined(MPIG_PROCESS_CM_SELF_DECL)
#define MPIG_PROCESS_CM_SELF_DECL
#else
#undef MPIG_PROCESS_CM_DECL_STRUCT_DEFINED
#define MPIG_PROCESS_CM_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_PROCESS_CM_VMPI_DECL)
#define MPIG_PROCESS_CM_VMPI_DECL
#else
#undef MPIG_PROCESS_CM_DECL_STRUCT_DEFINED
#define MPIG_PROCESS_CM_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_PROCESS_CM_XIO_DECL)
#define MPIG_PROCESS_CM_XIO_DECL
#else
#undef MPIG_PROCESS_CM_DECL_STRUCT_DEFINED
#define MPIG_PROCESS_CM_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_PROCESS_CM_OTHER_DECL)
#define MPIG_PROCESS_CM_OTHER_DECL
#else
#undef MPIG_PROCESS_CM_DECL_STRUCT_DEFINED
#define MPIG_PROCESS_CM_DECL_STRUCT_DEFINED
#endif

#if defined(MPIG_PROCESS_CM_DECL_STRUCT_DEFINED)
#define MPIG_PROCESS_CM_DECL		\
    struct				\
    {					\
	MPIG_PROCESS_CM_SELF_DECL	\
	MPIG_PROCESS_CM_VMPI_DECL	\
	MPIG_PROCESS_CM_XIO_DECL	\
	MPIG_PROCESS_CM_OTHER_DECL	\
    } cm;
#else
#define MPIG_PROCESS_CM_DECL
#endif

typedef struct mpig_process
{
    /*
     * Pointer to the the process group to which this process belongs, the size of the process group, and the rank of the
     * processs within the process group.  Also he sizeof of the subjob to which this process belongs, and the rank of the
     * process within the subjob.
     *
     * NOTE: DONT NOT REORDER THESE FIELDS AS A STATIC INITIALIZER IN mpid_env.c SETS THEM BASED ON THEIR ORDER!
     */
    struct mpig_pg * my_pg;
    const char * my_pg_id;
    int my_pg_size;
    int my_pg_rank;

    int my_sj_size;
    int my_sj_rank;
    int my_sj_num;

    /* process id and hostname associated with the local process */
    char my_hostname[MPIG_PROCESSOR_NAME_SIZE];
    pid_t my_pid;

    /* mutex to protect and insure coherence of data in the process structure */
    globus_mutex_t mutex;

    MPIG_PROCESS_CM_DECL
}
mpig_process_t;

extern mpig_process_t mpig_process;
/**********************************************************************************************************************************
						    END PROCESS DATA SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						      BEGIN DATA CONVERSION
**********************************************************************************************************************************/
typedef enum mpig_endian
{
    MPIG_ENDIAN_UNKNOWN = 0,
    MPIG_ENDIAN_LITTLE,
    MPIG_ENDIAN_BIG
}
mpig_endian_t;
/**********************************************************************************************************************************
						       END DATA CONVERSION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						    BEGIN I/O VECTOR SECTION
**********************************************************************************************************************************/
typedef struct mpig_iov
{
    int max_entries;
    int free_entry;
    int cur_entry;
    size_t num_bytes;
    MPID_IOV iov[1];
}
mpig_iov_t;

#define MPIG_IOV_DECL(iov_var_name_, entries_)							\
    mpig_aligned_t iov_var_name_[(sizeof(mpig_iov_t) + ((entries_) - 1) * sizeof(MPID_IOV) +	\
	sizeof(mpig_aligned_t) - 1) / sizeof(mpig_aligned_t)]
/**********************************************************************************************************************************
						     END I/O VECTOR SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						    BEGIN DATA BUFFER SECTION
**********************************************************************************************************************************/
typedef struct mpig_databuf
{
    MPIU_Size_t size;
    MPIU_Size_t eod;
    MPIU_Size_t pos;
}
mpig_databuf_t;

#define MPIG_DATABUF_DECL(dbuf_var_name_, size_) \
    mpig_databuf_t dbuf_var_name_[(sizeof(mpig_databuf_t) + (size_) + sizeof(mpig_aligned_t) - 1) / sizeof(mpig_aligned_t)]
/**********************************************************************************************************************************
						     END DATA BUFFER SECTION
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
						BEGIN VIRTUAL CONNECTION SECTION
**********************************************************************************************************************************/
typedef int (*mpig_vc_adi3_send_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
					 int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_vc_adi3_isend_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
					  int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_vc_adi3_rsend_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
					  int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_vc_adi3_irsend_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
					   int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_vc_adi3_ssend_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
					  int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_vc_adi3_issend_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
					   int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_vc_adi3_recv_fn_t)(void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
					 int ctxoff, MPI_Status * status, struct MPID_Request ** reqp);

typedef int (*mpig_vc_adi3_irecv_fn_t)(void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
					  int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_vc_adi3_cancel_recv_fn_t)(struct MPID_Request * rreq);
	     
typedef int (*mpig_vc_adi3_cancel_send_fn_t)(struct MPID_Request * sreq);
	     
typedef void (*mpig_vc_recv_any_source_fn_t)(struct mpig_vc * vc, struct MPID_Request * rreq, struct MPID_Comm * comm,
						int * mpi_errno_p, bool_t * failed_p);

typedef void (*mpig_vc_inc_ref_count_fn_t)(struct mpig_vc * vc, bool_t * was_inuse, int * mpi_errno_p, bool_t * failed_p);

typedef void (*mpig_vc_dec_ref_count_fn_t)(struct mpig_vc * vc, bool_t * inuse, int * mpi_errno_p, bool_t * failed_p);

typedef void (*mpig_vc_destruct_fn_t)(struct mpig_vc * vc);

typedef double (*mpig_vc_vtable_last_entry_fn_t)(float foo, int bar, const short * baz, char bif);

typedef struct mpig_vc_vtable
{
    mpig_vc_adi3_send_fn_t		adi3_send;
    mpig_vc_adi3_isend_fn_t		adi3_isend;
    mpig_vc_adi3_rsend_fn_t		adi3_rsend;
    mpig_vc_adi3_irsend_fn_t		adi3_irsend;
    mpig_vc_adi3_ssend_fn_t		adi3_ssend;
    mpig_vc_adi3_issend_fn_t		adi3_issend;
    mpig_vc_adi3_recv_fn_t		adi3_recv;
    mpig_vc_adi3_irecv_fn_t		adi3_irecv;
    mpig_vc_adi3_cancel_recv_fn_t	adi3_cancel_recv;
    mpig_vc_adi3_cancel_send_fn_t	adi3_cancel_send;
    mpig_vc_recv_any_source_fn_t	recv_any_source;
    mpig_vc_inc_ref_count_fn_t		vc_inc_ref_count;
    mpig_vc_dec_ref_count_fn_t		vc_dec_ref_count;
    mpig_vc_destruct_fn_t		vc_destruct;
    mpig_vc_vtable_last_entry_fn_t	vtable_last_entry;
}
mpig_vc_vtable_t;

/*
 * create a union containing information specific to the configuration modules.
 *
 * FIXME: the defintion of MPIG_VC_CM_DECL should be generated by configure based on a list of communication modules.
 */
#if !defined(MPIG_VC_CM_SELF_DECL)
#define MPIG_VC_CM_SELF_DECL
#else
#undef MPIG_VC_CM_DECL_UNION_DEFINED
#define MPIG_VC_CM_DECL_UNION_DEFINED
#endif
#if !defined(MPIG_VC_CM_VMPI_DECL)
#define MPIG_VC_CM_VMPI_DECL
#else
#undef MPIG_VC_CM_DECL_UNION_DEFINED
#define MPIG_VC_CM_DECL_UNION_DEFINED
#endif
#if !defined(MPIG_VC_CM_XIO_DECL)
#define MPIG_VC_CM_XIO_DECL
#else
#undef MPIG_VC_CM_DECL_UNION_DEFINED
#define MPIG_VC_CM_DECL_UNION_DEFINED
#endif
#if !defined(MPIG_VC_CM_OTHER_DECL)
#define MPIG_VC_CM_OTHER_DECL
#else
#undef MPIG_VC_CM_DECL_UNION_DEFINED
#define MPIG_VC_CM_DECL_UNION_DEFINED
#endif

#if defined(MPIG_VC_CM_DECL_UNION_DEFINED)
#define MPIG_VC_CM_DECL		\
    union			\
    {				\
	MPIG_VC_CM_SELF_DECL	\
	MPIG_VC_CM_VMPI_DECL	\
	MPIG_VC_CM_XIO_DECL	\
	MPIG_VC_CM_OTHER_DECL	\
    } cm;
#else
#define MPIG_VC_CM_DECL
#endif

/* create structure containing contact information for each configuration module.  the information stored in here is primarily
 * used to determine if two connections can speak to each other at the desired topology level; however, depending on the
 * contents, it may also be used during connection formation.
 *
 * NOTE: information not needed for computing topology information should reside in the union above to conserve space.
 *
 * FIXME: the defintion of MPIG_VC_CI_DECL should be generated by configure based on a list of communication modules.
 */
#if !defined(MPIG_VC_CI_SELF_DECL)
#define MPIG_VC_CI_SELF_DECL
#endif
#if !defined(MPIG_VC_CI_VMPI_DECL)
#define MPIG_VC_CI_VMPI_DECL
#endif
#if !defined(MPIG_VC_CI_XIO_DECL)
#define MPIG_VC_CI_XIO_DECL
#endif
#if !defined(MPIG_VC_CI_OTHER_DECL)
#define MPIG_VC_CI_OTHER_DECL
#endif

#define MPIG_VC_CI_DECL										\
    struct											\
    {												\
	/* business card containing information on how to connect to the remote process */	\
	mpig_bc_t bc;										\
												\
	/* topology information */								\
	int topology_num_levels;								\
	unsigned topology_levels;								\
	char * lan_id;										\
	int app_num;										\
												\
	MPIG_VC_CI_SELF_DECL									\
	MPIG_VC_CI_VMPI_DECL									\
	MPIG_VC_CI_XIO_DECL									\
	MPIG_VC_CI_OTHER_DECL									\
    } ci;

typedef globus_mutex_t mpig_vc_mutex_t;

typedef struct mpig_vc
{
    /* mutex tp protect data and insure RC systems see updates */
    mpig_vc_mutex_t mutex;
    
    /* number of references to this VC object */
    int ref_count;

    /* process group to which the process associated with this VC belongs, and the rank of that process in the process group */
    struct mpig_pg * pg;
    const char * pg_id;
    int pg_rank;

    /* local process id */
    int lpid;

    /* communication module type, associated functions, and module specific data structures */
    mpig_cm_type_t cm_type;
    mpig_vc_vtable_t * vtable;
    MPIG_VC_CM_DECL
    MPIG_VC_CI_DECL

    /* map of MPI datatypes to basic C types */
    unsigned char dt_cmap[MPIG_DATATYPE_MAX_BASIC_TYPES];
}
mpig_vc_t;
/**********************************************************************************************************************************
						 END VIRTUAL CONNECTION SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					BEGIN VIRTUAL CONNECTION REFERENCE TABLE SECTION
**********************************************************************************************************************************/
/*
 * MPID_VCRT/MPID_VCR
 *
 * MPID_VCRT is the virtual connection reference table object.  MPID_VCR is an array of virtual connection references, allowing
 * the MPICH and device layers fast access to the items in the table.
 *
 * FIXME: These structures should not be exposed as fields in the MPICH layer of MPID_Comm object.  A better technique would be to
 * define an interface that allows the MPICH layer to access the information without having any exposure to to the data
 * structures themselves.  We should work with the MPICH folks to define such an interface.
 */
typedef struct mpig_vcrt * MPID_VCRT;
typedef struct mpig_vc * MPID_VCR;
/**********************************************************************************************************************************
					 END VIRTUAL CONNECTION REFERENCE TABLE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN PROCESS GROUP SECTION
**********************************************************************************************************************************/
typedef globus_mutex_t mpig_pg_mutex_t;

typedef struct mpig_pg
{
    /* mutex to protect data and insure RC systems see updates -- replace by a global PG mutex */
    /* mpig_pg_mutex_t mutex; */

    /* number of references to this PG object */
    int ref_count;

    /* committed state of PG object.  committed PG objects may be destroyed when the reference count reaches zero. */
    bool_t committed;
    
    /* Number of processes in the process group */
    int size;

    /* unique id for the process group id.  this is required for MPI-2 dynamic process functionality. */
    char * id;
    
    /* next pointer used to maintain a list of all process groups known to this process */
    struct mpig_pg * next;

    /* table of virtual connection objects.  at present this is an array of VC objects.  someday we may make this an array of VC
       references, or perhaps something more wacky like a distributed array to reduce memory consumption when scaling to a very
       large number of processors.  thus, it is important to use mpig_pg_get_vc() instead of directly referencing this field. */
    mpig_vc_t vct[1];
}
mpig_pg_t;
/**********************************************************************************************************************************
						    END PROCESS GROUP SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						  BEGIN PROGRESS ENGINE SECTION
**********************************************************************************************************************************/
typedef unsigned long mpig_pe_count_t;
/*
 * MPID_PROGRESS_STATE_DECL
 *
 * This state object is used to prevent MPID_Progress_wait() from erroneous blocking if progress has occurred between
 * MPID_Progress_start() and MPID_Progress_wait().  The state object is allocated on the stack, and initialized by
 * MPID_Progress_start().  MPID_Progress_end() is only called if MPID_Progress_wait() is not, so any cleanup of data structures
 * in the state object must occur in both routines.
 *
 * FIXME: the defintion of MPIG_PE_STATE_CM_DECL should be generated by configure based on a list of communication modules.
 */
#if !defined(MPIG_PE_STATE_CM_SELF_DECL)
#define MPIG_PE_STATE_CM_SELF_DECL
#else
#undef MPIG_PE_STATE_CM_DECL_STRUCT_DEFINED
#define MPIG_PE_STATE_CM_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_PE_STATE_CM_VMPI_DECL)
#define MPIG_PE_STATE_CM_VMPI_DECL
#else
#undef MPIG_PE_STATE_CM_DECL_STRUCT_DEFINED
#define MPIG_PE_STATE_CM_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_PE_STATE_CM_XIO_DECL)
#define MPIG_PE_STATE_CM_XIO_DECL
#else
#undef MPIG_PE_STATE_CM_DECL_STRUCT_DEFINED
#define MPIG_PE_STATE_CM_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_PE_STATE_CM_OTHER_DECL)
#define MPIG_PE_STATE_CM_OTHER_DECL
#else
#undef MPIG_PE_STATE_CM_DECL_STRUCT_DEFINED
#define MPIG_PE_STATE_CM_DECL_STRUCT_DEFINED
#endif

#if defined(MPIG_PE_STATE_CM_DECL_STRUCT_DEFINED)
#define MPIG_PE_STATE_CM_DECL		\
    struct mpig_pe_state_cm		\
    {					\
	MPIG_PE_STATE_CM_SELF_DECL	\
	MPIG_PE_STATE_CM_VMPI_DECL	\
	MPIG_PE_STATE_CM_XIO_DECL	\
	MPIG_PE_STATE_CM_OTHER_DECL	\
    } cm;
#else
#define MPIG_PE_STATE_CM_DECL
#endif

typedef struct mpig_pe_state
{
    /* snapshot of the progress engine completion counter */
    mpig_pe_count_t count;
}
mpig_pe_state_t;

#define MPID_PROGRESS_STATE_DECL	\
    mpig_pe_state_t dev;		\
    MPIG_PE_STATE_CM_DECL
/**********************************************************************************************************************************
						   END PROGRESS ENGINE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					       BEGIN DEVICE THREAD SUPPORT SECTION
**********************************************************************************************************************************/
#define MPID_REQUIRES_THREAD_SAFETY
typedef globus_mutex_t MPID_Thread_mutex_t;
/**********************************************************************************************************************************
						END DEVICE THREAD SUPPORT SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						 BEGIN DEBUGGING OUTPUT SECTION
**********************************************************************************************************************************/
#if defined(MPIG_DEBUG)

/* enable MPICH layer debugging as well */
#if !defined(USE_DBG_LOGGING)
#define USE_DBG_LOGGING
#endif

/* include the macros that set the enter/exit macros to call MPIR_ENTER/EXIT_FUNC */
#include "mpifunclog.h"

#elif defined(USE_DEBUG_LOGING)
/* if the MPICH layer debugging has been enabled, then enable the device layer debugging as well */
#define MPIG_DEBUG
#endif
/**********************************************************************************************************************************
						  END DEBUGGING OUTPUT SECTION
**********************************************************************************************************************************/

#endif /* MPICH2_MPIDPRE_H_INCLUDED */
