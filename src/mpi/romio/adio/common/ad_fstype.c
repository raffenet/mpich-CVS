/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "adio.h"
#if (defined(HPUX) || defined(SPPUX) || defined(IRIX) || defined(SOLARIS) || defined(AIX) || defined(DEC) || defined(CRAY))
#include <sys/statvfs.h>
#endif
#ifdef LINUX
#include <sys/vfs.h>
/* #include <linux/nfs_fs.h> this file is broken in newer versions of linux */
#define NFS_SUPER_MAGIC 0x6969
#endif
#ifdef FREEBSD
#include <sys/param.h>
#include <sys/mount.h>
#endif
#ifdef PARAGON
#include <nx.h>
#include <pfs/pfs.h>
#include <sys/mount.h>
#endif
#ifdef SX4
#include <sys/stat.h>
#endif
#ifdef ROMIO_PVFS
#include "pvfs_config.h"
#endif
#ifdef tflops
#include <sys/mount.h>
#endif

/*
 ADIO_FileSysType_fncall - determines the file system type for a given file 
 using a system-dependent function call

Input Parameters:
. filename - pointer to file name character array

Output Parameters:
. fstype - location in which to store file system type (ADIO_XXX)
. error_code - location in which to store error code

 MPI_SUCCESS is stored in the location pointed to by error_code on success.

 This function is used by MPI_File_open() and MPI_File_delete() to determine 
 file system type.  Most other functions use the type which is stored when the 
 file is opened.
 */
void ADIO_FileSysType_fncall(char *filename, int *fstype, int *error_code)
{
    char *dir, *slash;
    int err;
#if (defined(HPUX) || defined(SPPUX) || defined(IRIX) || defined(SOLARIS) || defined(AIX) || defined(DEC) || defined(CRAY))
    struct statvfs vfsbuf;
#endif
#if (defined(LINUX) || defined(FREEBSD))
    struct statfs fsbuf;
#endif
#ifdef PARAGON
    struct estatfs ebuf;
#endif
#ifdef SX4
    struct stat sbuf;
#endif

    *error_code = MPI_SUCCESS;

    dir = strdup(filename);
    slash = strrchr(dir, '/');
    if (!slash) strcpy(dir, ".");
    else {
	if (slash == dir) *(dir + 1) = 0;
	else *slash = '\0';
    }

#if (defined(HPUX) || defined(SPPUX) || defined(IRIX) || defined(SOLARIS) || defined(AIX) || defined(DEC) || defined(CRAY))
    err = statvfs(filename, &vfsbuf);
    if (err && (errno == ENOENT)) err = statvfs(dir, &vfsbuf);
    free(dir);

    if (err) *error_code = MPI_ERR_UNKNOWN;
    else {
	/* FPRINTF(stderr, "%s\n", vfsbuf.f_basetype); */
	if (!strncmp(vfsbuf.f_basetype, "nfs", 3)) *fstype = ADIO_NFS;
	else {
# if (defined(HPUX) || defined(SPPUX))
#    ifdef HFS
	    *fstype = ADIO_HFS;
#    else
            *fstype = ADIO_UFS;
#    endif
# else
	    if (!strncmp(vfsbuf.f_basetype, "xfs", 3)) *fstype = ADIO_XFS;
	    else if (!strncmp(vfsbuf.f_basetype, "piofs", 4)) *fstype = ADIO_PIOFS;
	    else *fstype = ADIO_UFS;
# endif
	}
    }
#elif defined(LINUX)
    err = statfs(filename, &fsbuf);
    if (err && (errno == ENOENT)) err = statfs(dir, &fsbuf);
    free(dir);

    if (err) *error_code = MPI_ERR_UNKNOWN;
    else {
	/* FPRINTF(stderr, "%d\n", fsbuf.f_type);*/
	if (fsbuf.f_type == NFS_SUPER_MAGIC) *fstype = ADIO_NFS;
#ifdef ROMIO_PVFS
	else if (fsbuf.f_type == PVFS_SUPER_MAGIC) *fstype = ADIO_PVFS;
#endif
	else *fstype = ADIO_UFS;
    }
#elif (defined(FREEBSD) && defined(HAVE_MOUNT_NFS))
    err = statfs(filename, &fsbuf);
    if (err && (errno == ENOENT)) err = statfs(dir, &fsbuf);
    free(dir);

    if (err) *error_code = MPI_ERR_UNKNOWN;
    else {
#if (__FreeBSD_version>300004)
	if ( !strncmp("nfs",fsbuf.f_fstypename,3) ) *fstype = ADIO_NFS;
#else
	if (fsbuf.f_type == MOUNT_NFS) *fstype = ADIO_NFS;
#endif
	else *fstype = ADIO_UFS;
    }
#elif defined(PARAGON)
    err = statpfs(filename, &ebuf, 0, 0);
    if (err && (errno == ENOENT)) err = statpfs(dir, &ebuf, 0, 0);
    free(dir);

    if (err) *error_code = MPI_ERR_UNKNOWN;
    else {
	if (ebuf.f_type == MOUNT_NFS) *fstype = ADIO_NFS;
	else if (ebuf.f_type == MOUNT_PFS) *fstype = ADIO_PFS;
	else *fstype = ADIO_UFS;
    }
#elif defined(tflops)
    err = statfs(filename, &fsbuf);
    if (err && (errno == ENOENT)) err = statfs(dir, &fsbuf);
    free(dir);

    if (err) *error_code = MPI_ERR_UNKNOWN;
    else {
	if (fsbuf.f_type == MOUNT_NFS) *fstype = ADIO_NFS;
	else if (fsbuf.f_type == MOUNT_PFS) *fstype = ADIO_PFS;
	else *fstype = ADIO_UFS;
    }
#elif defined(SX4)
     err = stat (filename, &sbuf);
     if (err && (errno == ENOENT)) err = stat (dir, &sbuf);
     free(dir);
 
     if (err) *error_code = MPI_ERR_UNKNOWN;
     else {
         if (!strcmp(sbuf.st_fstype, "nfs")) *fstype = ADIO_NFS;
         else *fstype = ADIO_SFS;
     }
#else
    /* on other systems, make NFS the default */
    free(dir);
    *fstype = ADIO_NFS;   
#endif
}

