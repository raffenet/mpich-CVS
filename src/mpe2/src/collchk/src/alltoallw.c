/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 


int MPI_Alltoallw(void* sbuff, int *scnt, int *sdispls, MPI_Datatype *stype,
                  void* rbuff, int *rcnt, int *rdispls, MPI_Datatype *rtype,
                  MPI_Comm comm)
{
    int g2g = 1, r;
    char call[25];
    CollChk_hash_struct hs1, hs2;

    sprintf(call, "ALLTOALLW");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        MPI_Comm_rank(comm, &r);

        /* check call consistency */
        CollChk_same_call(comm, call);

        /* check data signature consistancy */
        CollChk_same_dtype_general(comm, rcnt, scnt, rtype, stype, call);
        CollChk_hash_dtype(rtype[r], rcnt[r],
                           &(hs1.hash_val), &(hs1.hash_cnt));
        CollChk_hash_dtype(stype[r], scnt[r],
                           &(hs2.hash_val), &(hs2.hash_cnt));
        if (    (hs1.hash_val != hs2.hash_val)
             || (hs1.hash_cnt != hs2.hash_cnt))  {
            CollChk_err_han("Sending and Receiving Datatype Signatures "
                            "do not match", 
                            COLLCHK_ERR_DTYPE, call, comm);
        }

        /* make the call */
        return PMPI_Alltoallw(sbuff, scnt, sdispls, stype,
                              rbuff, rcnt, rdispls, rtype, comm);
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, comm);
    }
}

