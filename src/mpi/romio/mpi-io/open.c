/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

extern int ADIO_Init_keyval;

/*@
    MPI_File_open - Opens a file

Input Parameters:
. comm - communicator (handle)
. filename - name of file to open (string)
. amode - file access mode (integer)
. info - info object (handle)

Output Parameters:
. fh - file handle (handle)

.N fortran
@*/
int MPI_File_open(MPI_Comm comm, char *filename, int amode, 
                  MPI_Info info, MPI_File *fh)
{
    int error_code, file_system, flag, tmp_amode, rank, orig_amode;
    int err, min_code;
    char *tmp;
#ifdef MPI_hpux
    int fl_xmpi;

    HPMP_IO_OPEN_START(fl_xmpi, comm);
#endif /* MPI_hpux */

    if (comm == MPI_COMM_NULL) {
	printf("MPI_File_open: Invalid communicator\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (!(amode & MPI_MODE_RDONLY) && !(amode & MPI_MODE_RDWR)
	  && !(amode & MPI_MODE_WRONLY) && !(amode & MPI_MODE_CREATE) 
            && !(amode & MPI_MODE_UNIQUE_OPEN) 
              && !(amode & MPI_MODE_DELETE_ON_CLOSE) 
                && !(amode & MPI_MODE_EXCL) 
                  && !(amode & MPI_MODE_APPEND)) {
	printf("MPI_File_open: Invalid amode\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (((amode & MPI_MODE_RDONLY) && (amode & MPI_MODE_RDWR))
      || ((amode & MPI_MODE_WRONLY) && (amode & MPI_MODE_RDWR))
	|| ((amode & MPI_MODE_RDONLY) && (amode & MPI_MODE_WRONLY))) {
	printf("MPI_File_open: Only one of MPI_MODE_RDONLY, MPI_MODE_WRONLY, or MPI_MODE_RDWR must be specified\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if ((amode & MPI_MODE_RDONLY) && 
            ((amode & MPI_MODE_CREATE) || (amode & MPI_MODE_EXCL))) {
	printf("MPI_File_open: It is erroneous to specify MPI_MODE_CREATE or MPI_MODE_EXCL with MPI_MODE_RDONLY\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

/* check if amode is the same on all processes */
    tmp_amode = amode;
    MPI_Bcast(&tmp_amode, 1, MPI_INT, 0, comm);
    if (amode != tmp_amode) {
	printf("MPI_File_open: amode must be the same on all processes\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }


/* check if ADIO has been initialized. If not, initialize it */
    if (ADIO_Init_keyval == MPI_KEYVAL_INVALID) {

/* check if MPI itself has been initialized. If not, flag an error.
   Can't initialize it here, because don't know argc, argv */
	MPI_Initialized(&flag);
	if (!flag) {
	    printf("Error: MPI_Init() must be called before using MPI-IO\n");
	    MPI_Abort(MPI_COMM_WORLD, 1);
	}

	MPI_Keyval_create(MPI_NULL_COPY_FN, ADIOI_End_call, &ADIO_Init_keyval,
			  (void *) 0);  

/* put a dummy attribute on MPI_COMM_WORLD, because we want the delete
   function to be called when MPI_COMM_WORLD is freed. Hopefully the
   MPI library frees MPI_COMM_WORLD when MPI_Finalize is called,
   though the standard does not mandate this. */

	MPI_Attr_put(MPI_COMM_WORLD, ADIO_Init_keyval, (void *) 0);

/* initialize ADIO */

	ADIO_Init( (int *)0, (char ***)0, &error_code);
    }


    file_system = -1;
    tmp = strchr(filename, ':');
    if (!tmp) {
	ADIO_FileSysType(filename, &file_system, &err);
	if (err != MPI_SUCCESS) {
	    printf("MPI_File_open: Can't determine the file-system type. Check the filename/path you provided and try again. Otherwise, prefix the filename with a string to indicate the type of file sytem (piofs:, pfs:, nfs:, ufs:, hfs:, xfs:, sfs:).\n");
	    MPI_Abort(MPI_COMM_WORLD, 1);
	}
	MPI_Allreduce(&file_system, &min_code, 1, MPI_INT, MPI_MIN, comm);
	if (min_code == ADIO_NFS) file_system = ADIO_NFS;
    }


#ifndef __PFS
    if (!strncmp(filename, "pfs:", 4) || !strncmp(filename, "PFS:", 4) || (file_system == ADIO_PFS)) {
	printf("MPI_File_open: ROMIO has not been configured to use the PFS file system\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }
#endif
#ifndef __PIOFS
    if (!strncmp(filename, "piofs:", 6) || !strncmp(filename, "PIOFS:", 6) || (file_system == ADIO_PIOFS)) {
	printf("MPI_File_open: ROMIO has not been configured to use the PIOFS file system\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }
#endif
#ifndef __UFS
    if (!strncmp(filename, "ufs:", 4) || !strncmp(filename, "UFS:", 4) || (file_system == ADIO_UFS)) {
	printf("MPI_File_open: ROMIO has not been configured to use the UFS file system\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }
#endif
#ifndef __NFS
    if (!strncmp(filename, "nfs:", 4) || !strncmp(filename, "NFS:", 4) || (file_system == ADIO_NFS)) {
	printf("MPI_File_open: ROMIO has not been configured to use the NFS file system\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }
#endif
#ifndef __HFS
    if (!strncmp(filename, "hfs:", 4) || !strncmp(filename, "HFS:", 4) || (file_system == ADIO_HFS)) {
	printf("MPI_File_open: ROMIO has not been configured to use the HFS file system\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }
#endif
#ifndef __XFS
    if (!strncmp(filename, "xfs:", 4) || !strncmp(filename, "XFS:", 4) || (file_system == ADIO_XFS)) {
	printf("MPI_File_open: ROMIO has not been configured to use the XFS file system\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }
#endif
#ifndef __SFS
    if (!strncmp(filename, "sfs:", 4) || !strncmp(filename, "SFS:", 4) || (file_system == ADIO_SFS)) {
	printf("MPI_File_open: ROMIO has not been configured to use the SFS file system\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }
#endif

    if (!strncmp(filename, "pfs:", 4) || !strncmp(filename, "PFS:", 4)) {
	file_system = ADIO_PFS;
	filename += 4;
    }
    else if (!strncmp(filename, "piofs:", 6) || !strncmp(filename, "PIOFS:", 6)) {
	file_system = ADIO_PIOFS;
	filename += 6;
    }
    else if (!strncmp(filename, "ufs:", 4) || !strncmp(filename, "UFS:", 4)) {
	file_system = ADIO_UFS;
	filename += 4;
    }
    else if (!strncmp(filename, "nfs:", 4) || !strncmp(filename, "NFS:", 4)) {
	file_system = ADIO_NFS;
	filename += 4;
    }
    else if (!strncmp(filename, "hfs:", 4) || !strncmp(filename, "HFS:", 4)) {
	file_system = ADIO_HFS;
	filename += 4;
    }
    else if (!strncmp(filename, "xfs:", 4) || !strncmp(filename, "XFS:", 4)) {
	file_system = ADIO_XFS;
	filename += 4;
    }
    else if (!strncmp(filename, "sfs:", 4) || !strncmp(filename, "SFS:", 4)) {
	file_system = ADIO_SFS;
	filename += 4;
    }

    orig_amode = amode;

    if ((amode & MPI_MODE_CREATE) && (amode & MPI_MODE_EXCL)) {
	/* the open should fail if the file exists. Only process 0 should
           check this. Otherwise, if all processes try to check and the file 
           does not exist, one process will create the file and others who 
           reach later will return error. */

	MPI_Comm_rank(comm, &rank);
	if (!rank) {
	    *fh = ADIO_Open(MPI_COMM_SELF, filename, file_system, amode, 0, 
               MPI_BYTE, MPI_BYTE, M_ASYNC, info, ADIO_PERM_NULL, &error_code);
	    /* broadcast the error code to other processes */
	    MPI_Bcast(&error_code, 1, MPI_INT, 0, comm);
	    /* if no error, close the file. It will be reopened normally 
               below. */
	    if (error_code == MPI_SUCCESS) ADIO_Close(*fh, &error_code);
	}
	else MPI_Bcast(&error_code, 1, MPI_INT, 0, comm);
	if (error_code != MPI_SUCCESS) {
	    *fh = MPI_FILE_NULL;
#ifdef MPI_hpux
	    HPMP_IO_OPEN_END(fl_xmpi, *fh, comm);
#endif /* MPI_hpux */
	    return error_code;
	}
	else amode = amode ^ MPI_MODE_EXCL;  /* turn off MPI_MODE_EXCL */
    }

/* use default values for disp, etype, filetype */    
/* set iomode=M_ASYNC. It is used to implement the Intel PFS interface
   on top of ADIO. Not relevant for MPI-IO implementation */    

    *fh = ADIO_Open(comm, filename, file_system, amode, 0, MPI_BYTE,
                     MPI_BYTE, M_ASYNC, info, ADIO_PERM_NULL, &error_code);

    /* if MPI_MODE_EXCL was removed, add it back */
    if ((error_code == MPI_SUCCESS) && (amode != orig_amode))
	(*fh)->access_mode = orig_amode;

#ifdef MPI_hpux
    HPMP_IO_OPEN_END(fl_xmpi, *fh, comm);
#endif /* MPI_hpux */
    return error_code;
}
