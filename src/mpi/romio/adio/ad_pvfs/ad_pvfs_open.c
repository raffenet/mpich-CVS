/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs.h"

void ADIOI_PVFS_Open(ADIO_File fd, int *error_code)
{
    int perm, amode, old_mask, flag;
    char *value;
    struct pvfs_stat pstat = {-1,-1,-1,0,0};

    if (fd->perm == ADIO_PERM_NULL) {
	old_mask = umask(022);
	umask(old_mask);
	perm = old_mask ^ 0666;
    }
    else perm = fd->perm;

    amode = O_META;
    if (fd->access_mode & ADIO_CREATE)
	amode = amode | O_CREAT;
    if (fd->access_mode & ADIO_RDONLY)
	amode = amode | O_RDONLY;
    if (fd->access_mode & ADIO_WRONLY)
	amode = amode | O_WRONLY;
    if (fd->access_mode & ADIO_RDWR)
	amode = amode | O_RDWR;
    if (fd->access_mode & ADIO_EXCL)
	amode = amode | O_EXCL;

    value = (char *) ADIOI_Malloc((MPI_MAX_INFO_VAL+1)*sizeof(char));

    MPI_Info_get(fd->info, "striping_factor", MPI_MAX_INFO_VAL, 
                         value, &flag);
    if (flag && (atoi(value) > 0)) pstat.pcount = atoi(value);

    MPI_Info_get(fd->info, "striping_unit", MPI_MAX_INFO_VAL, 
                         value, &flag);
    if (flag && (atoi(value) > 0)) pstat.ssize = atoi(value);

    MPI_Info_get(fd->info, "start_iodevice", MPI_MAX_INFO_VAL, 
                         value, &flag);
    if (flag && (atoi(value) >= 0)) pstat.base = atoi(value);

    ADIOI_Free(value);

    fd->fd_sys = pvfs_open(fd->filename, amode, perm, &pstat, NULL);

    if ((fd->fd_sys != -1) && (fd->access_mode & ADIO_APPEND))
	fd->fp_ind = fd->fp_sys_posn = pvfs_lseek(fd->fd_sys, 0, SEEK_END);

    if (fd->fd_sys != -1) {
	pvfs_ioctl(fd->fd_sys, GET_META, &pstat);
	sprintf(value, "%d", pstat.pcount);
	MPI_Info_set(fd->info, "striping_factor", value);
	sprintf(value, "%d", pstat.ssize);
	MPI_Info_set(fd->info, "striping_unit", value);
	sprintf(value, "%d", pstat.base);
	MPI_Info_set(fd->info, "start_iodevice", value);
    }

    *error_code = (fd->fd_sys == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
}
