/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */


/* header file for MPI-IO implementation. not intended to be
   user-visible */ 

#ifndef MPIOIMPL_INCLUDE
#define MPIOIMPL_INCLUDE

#include "adio.h"
#include "mpio.h"

/* info is a linked list of these structures */
struct MPIR_Info {
    int cookie;
    char *key, *value;
    struct MPIR_Info *next;
};

#define MPIR_INFO_COOKIE 5835657

MPI_Delete_function ADIOI_End_call;

#include "mpioprof.h"

#ifdef MPI_hpux
#  include "mpioinst.h"
#endif /* MPI_hpux */

#ifdef MPICH2
int MPIR_Err_return_file( MPI_File, const char [], int );
int MPIR_Err_create_code( int, const char [], ... );
const char * MPIR_Err_get_string(int);
int MPIR_Err_set_msg( int code, const char *msg_string );
#endif

#endif

