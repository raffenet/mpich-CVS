/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Function prototypes for communicator helper functions */
int MPIR_Get_contextid( MPI_Comm );
int MPIR_Get_intercomm_contextid( MPID_Comm * );
void MPIR_Free_contextid( int );

