/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* The number of bytes in a double.  */
#define SIZEOF_DOUBLE 8

/* The number of bytes in a float.  */
#define SIZEOF_FLOAT 4

/* The number of bytes in a int.  */
#define SIZEOF_INT 4

/* The number of bytes in a long.  */
#define SIZEOF_LONG 4

/* The number of bytes in a long double.  */
#define SIZEOF_LONG_DOUBLE 12

/* The number of bytes in a long long.  */
/*#define SIZEOF_LONG_LONG 8*/

/* The number of bytes in a short.  */
#define SIZEOF_SHORT 2

/* The number of bytes in a void *.  */
#define SIZEOF_VOID_P 4

/* The number of bytes in a wchar_t.  */
#define SIZEOF_WCHAR_T 2

/* Define if you have the getcwd function.  */
#define HAVE_GETCWD 1

/* Define if you have the gethostname function.  */
#define HAVE_GETHOSTNAME 1

/* Define if you have the gettimeofday function.  */
/*#define HAVE_GETTIMEOFDAY 1*/

/* Define if you have the mutex_init function.  */
/* #undef HAVE_MUTEX_INIT */

/* Define if you have the pthread_mutexattr_init function.  */
/*#define HAVE_PTHREAD_MUTEXATTR_INIT 1*/

/* Define if you have the pthread_mutexattr_setpshared function.  */
/*#define HAVE_PTHREAD_MUTEXATTR_SETPSHARED 1*/

/* Define if you have the putenv function.  */
#define HAVE_PUTENV 1

/* Define if you have the sched_yield function.  */
/*#define HAVE_SCHED_YIELD 1*/

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the shmat function.  */
/* #undef HAVE_SHMAT */

/* Define if you have the shmctl function.  */
/* #undef HAVE_SHMCTL */

/* Define if you have the shmdt function.  */
/* #undef HAVE_SHMDT */

/* Define if you have the shmget function.  */
/* #undef HAVE_SHMGET */

/* Define if you have the sleep function.  */
#define HAVE_SLEEP 1

/* Define if you have the socket function.  */
#define HAVE_SOCKET 1

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the usleep function.  */
/*#define HAVE_USLEEP 1*/

/* Define if you have the yield function.  */
/* #undef HAVE_YIELD */

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <linux/unistd.h> header file.  */
/* #undef HAVE_LINUX_UNISTD_H */

/* Define if you have the <netdb.h> header file.  */
/*#define HAVE_NETDB_H 1*/

/* Define if you have the <netinet/in.h> header file.  */
/*#define HAVE_NETINET_IN_H 1*/

/* Define if you have the <netinet/tcp.h> header file.  */
/*#define HAVE_NETINET_TCP_H 1*/

/* Define if you have the <pthread.h> header file.  */
/*#define HAVE_PTHREAD_H 1*/

/* Define if you have the <sched.h> header file.  */
/*#define HAVE_SCHED_H 1*/

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <stdio.h> header file.  */
#define HAVE_STDIO_H

/* Define if you have the <strings.h> header file.  */
#define HAVE_STRINGS_H 1

/* Define if you have the <sys/filio.h> header file.  */
/* #undef HAVE_SYS_FILIO_H */

/* Define if you have the <sys/ioctl.h> header file.  */
/*#define HAVE_SYS_IOCTL_H 1*/

/* Define if you have the <sys/ipc.h> header file.  */
/* #undef HAVE_SYS_IPC_H */

/* Define if you have the <sys/param.h> header file.  */
/*#define HAVE_SYS_PARAM_H 1*/

/* Define if you have the <sys/ptrace.h> header file.  */
/* #undef HAVE_SYS_PTRACE_H */

/* Define if you have the <sys/select.h> header file.  */
/*#define HAVE_SYS_SELECT_H 1*/

/* Define if you have the <sys/sem.h> header file.  */
/* #undef HAVE_SYS_SEM_H */

