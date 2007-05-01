/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *
 *   Copyright (C) 2004 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "adio.h"
#include "mpiu_greq.h"

/* Generic implementation of IreadContig calls the blocking ReadContig
 * immediately.
 */
void ADIOI_FAKE_IreadContig(ADIO_File fd, void *buf, int count, 
			   MPI_Datatype datatype, int file_ptr_type,
			   ADIO_Offset offset, ADIO_Request *request,
			   int *error_code)  
{
    ADIO_Status status;
    int len, typesize;

    MPI_Type_size(datatype, &typesize);
    len = count * typesize;

    /* Call the blocking function.  It will create an error code
     * if necessary.
     */
    ADIO_ReadContig(fd, buf, len, MPI_BYTE, file_ptr_type, offset, 
		    &status, error_code);  
    MPIO_Completed_request_create(fd, error_code, request);

#ifdef HAVE_STATUS_SET_BYTES
    if (*error_code == MPI_SUCCESS) {
	MPI_Get_elements(&status, MPI_BYTE, &len);
	/* need to do something with len */
    }
#endif
}


/* Generic implementation of IreadStrided calls the blocking ReadStrided
 * immediately.
 */
void ADIOI_FAKE_IreadStrided(ADIO_File fd, void *buf, int count, 
			    MPI_Datatype datatype, int file_ptr_type,
			    ADIO_Offset offset, ADIO_Request *request,
			    int *error_code)
{
    ADIO_Status status;
#ifdef HAVE_STATUS_SET_BYTES
    int typesize;
#endif

    /* Call the blocking function.  It will create an error code
     * if necessary.
     */
    ADIO_ReadStrided(fd, buf, count, datatype, file_ptr_type, 
		     offset, &status, error_code);  
    MPIO_Completed_request_create(fd, error_code, request);

#ifdef HAVE_STATUS_SET_BYTES
    if (*error_code == MPI_SUCCESS) {
	MPI_Type_size(datatype, &typesize);
	/* do something with count * typesize */
    }
#endif
}