/*
  ADIO_FileSysType_prefix - determines file system type for a file using 
  a prefix on the file name.  upper layer should have already determined
  that a prefix is present.

Input Parameters:
. filename - path to file, including prefix (xxx:)

Output Parameters:
. fstype - pointer to integer in which to store file system type (ADIO_XXX)
. error_code - pointer to integer in which to store error code

  Returns MPI_SUCCESS in error_code on success.  Filename not having a prefix
  is considered an error.

 */
void ADIO_FileSysType_prefix(char *filename, int *fstype, int *error_code)
{
    *error_code = MPI_SUCCESS;

    if (!strncmp(filename, "pfs:", 4) || !strncmp(filename, "PFS:", 4)) {
	*fstype = ADIO_PFS;
    }
    else if (!strncmp(filename, "piofs:", 6) || !strncmp(filename, "PIOFS:", 6)) {
	*fstype = ADIO_PIOFS;
    }
    else if (!strncmp(filename, "ufs:", 4) || !strncmp(filename, "UFS:", 4)) {
	*fstype = ADIO_UFS;
    }
    else if (!strncmp(filename, "nfs:", 4) || !strncmp(filename, "NFS:", 4)) {
	*fstype = ADIO_NFS;
    }
    else if (!strncmp(filename, "hfs:", 4) || !strncmp(filename, "HFS:", 4)) {
	*fstype = ADIO_HFS;
    }
    else if (!strncmp(filename, "xfs:", 4) || !strncmp(filename, "XFS:", 4)) {
	*fstype = ADIO_XFS;
    }
    else if (!strncmp(filename, "sfs:", 4) || !strncmp(filename, "SFS:", 4)) {
	*fstype = ADIO_SFS;
    }
    else if (!strncmp(filename, "pvfs:", 5) || !strncmp(filename, "PVFS:", 5)) {
	*fstype = ADIO_PVFS;
    }
    else {
	*fstype = 0;
	*error_code = MPI_ERR_UNKNOWN;
    }
}

