/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Alltoall( void* sbuff, int scnt, MPI_Datatype stype,
                  void* rbuff, int rcnt, MPI_Datatype rtype,
                  MPI_Comm comm)
{
    int g2g = 1;
    char call[25];
    CollChk_hash_struct hs1, hs2;

    sprintf( call, "ALLTOALL" );

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if( g2g ) {
        /* check call consistancy */
        CollChk_same_call( comm, call );
        
        /* check data signature consistancy */
        CollChk_same_dtype( comm, rcnt, rtype, call );
        CollChk_hash_dtype( rtype, rcnt, &(hs1.hash_val), &(hs1.hash_cnt) );
        CollChk_hash_dtype( stype, scnt, &(hs2.hash_val), &(hs2.hash_cnt) );
        if (    (hs1.hash_val != hs2.hash_val)
             || (hs1.hash_cnt != hs2.hash_cnt))  {
            CollChk_err_han( "Sending and Receiving Datatype Signatures "
                             "do not match",
                             COLLCHK_ERR_DTYPE, call, comm);
        }

        /* make the call */
        return (PMPI_Alltoall(sbuff, scnt, stype, rbuff, rcnt, rtype, comm));
    }
    else {
        /* init not called */
        return CollChk_err_han( "MPI_Init() has not been called!",
                                COLLCHK_ERR_NOT_INIT, call, comm );
    }
}

