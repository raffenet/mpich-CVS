/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs.h"

void ADIOI_PVFS_SetInfo(ADIO_File fd, MPI_Info users_info, int *error_code)
{
    char *value;
    int flag, tmp_val, str_factor=-1, str_unit=-1, start_iodev=-1;

    if (!(fd->info)) {
	/* This must be part of the open call. can set striping parameters 
           if necessary. */ 
	MPI_Info_create(&(fd->info));
	
	/* has user specified striping parameters 
           and do they have the same value on all processes? */
	if (users_info != MPI_INFO_NULL) {
	    value = (char *) ADIOI_Malloc((MPI_MAX_INFO_VAL+1)*sizeof(char));

	    MPI_Info_get(users_info, "striping_factor", MPI_MAX_INFO_VAL, 
			 value, &flag);
	    if (flag) {
		str_factor=atoi(value);
		tmp_val = str_factor;
		MPI_Bcast(&tmp_val, 1, MPI_INT, 0, fd->comm);
		if (tmp_val != str_factor) {
		    printf("ADIOI_PVFS_SetInfo: the value for key \"striping_factor\" must be the same on all processes\n");
		    MPI_Abort(MPI_COMM_WORLD, 1);
		}
		else MPI_Info_set(fd->info, "striping_factor", value);
	    }

	    MPI_Info_get(users_info, "striping_unit", MPI_MAX_INFO_VAL, 
			 value, &flag);
	    if (flag) {
		str_unit=atoi(value);
		tmp_val = str_unit;
		MPI_Bcast(&tmp_val, 1, MPI_INT, 0, fd->comm);
		if (tmp_val != str_unit) {
		    printf("ADIOI_PVFS_SetInfo: the value for key \"striping_unit\" must be the same on all processes\n");
		    MPI_Abort(MPI_COMM_WORLD, 1);
		}
		else MPI_Info_set(fd->info, "striping_unit", value);
	    }

	    MPI_Info_get(users_info, "start_iodevice", MPI_MAX_INFO_VAL, 
			 value, &flag);
	    if (flag) {
		start_iodev=atoi(value);
		tmp_val = start_iodev;
		MPI_Bcast(&tmp_val, 1, MPI_INT, 0, fd->comm);
		if (tmp_val != start_iodev) {
		    printf("ADIOI_PVFS_SetInfo: the value for key \"start_iodevice\" must be the same on all processes\n");
		    MPI_Abort(MPI_COMM_WORLD, 1);
		}
		else MPI_Info_set(fd->info, "start_iodevice", value);
	    }

	    ADIOI_Free(value);
	}
    }	

    /* set the values for collective I/O and data sieving parameters */
    ADIOI_GEN_SetInfo(fd, users_info, error_code);

    *error_code = MPI_SUCCESS;
}
