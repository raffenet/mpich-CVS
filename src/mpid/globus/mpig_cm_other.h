/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIG_CM_OTHER_H_INCLUDED)
#define MPICH2_MPIG_CM_OTHER_H_INCLUDED

/*
 * Special case VC for handling things like MPI_ANY_SOURCE and MPI_PROC_NULL
 */
extern struct mpig_vc * mpig_cm_other_vc;

/*
 * Add communication module types to be included in the enumeration of modules
 */
#define MPIG_CM_TYPE_OTHER_LIST			\
    ,MPIG_CM_TYPE_OTHER

/*
 * Define the communication module structure to be included in a VC
 */
#define MPIG_VC_CM_OTHER_DECL			\
struct mpig_vc_vm_other				\
{						\
    int dummy;					\
}						\
other;

/*
 * Define the communication module structure to be included in a request
 */
#define MPIG_REQUEST_CM_OTHER_DECL		\
struct mpig_request_cm_other			\
{						\
    int dummy;					\
}						\
other;


/*
 * Global funciton prototypes
 */
int mpig_cm_other_init(int * argc, char *** argv);

int mpig_cm_other_finalize(void);

int mpig_cm_other_add_contact_info(struct mpig_bc * bc);

int mpig_cm_other_select_module(struct mpig_bc * bc, struct mpig_vc * vc, int * flag);

#endif /* !defined(MPICH2_MPIG_CM_OTHER_H_INCLUDED) */
