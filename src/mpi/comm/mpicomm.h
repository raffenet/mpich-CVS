/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * Function prototypes for communicator helper functions
 *
 * The MPIR_Get_contextid and MPIR_Free_conntextid routines are declared in
 * mpiimpl.h so that the device may use them.
 *
 * int MPIR_Get_contextid( MPID_Comm * );
 * void MPIR_Free_contextid( int );
 */
int MPIR_Get_intercomm_contextid( MPID_Comm *, int *, int * );
