/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

int MPI_File_write_all_begin(MPI_File fh, void *buff, int cnt,
                             MPI_Datatype dtype)
{
    int g2g = 1;
    char call[25];
    char err_str[255];
    MPI_Comm comm;

    sprintf(call, "FILE_WRITE_ALL_BEGIN");

    /* Check if init has been called */
    g2g = CollChk_is_init();

    if(g2g) {
        /* get the communicator */
        if (CollChk_get_fh(fh, &comm)) {
            return CollChk_err_han("File has not been opened",
                                   COLLCHK_ERR_FILE_NOT_OPEN,
                                   call, MPI_COMM_WORLD);
        }

        /* check for call consistancy */
        CollChk_same_call(comm, call);

        /* check for previous begin */
        if(COLLCHK_CALLED_BEGIN) {
            sprintf(err_str, "Previous MPI_File_%s_begin called, "
                             "must call MPI_File_%s_end first.",
                             CollChk_begin_str, CollChk_begin_str);
            return CollChk_err_han(err_str, COLLCHK_ERR_PREVIOUS_BEGIN,
                                   call, comm);
        }
        else {
            CollChk_set_begin("WRITE_ALL");
        }    

        /* make the call */
        return PMPI_File_write_all_begin(fh, buff, cnt, dtype);
    }
    else {
        /* init not called */
        return CollChk_err_han("MPI_Init() has not been called!",
                               COLLCHK_ERR_NOT_INIT, call, MPI_COMM_WORLD);
    }
}
