/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_Scatterv(void* sbuff, int* scnt, int* displs, MPI_Datatype stype,
                 void* rbuff, int rcnt, MPI_Datatype rtype,
                 int root, MPI_Comm comm)
{
    int g2g = 1, r;
    char call[25];
    CollChk_hash_struct hs1, hs2;

    sprintf(call, "SCATTERV");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        MPI_Comm_rank(comm, &r);

        /* check for call consistancy */
        CollChk_same_call(comm, call);
        /* check for same root     */
        CollChk_same_root(comm, root, call);

        /* check for matching data sizes */
        if(r == root) {
            CollChk_same_dtype_vector(comm, root, rcnt, scnt, rtype, call);
            if (rbuff != MPI_IN_PLACE) {
                CollChk_hash_dtype(rtype, rcnt,
                                   &(hs1.hash_val), &(hs1.hash_cnt));
                CollChk_hash_dtype(stype, scnt,
                                   &(hs2.hash_val), &(hs2.hash_cnt));
                if (    (hs1.hash_val != hs2.hash_val)
                     || (hs1.hash_cnt != hs2.hash_cnt) ) {
                    CollChk_err_han("Sending and Receiving "
                                    "Datatype Signatures do not match", 
                                    COLLCHK_ERR_DTYPE, call, comm);
                }
            }
        }
        else {
            CollChk_same_dtype_vector(comm, root, rcnt, scnt, stype, call);
        }

        /* make the call */
        return PMPI_Scatterv(sbuff, scnt, displs, stype,
                             rbuff, rcnt, rtype, root, comm);
    }
    else {
        /* init has not been called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, comm);
    }
}
