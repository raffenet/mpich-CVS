/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_piofs.h"
#ifdef __PROFILE
#include "mpe.h"
#endif

void ADIOI_PIOFS_Open(ADIO_File fd, int *error_code)
{
    int amode, perm, old_mask, err;
    piofs_fstat_t piofs_fstat;
    char *value;

    if (fd->perm == ADIO_PERM_NULL) {
	old_mask = umask(022);
	umask(old_mask);
	perm = old_mask ^ 0666;
    }
    else perm = fd->perm;

    amode = 0;
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

#ifdef __PROFILE
    MPE_Log_event(1, 0, "start open");
#endif
    fd->fd_sys = open(fd->filename, amode, perm);
#ifdef __PROFILE
    MPE_Log_event(2, 0, "end open");
#endif

    llseek(fd->fd_sys, 0, SEEK_SET);
/* required to initiate use of 64-bit offset */

    if (fd->fd_sys != -1) {
	value = (char *) ADIOI_Malloc((MPI_MAX_INFO_VAL+1)*sizeof(char));

        /* get file striping information and set it in info */
	err = piofsioctl(fd->fd_sys, PIOFS_FSTAT, &piofs_fstat);

	if (!err) {
	    sprintf(value, "%d", piofs_fstat.st_bsu);
	    MPI_Info_set(fd->info, "striping_unit", value);

	    sprintf(value, "%d", piofs_fstat.st_cells);
	    MPI_Info_set(fd->info, "striping_factor", value);

	    sprintf(value, "%d", piofs_fstat.st_base_node);
	    MPI_Info_set(fd->info, "start_iodevice", value);
	}
	ADIOI_Free(value);

	if (fd->access_mode & ADIO_APPEND) {
	    fd->fp_ind = fd->fp_sys_posn = llseek(fd->fd_sys, 0, SEEK_END);
	    MPI_Barrier(fd->comm);
        /* the barrier ensures that no process races ahead and modifies
           the file size before all processes have opened the file. */
	}
    }

    *error_code = (fd->fd_sys == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
}
