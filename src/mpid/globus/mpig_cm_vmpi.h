/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIG_CM_VMPI_H_INCLUDED)
#define MPICH2_MPIG_CM_VMPI_H_INCLUDED 1
#if defined(MPIG_VMPI)

#include "mpig_vmpi.h"


/* change any occurences of MPID and MPID_ to MPIG and MPIG_ respectively.  this symbols are found in macros that construct
   symbol names.  One example of such macros are those that convert object handles to object pointers in
   src/include/mpiimpl.h. */
#define MPID				MPIG
#define MPID_				MPIG_

/* because the MPIR_Nest routines are defined as both CPP macros and functions, their renaming has to be handled as a special
   case. */
#define MPIR_NEST_INCR_FCNAME		MPIR_Nest_incr_MPIG
#define MPIR_NEST_DECR_FCNAME		MPIR_Nest_decr_MPIG
#define MPIR_NEST_VALUE_FCNAME		MPIR_Nest_value_MPIG


/*
 * add communication module types to be included in the enumeration of modules
 */
#define MPIG_CM_TYPE_VMPI_LIST \
    MPIG_CM_TYPE_VMPI


/*
 * define the communication module structure to be included in a communicator object
 */
#define MPIG_COMM_CM_VMPI_DECL				\
    struct						\
    {							\
	mpig_vmpi_comm_t comms[MPIG_COMM_NUM_CONTEXTS];	\
	int * remote_ranks_mtov;			\
	int * remote_ranks_vtom;			\
    } vmpi;


/*
 * define the communication module structure to be included in a datatype object
 */
#define MPIG_DATATYPE_CM_VMPI_DECL	\
    struct				\
    {					\
	mpig_vmpi_datatype_t dt;	\
    } vmpi;


/*
 * define the communication module structure to be included in a request object
 */
#define MPIG_REQUEST_CM_VMPI_DECL	\
    struct				\
    {					\
	mpig_vmpi_request_t req;	\
    } vmpi;


/*
 * define the communication module structure to be included in the mpig process structure
 */
#define MPIG_PROCESS_CM_VMPI_DECL	\
    struct				\
    {					\
	int cw_rank;			\
	int cw_size;			\
    } vmpi;


/*
 * define the communication module structure to be included in a VC
 */
#define MPIG_VC_CM_VMPI_DECL	\
    struct			\
    {				\
	int cw_rank;		\
    } vmpi;



/*
 * global macro definitions
 */
#if defined(HAVE_ERROR_CHECKING) || defined(MPIG_DEBUG)
#define MPIG_ERR_VMPI_SET(vrc_, vfcname_, mpi_errno_p_)									\
{															\
    int error_class__;													\
    char error_str__[MPIG_VMPI_MAX_ERROR_STRING + 1];									\
    int error_str_len__;												\
    															\
    mpig_vmpi_error_class((vrc_), &error_class__);									\
    mpig_vmpi_error_string((vrc_), error_str__, &error_str_len__);							\
    MPIU_ERR_SET2(*(mpi_errno_p_), mpig_cm_vmpi_error_class_vtom(error_class__), "**globus|vmpi_fn_failed",		\
	"**globus|vmpi_fn_failed %s %s", (vfcname_), error_str__);							\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR, "ERROR: call to vendor %s failed: %s", (vfcname_), (error_str__)));	\
}
#else
#define MPIG_ERR_VMPI_SET(vrc_, vfcname_, mpi_errno_p_)			\
{									\
    int error_class__;							\
    mpig_vmpi_error_class((vrc_), &error_class__);			\
    *(mpi_errno_p_) = mpig_cm_vmpi_error_class_vtom(error_class__);	\
}
#endif

#define MPIG_ERR_VMPI_SETANDSTMT(vrc_, vfcname_, stmt_, mpi_errno_p_)	\
{									\
    MPIG_ERR_VMPI_SET((vrc_), (vfcname_), (mpi_errno_p_));		\
    { stmt_ ; }								\
}

#define MPIG_ERR_VMPI_CHKANDSTMT(vrc_, vfcname_, stmt_, mpi_errno_p_)	\
{									\
    if (vrc_)								\
    {									\
	MPIG_ERR_VMPI_SET((vrc_), (vfcname_), (mpi_errno_p_));		\
	{ stmt_ ; }							\
    }									\
}

#define MPIG_ERR_VMPI_CHKANDJUMP(vrc_, vfcname_, mpi_errno_p_)				\
{											\
    MPIG_ERR_VMPI_CHKANDSTMT((vrc_), (vfcname_), {goto fn_fail;}, (mpi_errno_p_));	\
}

#endif /* defined(MPIG_VMPI) */


/*
 * global function prototypes
 */
int mpig_cm_vmpi_init(int * argc, char *** argv);

int mpig_cm_vmpi_finalize(void);

int mpig_cm_vmpi_add_contact_info(struct mpig_bc * bc);

int mpig_cm_vmpi_select_module(struct mpig_bc * bc, struct mpig_vc * vc, int * flag);

#if defined(MPIG_VMPI)

void mpig_cm_vmpi_pe_start(struct MPID_Progress_state * state);

void mpig_cm_vmpi_pe_end(struct MPID_Progress_state * state);

void mpig_cm_vmpi_pe_wait(struct MPID_Progress_state * state, int * mpi_errno_p, bool_t * failed_p);

void mpig_cm_vmpi_pe_test(int * mpi_errno_p, bool_t * failed_p);

void mpig_cm_vmpi_pe_poke(int * mpi_errno_p, bool_t * failed_p);


void mpig_cm_vmpi_dev_comm_dup_hook(struct MPID_Comm * orig_comm, struct MPID_Comm * new_comm, int * mpi_errno_p);

void mpig_cm_vmpi_dev_intercomm_create_hook(struct MPID_Comm * local_comm, int local_leader, struct MPID_Comm * peer_comm,
    int remote_leader, int tag, struct MPID_Comm * new_intercomm, int * mpi_errno_p);

void mpig_cm_vmpi_dev_comm_free_hook(struct MPID_Comm * comm, int * mpi_errno_p);


int mpig_cm_vmpi_adi3_iprobe(int src, int tag, struct MPID_Comm * comm, int ctxoff, int * flag, MPI_Status * status);

int mpig_cm_vmpi_error_class_vtom(int vendor_error_class);

#endif /* defined(MPIG_VMPI) */


/*
 * macro implementations of CM interface functions
 */
#if defined(MPIG_VMPI)

#define mpig_cm_vmpi_pe_start(state_)

#define mpig_cm_vmpi_pe_end(state_)

#define mpig_cm_vmpi_pe_poke(mpi_errno_p_, failed_p_) mpig_cm_vmpi_progess_test((mpi_errno_p_), (failed_p_))

#else /* !defined(MPIG_VMPI) */

#define mpig_cm_vmpi_pe_start(state_)

#define mpig_cm_vmpi_pe_end(state_)

#define mpig_cm_vmpi_pe_wait(state_, mpi_errno_p_, failed_p_)	\
{								\
    *(failed_p_) = FALSE;					\
}

#define mpig_cm_vmpi_pe_test(mpi_errno_p_, failed_p_)	\
{							\
    *(failed_p_) = FALSE;				\
}

#define mpig_cm_vmpi_pe_poke(mpi_errno_p_, failed_p_)	\
{							\
    *(failed_p_) = FALSE;				\
}

#endif /* else !defined(MPIG_VMPI) */

#endif /* !defined(MPICH2_MPIG_CM_VMPI_H_INCLUDED) */
