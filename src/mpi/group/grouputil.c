/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#ifndef MPID_GROUP_PREALLOC 
#define MPID_GROUP_PREALLOC 8
#endif

/* Preallocated group objects */
MPID_Group MPID_Group_builtin[MPID_GROUP_N_BUILTIN];
MPID_Group MPID_Group_direct[MPID_GROUP_PREALLOC];
MPIU_Object_alloc_t MPID_Group_mem = { 0, 0, 0, 0, MPID_GROUP, 
				      sizeof(MPID_Group), MPID_Group_direct,
				       MPID_GROUP_PREALLOC};
