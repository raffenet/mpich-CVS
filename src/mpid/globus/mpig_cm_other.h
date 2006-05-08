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
 * special case VC for handling things like MPI_ANY_SOURCE and MPI_PROC_NULL
 */
extern struct mpig_vc * mpig_cm_other_vc;


/*
 * add communication module types to be included in the enumeration of modules
 */
#define MPIG_CM_TYPE_OTHER_LIST	\
    MPIG_CM_TYPE_OTHER


/*
 * global function prototypes
 */
int mpig_cm_other_init(int * argc, char *** argv);

int mpig_cm_other_finalize(void);

int mpig_cm_other_add_contact_info(struct mpig_bc * bc);

int mpig_cm_other_select_module(struct mpig_bc * bc, struct mpig_vc * vc, bool_t * selected);

#endif /* !defined(MPICH2_MPIG_CM_OTHER_H_INCLUDED) */
