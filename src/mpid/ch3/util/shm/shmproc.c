/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* 
 * This file defines a few routines to allow one process to write to the 
 * memory of another process.  This makes use of a few special interfaces
 * provided by some operating systems.  For some Unix versions, ptrace
 * may be used.  Windows provides WriteProcessMemory etc
 * 
 */

#ifndef HAVE_WINDOWS_H
#include <sys/ptrace.h>

/* Initialize for reading and writing to the designated process */
int MPIDI_SHM_InitRWProc( pid_t pid, int *fd )
{
    char filename[256];
    int mpi_errno = MPI_SUCCESS;

    MPIU_Snprintf(filename, sizeof(filename), "/proc/%d/mem", pid);
    *fd = open(filename, O_RDRW );
    if (*fd == -1) {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**open", "**open %s %d %d", filename, info.pid, errno);
	return mpi_errno;
	
    }
    return mpi_errno;
}

/* Call with vc->ch.nSharedProcessID 
 * This must be called to allow other operations
 */
int MPIDI_SHM_AttachProc( pid_t pid )
{
    int mpi_errno = MPI_SUCCESS;
    int status;

    if (ptrace(PTRACE_ATTACH, pid, 0, 0) != 0) {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "ptrace attach failed", errno);
	return mpi_errno;
    }
    if (waitpid(pid, &status, WUNTRACED) != pid)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "waitpid failed", errno);
	return mpi_errno;
    }
    return MPI_SUCCESS;
}


/* Call with vc->ch.nSharedProcessID 
 * This should be called when access to the designated processes memory 
 * is no longer needed.  Use MPIDI_SHM_AttachProc to renew access to that
 * processes memory.
 */
int MPIDI_SHM_DetachProc( pid_t pid )
{
    int mpi_errno;
    if (ptrace(PTRACE_DETACH, vc->ch.nSharedProcessID, 0, 0) != 0) {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "ptrace detach failed", errno);
    }
    return mpi_errno;
}

/* Read by seeking to the memory location on the file descriptor and then
   using read. */
int MPIDI_SHM_ReadProcessMemory( int fd, 
				 const char *source, char *dest, size_t len )
{
    off_t offset = OFF_T_CAST(source);
    off_t uOffset;
    int mpi_errno = MPI_SUCCESS;

    uOffest = lseek( fd, offset, SEEK_SET );
    if (uOffset != offset) {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "lseek failed", errno);
	return mpi_errno;
    }

    num_read = read( fd, dest, len );
    if (num_read < 1) {
	if (num_read == -1) {
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "read failed", errno);
	    return mpi_errno;
	}
	/* If we only read part of the data, use ptrace to do what? */
	ptrace( PTRACE_PEEKDATA, pid, source+len - num_read, 0 );
    }
    /* FIXME: Now what? Why not continue to read? */
    return mpi_errno;
}

#else
/* HAVE_WINDOWS_H and use Windows interface */
/* Initialize for reading and writing to the designated process */
int MPIDI_SHM_InitRWProc( pid_t pid, int *phandle )
{
    *phandle =
	OpenProcess(STANDARD_RIGHTS_REQUIRED | PROCESS_VM_READ | 
		    PROCESS_VM_WRITE | PROCESS_VM_OPERATION, 
		    FALSE, pid);
    if (*phandle == NULL) {
	int err = GetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**OpenProcess", "**OpenProcess %d %d", info.pg_rank, err);
    }
    return mpi_errno;
}
#endif
