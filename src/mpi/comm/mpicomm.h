/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
extern MPID_Comm *MPIU_Comm_create( void );
extern void MPIU_Comm_free( MPID_Comm *comm_ptr );
extern void MPIU_Comm_destroy( MPID_Comm *comm_ptr );
