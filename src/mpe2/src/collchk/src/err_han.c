/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int CollChk_err_han(char * err_str, int err_code, char * call, MPI_Comm comm)
{
    int r, i;
    char err[255];

    if(err_code == COLLCHK_ERR_NOT_INIT) {
        printf("VALIDATE %s --> %s\n", call, err_str); fflush(stdout);
    }
    else if (strcmp(err_str, "no error") != 0) {    
        MPI_Comm_rank(comm, &r);
        sprintf(err, "\n\nVALIDATE %s (Rank %d) --> %s\n\n", call, r, err_str);
        fflush(stdout);
        MPI_Add_error_string(err_code, err);
    }
    else {
        MPI_Add_error_string(err_code, "Error on another process");
        sleep(1);
    }

    return MPI_Comm_call_errhandler(comm, err_code);
}
