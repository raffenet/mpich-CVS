/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIG_CM_VMPI_H_INCLUDED)
#define MPICH2_MPIG_CM_VMPI_H_INCLUDED 1

/*
 * expose the communication module's vtable so that it is accessible to other modules in the device
 */
extern struct mpig_cm mpig_cm_vmpi;


/*
 * define the structure to be included in the communication method structure (CMS) of a VC object
 */
#define MPIG_VC_CMS_VMPI_DECL	\
struct				\
{				\
    char * subjob_id;		\
    int cw_rank;		\
} vmpi;


#if defined(MPIG_VMPI)
#include "mpig_vmpi.h"

/* change any occurences of MPID and MPID_ to MPIG and MPIG_ respectively.  this symbols are found in macros that construct
   symbol names.  One example of such macros are those that convert object handles to object pointers in
   src/include/mpiimpl.h. */
#define MPID				MPIG
#define MPID_				MPIG_


/*
 * define the structure to be included in the communcation method structure (CMS) of a communicator object
 */
#define MPIG_COMM_CMS_VMPI_DECL				\
    struct						\
    {							\
	mpig_vmpi_comm_t comms[MPIG_COMM_NUM_CONTEXTS];	\
	int * remote_ranks_mtov;			\
	int * remote_ranks_vtom;			\
    } vmpi;


/*
 * define the structure to be included in the communication method structure (CMS) of a datatype object
 */
#define MPIG_DATATYPE_CMS_VMPI_DECL	\
    struct				\
    {					\
	mpig_vmpi_datatype_t dt;	\
    } vmpi;


/*
 * define the structure to be included in the communication method union (CMU) of a request object
 */
#define MPIG_REQUEST_CMU_VMPI_DECL	\
    struct				\
    {					\
	mpig_vmpi_request_t req;	\
    } vmpi;


/*
 * define the structure to be included in the communication method structure (CMS) of the MPIG process structure
 */
#define MPIG_PROCESS_CMS_VMPI_DECL	\
    struct				\
    {					\
	int cw_rank;			\
	int cw_size;			\
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
#if defined(MPIG_VMPI)

void mpig_cm_vmpi_pe_start(struct MPID_Progress_state * state);

void mpig_cm_vmpi_pe_end(struct MPID_Progress_state * state);

int mpig_cm_vmpi_pe_wait(struct MPID_Progress_state * state);

int mpig_cm_vmpi_pe_test(void);

int mpig_cm_vmpi_pe_poke(void);


int mpig_cm_vmpi_dev_comm_dup_hook(struct MPID_Comm * orig_comm, struct MPID_Comm * new_comm);

int mpig_cm_vmpi_dev_intercomm_create_hook(struct MPID_Comm * local_comm, int local_leader, struct MPID_Comm * peer_comm,
    int remote_leader, int tag, struct MPID_Comm * new_intercomm);

int mpig_cm_vmpi_dev_comm_free_hook(struct MPID_Comm * comm);


int mpig_cm_vmpi_adi3_iprobe(int src, int tag, struct MPID_Comm * comm, int ctxoff, int * flag, MPI_Status * status);

int mpig_cm_vmpi_error_class_vtom(int vendor_error_class);

#endif /* defined(MPIG_VMPI) */


/*
 * macro implementations of CM interface functions
 */
#if defined(MPIG_VMPI)

#define mpig_cm_vmpi_pe_start(state_)

#define mpig_cm_vmpi_pe_end(state_)

#define mpig_cm_vmpi_pe_poke() mpig_cm_vmpi_progess_test()

#else /* !defined(MPIG_VMPI) */

#define mpig_cm_vmpi_pe_start(state_)

#define mpig_cm_vmpi_pe_end(state_)

#define mpig_cm_vmpi_pe_wait(state_) (MPI_SUCCESS)

#define mpig_cm_vmpi_pe_test() (MPI_SUCCESS)

#define mpig_cm_vmpi_pe_poke() (MPI_SUCCESS)

#endif /* else !defined(MPIG_VMPI) */

#endif /* !defined(MPICH2_MPIG_CM_VMPI_H_INCLUDED) */
