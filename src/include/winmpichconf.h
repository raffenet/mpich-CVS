/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define as __inline if that's what the C compiler calls it.  */
#define inline __inline

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

/* Define if you have the putenv function.  */
#define HAVE_PUTENV 1

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the sleep function.  */
#define HAVE_SLEEP 1

/* Define if you have the socket function.  */
#define HAVE_SOCKET 1

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <stdio.h> header file.  */
#define HAVE_STDIO_H

/* define to enable error checking */
#define HAVE_ERROR_CHECKING MPID_ERROR_LEVEL_ALL
/*#undef HAVE_ERROR_CHECKING*/

/* define to enable timing collection */
/*#define HAVE_TIMING MPID_TIMING_KIND_LOG*/

/* define to enable the RLOG logging library */
/*#define USE_LOGGING MPID_LOGGING_RLOG*/

/* define to enable the DLOG logging library */
/*#define USE_LOGGING MPID_LOGGING_DLOG*/

/* define to for single threaded */
#define MPICH_SINGLE_THREADED 

/* Define to enable memory tracing */
/* #undef USE_MEMORY_TRACING */

/* if C does not support restrict */
#define restrict

/* define if char * is byte pointer */
/* #undef CHAR_PTR_IS_BYTE */

#define HAVE_PROCESS_H
#ifndef HAVE_WINDOWS_H
#define HAVE_WINDOWS_H
#endif
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
#define MPIU_SIZE_T unsigned int
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

/*#define MPICH_ERROR_MSG_LEVEL MPICH_ERROR_MSG_NONE*/
#define MPICH_ERROR_MSG_LEVEL MPICH_ERROR_MSG_ALL

#define USE_PROCESS_LOCKS 1

/*#define MPICH_DBG_OUTPUT*/
