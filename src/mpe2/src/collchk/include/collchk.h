/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_collchk_conf.h"

#if defined( STDC_HEADERS ) || defined( HAVE_UNISTD_H )
#include <unistd.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STRING_H )
#include <string.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDLIB_H )
#include <stdlib.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif

#include "mpi.h"

/* file handlers */
typedef struct {
        MPI_File fh;
        MPI_Comm comm;
} CollChk_fh_struct;

CollChk_fh_struct *CollChk_fh_list;

int CollChk_fh_cnt;

/* windows */
typedef struct {
        MPI_Win win;
        MPI_Comm comm;
} CollChk_win_struct;

CollChk_win_struct *CollChk_win_list;

int CollChk_win_cnt;

/* begin string */
char CollChk_begin_str[128];

/* the hash struct */
typedef struct {
        int hash_val; 
        int hash_cnt;
} CollChk_hash_struct;

/* constants */
int     COLLCHK_CALLED_BEGIN,
        COLLCHK_ERRORS,
        COLLCHK_ERR_NOT_INIT, 
        COLLCHK_ERR_ROOT, 
        COLLCHK_ERR_CALL, 
        COLLCHK_ERR_OP,
        COLLCHK_ERR_INPLACE,
        COLLCHK_ERR_DTYPE,
        COLLCHK_ERR_HIGH_LOW,
        COLLCHK_ERR_LL,
        COLLCHK_ERR_TAG,
        COLLCHK_ERR_DIMS,
        COLLCHK_ERR_GRAPH,
        COLLCHK_ERR_AMODE,
        COLLCHK_ERR_WHENCE,
        COLLCHK_ERR_DATAREP,
        COLLCHK_ERR_PREVIOUS_BEGIN,
        COLLCHK_ERR_FILE_NOT_OPEN;


void CollChk_add_fh( MPI_File fh, MPI_Comm comm );
void CollChk_add_win( MPI_Win win, MPI_Comm comm );
void CollChk_set_begin(char* in);
void CollChk_unset_begin(void);
int CollChk_check_buff(MPI_Comm comm, void * buff, char* call);
int CollChk_check_dims(MPI_Comm comm, int ndims, int *dims, char* call);
int CollChk_check_graph(MPI_Comm comm, int nnodes, int *index, int* edges,
                        char* call);
int CollChk_check_size(MPI_Comm comm, int size, char* call);
int CollChk_err_han(char * err_str, int err_code, char * call, MPI_Comm comm);
int CollChk_get_fh(MPI_File fh, MPI_Comm *comm);
int CollChk_get_win(MPI_Win win, MPI_Comm *comm);
int CollChk_is_init(void);
int CollChk_same_amode(MPI_Comm comm, int amode, char* call);
int CollChk_same_call(MPI_Comm comm, char* call);
int CollChk_same_datarep(MPI_Comm comm, char* datarep, char *call);

void CollChk_h(int a, int n, int b, int m, int *hash_val, int *hash_cnt);
int CollChk_get_val(MPI_Datatype dt);
int CollChk_get_cnt(int n, int *ints, int combiner);
void CollChk_hash_dtype(MPI_Datatype dt, int cnt, int *hash_val, int *hash_cnt);
int CollChk_same_dtype(MPI_Comm comm, int cnt, MPI_Datatype dt, char* call);
int CollChk_same_dtype_vector(MPI_Comm comm, int root, int cnt,
                              int *rootcnts, MPI_Datatype dt, char *call);
int CollChk_same_dtype_vector2(MPI_Comm comm, int *cnts,
                               MPI_Datatype dt, char *call);
int CollChk_same_dtype_general(MPI_Comm comm, int *rcnts, int *scnts,
                               MPI_Datatype *rtypes, MPI_Datatype *stypes,
                               char *call);

int CollChk_same_high_low(MPI_Comm comm, int high_low, char* call);
int CollChk_same_int(MPI_Comm comm, int val, char* call, char* check,
                     char* err_str);
int CollChk_same_local_leader(MPI_Comm comm, int ll, char* call);
char* CollChk_get_op_string(MPI_Op op);
int CollChk_same_op(MPI_Comm comm, MPI_Op op, char* call);
int CollChk_same_root(MPI_Comm comm, int root, char* call);
int CollChk_same_tag(MPI_Comm comm, int tag, char* call);
int CollChk_same_whence(MPI_Comm comm, int whence, char* call);
