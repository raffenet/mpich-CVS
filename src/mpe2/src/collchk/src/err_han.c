/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int CollChk_err_han(char *err_str, int err_code, char *call, MPI_Comm comm)
{
    int   rank;
    char  msg[COLLCHK_STD_STRLEN];

    if(err_code == COLLCHK_ERR_NOT_INIT) {
        printf("Collective Checking: %s --> %s\n", call, err_str);
        fflush(stdout); fflush(stderr);
    }
    else if (strcmp(err_str, COLLCHK_NO_ERROR_STR) != 0) {    
        MPI_Comm_rank(comm, &rank);
        sprintf(msg, "\n\nCollective Checking: %s (Rank %d) --> %s\n\n",
                     call, rank, err_str);
        fflush(stdout); fflush(stderr);
        MPI_Add_error_string(err_code, msg);
    }
    else {
        MPI_Add_error_string(err_code, "Error on another process");
        sleep(1);
    }

    return MPI_Comm_call_errhandler(comm, err_code);
}
