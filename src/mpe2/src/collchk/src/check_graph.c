/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int CollChk_check_graph(MPI_Comm comm, int nnodes, int *index, int* edges,
                        char* call)
{
    char err_str[256], check[20];
    int i, e;

    if(CollChk_same_int(comm, nnodes, call, "Nnodes", err_str) != MPI_SUCCESS) {
        return CollChk_err_han(err_str, COLLCHK_ERR_GRAPH, call, comm);
    }

    for(i=0; i<nnodes; i++) {
        sprintf(check, "Index Sub %d", i);

        if(CollChk_same_int(comm, index[i], call, check, err_str) != MPI_SUCCESS) {
            return CollChk_err_han(err_str, COLLCHK_ERR_GRAPH, call, comm);
        }
        
        e = index[i];
    }

    for(i=0; i<e; i++) {
        sprintf(check, "Edges Sub %d", i);
        
        if(    CollChk_same_int(comm, edges[i], call, check, err_str)
            != MPI_SUCCESS) {
            return CollChk_err_han(err_str, COLLCHK_ERR_GRAPH, call, comm);
        }

        e = index[i];
    }

    return MPI_SUCCESS;
}
