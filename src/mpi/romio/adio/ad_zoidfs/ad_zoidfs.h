#ifndef AD_ZOIDFS_INCLUDE
#define AD_ZOIDFS_INCLUDE

#include "adio.h"
#include "zoidfs.h"

struct ADIOI_ZOIDFS_fs_s {
  zoidfs_handle_t fhandle;
} ADIOI_ZOIDFS_fs_s;

typedef struct ADIOI_ZOIDFS_fs_s ADIOI_ZOIDFS_fs;

void ADIOI_ZOIDFS_Open(ADIO_File fd, int *error_code);
void ADIOI_ZOIDFS_Close(ADIO_File fd, int *error_code);
void ADIOI_ZOIDFS_ReadContig(ADIO_File fd, void *buf, int count, 
                      MPI_Datatype datatype, int file_ptr_type,
                     ADIO_Offset offset, ADIO_Status *status, int
		     *error_code);
void ADIOI_ZOIDFS_WriteContig(ADIO_File fd, void *buf, int count, 
                      MPI_Datatype datatype, int file_ptr_type,
                      ADIO_Offset offset, ADIO_Status *status, int
		      *error_code);   
void ADIOI_ZOIDFS_Fcntl(ADIO_File fd, int flag, ADIO_Fcntl_t *fcntl_struct, int
		*error_code); 
void ADIOI_ZOIDFS_WriteStrided(ADIO_File fd, void *buf, int count,
		       MPI_Datatype datatype, int file_ptr_type,
		       ADIO_Offset offset, ADIO_Status *status, int
		       *error_code);
void ADIOI_ZOIDFS_ReadStrided(ADIO_File fd, void *buf, int count,
		       MPI_Datatype datatype, int file_ptr_type,
		       ADIO_Offset offset, ADIO_Status *status, int
		       *error_code);
void ADIOI_ZOIDFS_Flush(ADIO_File fd, int *error_code);
void ADIOI_ZOIDFS_Delete(char *filename, int *error_code);
void ADIOI_ZOIDFS_Resize(ADIO_File fd, ADIO_Offset size, int *error_code);
void ADIOI_ZOIDFS_SetInfo(ADIO_File fd, MPI_Info users_info, int *error_code);
#endif
