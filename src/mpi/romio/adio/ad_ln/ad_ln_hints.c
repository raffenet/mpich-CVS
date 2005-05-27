#include "ad_ln.h"

void ADIOI_LN_SetInfo(ADIO_File fd, MPI_Info users_info, int *error_code)
{
    char *value;
    int flag, tmp_val;
    static char myname[] = "ADIOI_LN_SETINFO";

    if (fd->info == MPI_INFO_NULL) MPI_Info_create(&(fd->info));
    
    if (users_info != MPI_INFO_NULL) {
      value = (char *) ADIOI_Malloc((MPI_MAX_INFO_VAL+1)*sizeof(char));
      
      MPI_Info_get(users_info, "LN_LBONE_SERVER", MPI_MAX_INFO_VAL,
		   value, &flag);
      if (flag) MPI_Info_set(fd->info, "LN_LBONE_SERVER", value);
      
      MPI_Info_get(users_info, "LN_LBONE_PORT", MPI_MAX_INFO_VAL,
		   value, &flag);
      if (flag) MPI_Info_set(fd->info, "LN_LBONE_PORT", value);
      
      MPI_Info_get(users_info, "LN_LBONE_LOCATION", MPI_MAX_INFO_VAL,
		   value, &flag);
      if (flag) MPI_Info_set(fd->info, "LN_LBONE_LOCATION", value);
      
      MPI_Info_get(users_info, "LN_LORS_BLOCKSIZE", MPI_MAX_INFO_VAL,
		   value, &flag);
      if (flag) MPI_Info_set(fd->info, "LN_LORS_BLOCKSIZE", value);
      
      MPI_Info_get(users_info, "LN_LORS_DURATION", MPI_MAX_INFO_VAL,
		   value, &flag);
      if (flag) MPI_Info_set(fd->info, "LN_LORS_DURATION", value);
      
      MPI_Info_get(users_info, "LN_LORS_COPIES", MPI_MAX_INFO_VAL,
		   value, &flag);
      if (flag) MPI_Info_set(fd->info, "LN_LORS_COPIES", value);
      
      MPI_Info_get(users_info, "LN_LORS_THREADS", MPI_MAX_INFO_VAL,
		   value, &flag);
      if (flag) MPI_Info_set(fd->info, "LN_LORS_THREADS", value);
      
      MPI_Info_get(users_info, "LN_LORS_TIMEOUT", MPI_MAX_INFO_VAL,
		   value, &flag);
      if (flag) MPI_Info_set(fd->info, "LN_LORS_TIMEOUT", value);
      
      MPI_Info_get(users_info, "LN_LORS_SERVERS", MPI_MAX_INFO_VAL,
		   value, &flag);
      if (flag) MPI_Info_set(fd->info, "LN_LORS_SERVERS", value);
      
      MPI_Info_get(users_info, "LN_LORS_SIZE", MPI_MAX_INFO_VAL,
		   value, &flag);
      if (flag) MPI_Info_set(fd->info, "LN_LORS_SIZE", value);
      
      MPI_Info_get(users_info, "LN_LORS_DEMO", MPI_MAX_INFO_VAL,
		   value, &flag);
      if (flag) MPI_Info_set(fd->info, "LN_LORS_DEMO", value);
      
      MPI_Info_get(users_info, "LN_IO_BUFFER_SIZE", MPI_MAX_INFO_VAL,
		   value, &flag);
      if (flag) MPI_Info_set(fd->info, "LN_IO_BUFFER_SIZE", value);
     
      MPI_Info_get(users_info, "LN_NO_SYNC_AT_COLLECTIVE_IO", MPI_MAX_INFO_VAL,
		   value, &flag);
      if (flag) MPI_Info_set(fd->info, "LN_NO_SYNC_AT_COLLECTIVE_IO", value);
     
      MPI_Info_get(users_info, "LN_DEPOT_LIST", MPI_MAX_INFO_VAL,
		   value, &flag);
      if (flag) MPI_Info_set(fd->info, "LN_DEPOT_LIST", value);
 
      ADIOI_Free(value);
    }
    
    /* set the values for collective I/O and data sieving parameters */
    ADIOI_GEN_SetInfo(fd, users_info, error_code);

   *error_code = MPI_SUCCESS;
}
