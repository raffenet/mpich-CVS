/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This file contains the definitions and prototypes exported by the 
 * pmi server package
 */

/*
 * This struct is maintained within the process structure managed by 
 * mpiexec.  This allows the PMI server implementation to be separate from
 * any particular mpiexec implementation.  
 * 
 * We use indices instead of pointers for the group and the kvs space.
 */
#define MAXKVSNAME  256         /* max length of a kvsname */
typedef struct PMI_Process {
  int  fd;          /* Fd for PMI communications */
  int  group;       /* Index to the PMI group */
  int  kvs;         /* Index to the KVS space */
} PMI_Process;
/* Note that the fd may be duplicated within the process structure managed by 
   the mpiexec implementation to simplify setting up the select/poll loops */

/* This routine is called when there is input on the fd for the given 
   process */
extern int PMIServ_handle_input_fd ( PMI_Process * );