/* Define if you have the <sys/shm.h> header file.  */
/* #undef HAVE_SYS_SHM_H */

/* Define if you have the <sys/socket.h> header file.  */
/*#define HAVE_SYS_SOCKET_H 1*/

/* Define if you have the <sys/stat.h> header file.  */
/*#define HAVE_SYS_STAT_H 1*/

/* Define if you have the <sys/time.h> header file.  */
/*#define HAVE_SYS_TIME_H 1*/

/* Define if you have the <sys/types.h> header file.  */
/*#define HAVE_SYS_TYPES_H 1*/

/* Define if you have the <sys/uio.h> header file.  */
/*#define HAVE_SYS_UIO_H 1*/

/* Define if you have the <sys/wait.h> header file.  */
/*#define HAVE_SYS_WAIT_H 1*/

/* Define if you have the <unistd.h> header file.  */
/*#define HAVE_UNISTD_H 1*/

/* Define if you have the <values.h> header file.  */
/* #undef HAVE_VALUES_H */

/* Define if you have the thread library (-lthread).  */
/* #undef HAVE_LIBTHREAD */

/* define to enable error checking */
#define HAVE_ERROR_CHECKING MPID_ERROR_LEVEL_ALL

/* define to enable timing collection */
/*#define HAVE_TIMING MPID_TIMING_KIND_LOG*/

/* define to enable the RLOG logging library */
/*#define USE_LOGGING MPID_LOGGING_RLOG*/

/* define to enable the DLOG logging library */
/*#define USE_LOGGING_DLOG*/

/* define to for single threaded */
#define MPICH_SINGLE_THREADED 

/* Define to enable memory tracing */
/* #undef USE_MEMORY_TRACING */

/* Supports weak pragma */
/* #undef HAVE_PRAGMA_WEAK */

/* HP style weak pragma */
/* #undef HAVE_PRAGMA_HP_SEC_DEF */

/* Cray style weak pragma */
/* #undef HAVE_PRAGMA_CRI_DUP */

/* if C does not support volatile */
/* #undef volatile */

/* if C does not support restrict */
#define restrict

/* Define if int16_t is supported by the C compiler */
/*#define HAVE_INT16_T */

/* Define if int32_t is supported by the C compiler */
/*#define HAVE_INT32_T */

/* define if char * is byte pointer */
/* #undef CHAR_PTR_IS_BYTE */

/* Define for pthreads */
/* #undef HAVE_PTHREAD_CREATE */

/* Define for Solaris threads */
/* #undef HAVE_THR_CREATE */

/* Define if using gcc on a system with an Intel Pentium class chip */
/*#define HAVE_GCC_AND_PENTIUM_ASM 1*/

#define HAVE_PROCESS_H
#define HAVE_WINDOWS_H
#define HAVE_WINDOWS_SOCKET
#define HAVE_WINSOCK2_H
#define HAVE_WIN32_SLEEP
#define HAVE_NT_LOCKS
#define HAVE_MAPVIEWOFFILE
#define HAVE_CREATEFILEMAPPING
#define HAVE_INTERLOCKEDEXCHANGE
#define HAVE_BOOL

#define MPIU_INT16_T __int16
#define MPIU_INT32_T __int32
#define MPIU_INT64_T __int64

#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define inline __inline
#define HAVE_VSNPRINTF

#define WITH_SOCK_TYPE SOCK_IOCP

/* Define to use socket functions instead of bsocket functions */
/*#define NO_BSOCKETS*/
/* Define if building mpich in development mode */
#define MPICH_DEV_BUILD

/*#define MPICH_MPI_FROM_PMPI*/

#define MPICH_ERROR_MSG_LEVEL MPICH_ERROR_MSG_ALL
#define MPIR_MAX_ERROR_CLASS_INDEX 1
#define MPICH_MACROS_ARE_FUNCTIONS

#define USE_PROCESS_LOCKS 1

/*#define MPICH_DBG_OUTPUT*/
