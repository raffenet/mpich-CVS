/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Allgatherv( void* sbuff, int scnt, MPI_Datatype stype,
                    void* rbuff, int *rcnt, int *displs, MPI_Datatype rtype,
                    MPI_Comm comm )
{
    int g2g = 1, r;
    char call[25];
    CollChk_hash_struct hs1, hs2;

    sprintf( call, "ALLGATHERV" );

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if( g2g ) {
        MPI_Comm_rank(comm, &r);

        /* check for call consistency */
        CollChk_same_call( comm, call );
        /* check MPI_IN_PLACE consistency */
        CollChk_check_buff( comm, sbuff, call );

        /* check data signature consistancy */
        CollChk_same_dtype_vector2( comm, rcnt, rtype, call );
        if ( sbuff != MPI_IN_PLACE ) {
            CollChk_hash_dtype( rtype, rcnt[r],
                                &(hs1.hash_val), &(hs1.hash_cnt));
            CollChk_hash_dtype( stype, scnt,
                                &(hs2.hash_val), &(hs2.hash_cnt));
            if (    (hs1.hash_val != hs2.hash_val)
                 || (hs1.hash_cnt != hs2.hash_cnt)) {
                CollChk_err_han( "Sending and Receiving Datatype Signatures "
                                 "do not match", 
                                 COLLCHK_ERR_DTYPE, call, comm );
            }
        }

        /* make the call */
        return PMPI_Allgatherv( sbuff, scnt, stype, rbuff, rcnt, displs, rtype,
                                comm );
    }
    else {
        /* init not called */
        return CollChk_err_han( "MPI_Init() has not been called!",
                                COLLCHK_ERR_NOT_INIT, call, comm );
    }
}
