/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_delete = PMPI_File_delete
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_delete MPI_File_delete
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_delete as PMPI_File_delete
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

extern int ADIO_Init_keyval;

/*@
    MPI_File_delete - Deletes a file

Input Parameters:
. filename - name of file to delete (string)
. info - info object (handle)

.N fortran
@*/
int MPI_File_delete(char *filename, MPI_Info info)
{
    int flag, error_code, file_system;
#ifndef PRINT_ERR_MSG
    static char myname[] = "MPI_FILE_DELETE";
#endif
    char *tmp;
#ifdef MPI_hpux
    int fl_xmpi;
  
    HPMP_IO_START(fl_xmpi, BLKMPIFILEDELETE, TRDTBLOCK,
                MPI_FILE_NULL, MPI_DATATYPE_NULL, -1);
#endif /* MPI_hpux */

    /* first check if ADIO has been initialized. If not, initialize it */
    if (ADIO_Init_keyval == MPI_KEYVAL_INVALID) {

   /* check if MPI itself has been initialized. If not, flag an error.
   Can't initialize it here, because don't know argc, argv */
        MPI_Initialized(&flag);
        if (!flag) {
            FPRINTF(stderr, "Error: MPI_Init() must be called before using MPI-IO\n");
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

    /* discover file system type (code swiped from MPI_File_open()) */
    file_system = -1;
    tmp = strchr(filename, ':');
    if (!tmp) {
	/* no prefix; use system-dependent function call to determine type */
	ADIO_FileSysType_fncall(filename, &file_system, &error_code);
	if (error_code != MPI_SUCCESS) {
#ifdef PRINT_ERR_MSG
	    FPRINTF(stderr, "MPI_File_delete: Can't determine the file-system type. Check the filename/path you provided and try again. Otherwise, prefix the filename with a string to indicate the type of file sytem (piofs:, pfs:, nfs:, ufs:, hfs:, xfs:, sfs:, pvfs:).\n");
#else
	error_code = MPIR_Err_setmsg(MPI_ERR_IO, MPIR_ERR_NO_FSTYPE,
				     myname, (char *) 0, (char *) 0);
	return ADIOI_Error(MPI_FILE_NULL, error_code, myname);
#endif
	}
    }
    else {
	/* prefix specified; just match via prefix, assume everyone has same */
	ADIO_FileSysType_prefix(filename, &file_system, &error_code);
	if (error_code != MPI_SUCCESS) {
#ifdef PRINT_ERR_MSG
	    FPRINTF(stderr, "MPI_File_delete: Can't determine the file-system type from the specified prefix. Check the filename/path and prefix you provided and try again.\n");
#else
	error_code = MPIR_Err_setmsg(MPI_ERR_IO, MPIR_ERR_NO_FSTYPE,
				     myname, (char *) 0, (char *) 0);
	return ADIOI_Error(MPI_FILE_NULL, error_code, myname);
#endif
	}
	/* move filename to point past the prefix: */
	filename = tmp + 1;
    }

    /* note: really we should check here to see if we support the given 
     * file system.  so this is a bit of a hack, but it's better than it 
     * was before.  all file systems other than PVFS are ok with the generic
     * delete call (so far).
     */
    switch (file_system) {
    case ADIO_PVFS:
#ifdef ROMIO_PVFS
	ADIOI_PVFS_Delete(filename, &error_code);
#else
#ifdef PRINT_ERR_MSG
	FPRINTF(stderr, "MPI_File_delete: ROMIO has not been configured to use the PVFS file system\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
#else
	error_code = MPIR_Err_setmsg(MPI_ERR_IO, MPIR_ERR_NO_PVFS,
				     myname, (char *) 0, (char *) 0);
	return ADIOI_Error(MPI_FILE_NULL, error_code, myname);
#endif	
#endif
	break;
    default:
	ADIOI_GEN_Delete(filename, &error_code);
	break;
    }
	
#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, MPI_FILE_NULL, MPI_DATATYPE_NULL, -1);
#endif /* MPI_hpux */
    return error_code;
}
