/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
  Keyval and attribute storage
 */
extern MPIU_Object_alloc_t MPID_Keyval_mem;
extern MPIU_Object_alloc_t MPID_Attr_mem;
extern MPID_Keyval MPID_Keyval_direct[];

extern int MPIR_Comm_attr_dup_list( MPID_Comm *, MPID_Attribute ** );
extern int MPIR_Comm_attr_delete_list( MPID_Comm *, MPID_Attribute * );
extern void MPID_Keyval_free(MPID_Keyval *keyval_ptr);
extern void MPID_Attr_free(MPID_Attribute *attr_ptr);
extern int MPIR_Comm_call_attr_delete( MPI_Comm, MPID_Attribute * );

