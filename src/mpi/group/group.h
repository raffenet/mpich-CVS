/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

int MPIR_Group_create( int, MPID_Group ** );
void MPIR_Group_setup_lpid_list( MPID_Group * );
int MPIR_Group_check_valid_ranks( MPID_Group *, int [], int );
int MPIR_Group_check_valid_ranges( MPID_Group *, int [][3], int );
void MPIR_Group_setup_lpid_pairs( MPID_Group *, MPID_Group * );

