/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIG_CM_SELF_H_INCLUDED)
#define MPICH2_MPIG_CM_SELF_H_INCLUDED

/*
 * Add communication module types to be included in the enumeration of modules
 */
#define MPIG_CM_TYPE_SELF_LIST			\
    ,MPIG_CM_TYPE_SELF

/*
 * Define the communication module structure to be included in a VC
 */
#define MPIG_VC_CM_SELF_DECL			\
struct mpig_vc_vm_self				\
{						\
    int dummy;					\
}						\
self;

/*
 * Define the communication module structure to be included in a request
 */
#define MPIG_REQUEST_CM_SELF_DECL		\
struct mpig_request_cm_self			\
{						\
    int dummy;					\
}						\
self;


/*
 * Global funciton prototypes
 */
int mpig_cm_self_init(int * argc, char *** argv);

int mpig_cm_self_finalize(void);

int mpig_cm_self_add_contact_info(struct mpig_bc * bc);

int mpig_cm_self_select_module(struct mpig_bc * bc, struct mpig_vc * vc, int * flag);

#endif /* !defined(MPICH2_MPIG_CM_SELF_H_INCLUDED) */
