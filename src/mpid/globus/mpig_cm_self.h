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
 * expose the communication module's vtable so that it is accessible to other modules in the device
 */
extern const mpig_cm_vtable_t mpig_cm_self_vtable;


/*
 * define the connect information structure to be included in a VC
 */
#define MPIG_VC_CI_SELF_DECL						\
struct mpig_ci_self_vc							\
{									\
    /* name of hostname running the process */				\
    char * hostname;							\
									\
    /* process id of the process */					\
    unsigned long pid;							\
}									\
self;


/*
 * global function prototypes
 */
#endif /* !defined(MPICH2_MPIG_CM_SELF_H_INCLUDED) */
