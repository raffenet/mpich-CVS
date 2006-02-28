/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIG_CM_VMPI_H)
#define MPICH2_MPIG_CM_VMPI_H 1

#if !defined(MPIG_VMPI)

/*
 * Add communication module types to be included in the enumeration of modules
 */
#define MPIG_CM_TYPE_VMPI_LIST \
    MPIG_CM_TYPE_VMPI

/*
 * Define the communication module structure to be included in a VC
 */
#define MPIG_VC_CM_VMPI_DECL

/*
 * Define the communication module structure to be included in a request
 */
#define MPIG_REQUEST_CM_VMPI_DECL

#else

... XXX ...

#endif /* defined(MPIG_VMPI) */

/*
 * Global funciton prototypes
 */
int mpig_cm_vmpi_init(int * argc, char *** argv);

int mpig_cm_vmpi_finalize(void);

int mpig_cm_vmpi_add_contact_info(struct mpig_bc * bc);

int mpig_cm_vmpi_select_module(struct mpig_bc * bc, struct mpig_vc * vc, int * flag);

#endif /* !defined(MPICH2_MPIG_CM_VMPI_H) */
