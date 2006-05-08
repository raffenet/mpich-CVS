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
 * add communication module types to be included in the enumeration of modules
 */
#define MPIG_CM_TYPE_SELF_LIST	\
    MPIG_CM_TYPE_SELF


/*
 * global function prototypes
 */
int mpig_cm_self_init(int * argc, char *** argv);

int mpig_cm_self_finalize(void);

int mpig_cm_self_add_contact_info(struct mpig_bc * bc);

int mpig_cm_self_select_module(struct mpig_bc * bc, struct mpig_vc * vc, bool_t * selected);

#endif /* !defined(MPICH2_MPIG_CM_SELF_H_INCLUDED) */
