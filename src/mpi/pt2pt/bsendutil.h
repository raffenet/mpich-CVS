/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* Function Prototypes for the bsend utility functions */
int MPIR_Bsend_attach( void *, int );
int MPIR_Bsend_detach( void *, int * );
int MPIR_Bsend_isend( void *, int, MPI_Datatype, int, int, MPID_Comm *, 
		      MPI_Request * );

