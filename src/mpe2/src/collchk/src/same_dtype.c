/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#else
# if defined( HAVE_ALLOCA_H )
# include <alloca.h>
# endif
#endif


unsigned int CollChk_cirleftshift( unsigned int alpha, unsigned n );
unsigned int CollChk_cirleftshift( unsigned int alpha, unsigned n )
{
    /* Doing circular left shift of alpha by n bits */
    unsigned int t1, t2;
    t1 = alpha >> (sizeof(unsigned int)-n);
    t2 = alpha << n;
    return t1 | t2;
}

void CollChk_hash_add(const CollChk_hash_t *alpha,
                      const CollChk_hash_t *beta,
                            CollChk_hash_t *lamda);
void CollChk_hash_add(const CollChk_hash_t *alpha,
                      const CollChk_hash_t *beta,
                            CollChk_hash_t *lamda)
{
    lamda->value = (alpha->value)
                 ^ CollChk_cirleftshift(beta->value, alpha->count);
    lamda->count = alpha->count + beta->count;
}

int CollChk_hash_equal(const CollChk_hash_t *alpha,
                       const CollChk_hash_t *beta)
{
    return alpha->count == beta->count && alpha->value == beta->value;
}


unsigned int CollChk_basic_value(MPI_Datatype type);
unsigned int CollChk_basic_value(MPI_Datatype type)
{
    switch (type) {
        /*
           MPI_Datatype's that return 0x0 are as if they are being
           skipped/ignored in the comparison of any 2 MPI_Datatypes.
        */
        case MPI_DATATYPE_NULL :
        case MPI_UB :
        case MPI_LB :
            return 0x0;

        case MPI_CHAR :
            return 0x1;
        case MPI_SIGNED_CHAR :
            return 0x3;
        case MPI_UNSIGNED_CHAR :
            return 0x5;
        case MPI_BYTE :
            return 0x7;
        case MPI_WCHAR :
            return 0x9;
        case MPI_SHORT :
            return 0xb;
        case MPI_UNSIGNED_SHORT :
            return 0xd;
        case MPI_INT :
            return 0xf;
        case MPI_UNSIGNED :
            return 0x11;
        case MPI_LONG :
            return 0x13;
        case MPI_UNSIGNED_LONG :
            return 0x15;
        case MPI_FLOAT :
            return 0x17;
        case MPI_DOUBLE :
            return 0x19;
        case MPI_LONG_DOUBLE :
            return 0x1b;
        /* case MPI_LONG_LONG_INT : return 0x1d; */
        case MPI_LONG_LONG :
            return 0x1f;
        case MPI_UNSIGNED_LONG_LONG :
            return 0x21;
        case MPI_PACKED :
            return 0x23;

        case MPI_FLOAT_INT :
            return 0x8;       /* (0x17,1)@(0xf,1) */
        case MPI_DOUBLE_INT :
            return 0x6;       /* (0x19,1)@(0xf,1) */
        case MPI_LONG_INT :
            return 0xc;       /* (0x13,1)@(0xf,1) */
        case MPI_SHORT_INT :
            return 0x14;      /* (0xb,1)@(0xf,1) */
        case MPI_2INT :
            return 0x10;      /* (0xf,1)@(0xf,1) */
        case MPI_LONG_DOUBLE_INT :
            return 0x4;       /* (0x1b,1)@(0xf,1) */

        case MPI_COMPLEX :
            return 0x101;
        case MPI_DOUBLE_COMPLEX :
            return 0x103;
        case MPI_LOGICAL :
            return 0x105;
        case MPI_REAL :
            return 0x107;
        case MPI_DOUBLE_PRECISION :
            return 0x109;
        case MPI_INTEGER :
            return 0x10b;
        case MPI_CHARACTER :
            return 0x10d;

        case MPI_2INTEGER :
            return 0x33c;      /* (0x10b,1)@(0x10b,1) */
        case MPI_2COMPLEX :
            return 0x323;      /* (0x101,1)@(0x101,1) */
        case MPI_2DOUBLE_COMPLEX :
            return 0x325;      /* (0x103,1)@(0x103,1) */
        case MPI_2REAL :
            return 0x329;      /* (0x107,1)@(0x107,1) */
        case MPI_2DOUBLE_PRECISION :
            return 0x33a;      /* (0x109,1)@(0x109,1) */

        case MPI_REAL4 :
            return 0x201;
        case MPI_REAL8 :
            return 0x203;
        /* case MPI_REAL16 : return 0x205; */
        case MPI_COMPLEX8 :
            return 0x207;
        case MPI_COMPLEX16 :
            return 0x209;
        /* case MPI_COMPLEX32 : return 0x20b; */
        case MPI_INTEGER1 :
            return 0x211;
        case MPI_INTEGER2 :
            return 0x213;
        case MPI_INTEGER4 :
            return 0x215;
        case MPI_INTEGER8 :
            return 0x217;
        /* case MPI_INTEGER16 : return 0x219; */

        default :
            fprintf( stderr, "CollChk_basic_value(): "
                             "Unknown basic MPI datatype %x.\n", type );
            fflush( stderr );
            return 0;
    }
}

unsigned int CollChk_basic_count(MPI_Datatype type);
unsigned int CollChk_basic_count(MPI_Datatype type)
{
    switch (type) {
        /* MPI_Datatype's that return 0 are being skipped/ignored. */
        case MPI_DATATYPE_NULL :
        case MPI_UB :
        case MPI_LB :
            return 0;

        case MPI_CHAR :
        case MPI_SIGNED_CHAR :
        case MPI_UNSIGNED_CHAR :
        case MPI_BYTE :
        case MPI_WCHAR :
        case MPI_SHORT :
        case MPI_UNSIGNED_SHORT :
        case MPI_INT :
        case MPI_UNSIGNED :
        case MPI_LONG :
        case MPI_UNSIGNED_LONG :
        case MPI_FLOAT :
        case MPI_DOUBLE :
        case MPI_LONG_DOUBLE :
        case MPI_LONG_LONG_INT :
        /* case MPI_LONG_LONG : */
        case MPI_UNSIGNED_LONG_LONG :
        case MPI_PACKED :
            return 1;

        case MPI_FLOAT_INT :        
        case MPI_DOUBLE_INT :
        case MPI_LONG_INT :
        case MPI_SHORT_INT :
        case MPI_2INT :
        case MPI_LONG_DOUBLE_INT :
            return 2;

        case MPI_COMPLEX :
        case MPI_DOUBLE_COMPLEX :
        case MPI_LOGICAL :
        case MPI_REAL :
        case MPI_DOUBLE_PRECISION :
        case MPI_INTEGER :
        case MPI_CHARACTER :
            return 1;

        case MPI_2INTEGER :
        case MPI_2COMPLEX :
        case MPI_2DOUBLE_COMPLEX :
        case MPI_2REAL :
        case MPI_2DOUBLE_PRECISION :
            return 2;

        case MPI_REAL4 :
        case MPI_REAL8 :
        /* case MPI_REAL16 : */
        case MPI_COMPLEX8 :
        case MPI_COMPLEX16 :
        /* case MPI_COMPLEX32 : */
        case MPI_INTEGER1 :
        case MPI_INTEGER2 :
        case MPI_INTEGER4 :
        case MPI_INTEGER8 :
        /* case MPI_INTEGER16 : */
            return 1;

        default :
            fprintf( stderr, "CollChk_basic_count(): "
                             "Unknown basic MPI datatype %x.\n", type );
            fflush( stderr );
            return 0;
    }
}


int CollChk_derived_count(int idx, int *ints, int combiner);
int CollChk_derived_count(int idx, int *ints, int combiner)
{
    int ii, tot_cnt;
    int dim_A, dim_B;
    
    tot_cnt = 0;
    switch(combiner) {
        case MPI_COMBINER_DUP : 
        case MPI_COMBINER_F90_REAL :
        case MPI_COMBINER_F90_COMPLEX :
        case MPI_COMBINER_F90_INTEGER :
        case MPI_COMBINER_RESIZED :
            return 1;
        case MPI_COMBINER_CONTIGUOUS :
            return ints[0];
        case MPI_COMBINER_VECTOR :
        case MPI_COMBINER_HVECTOR :
        case MPI_COMBINER_HVECTOR_INTEGER :
        case MPI_COMBINER_INDEXED_BLOCK :
            return ints[0]*ints[1];
        case MPI_COMBINER_INDEXED :
        case MPI_COMBINER_HINDEXED :
        case MPI_COMBINER_HINDEXED_INTEGER :
            for ( ii = ints[0]; ii > 0; ii-- ) {
                 tot_cnt += ints[ ii ];
            }
            return tot_cnt;
        case MPI_COMBINER_STRUCT :
        case MPI_COMBINER_STRUCT_INTEGER :
            return ints[idx+1];
        case MPI_COMBINER_SUBARRAY :
            dim_A   = ints[ 0 ] + 1;
            dim_B   = 2 * ints[ 0 ];
            for ( ii=dim_A; ii<=dim_B; ii++ ) {
                tot_cnt += ints[ ii ];
            }
            return tot_cnt;
        case MPI_COMBINER_DARRAY :
            for ( ii=3; ii<=ints[2]+2; ii++ ) {
                tot_cnt += ints[ ii ];
            }
            return tot_cnt;
    }
    return tot_cnt;
}


void CollChk_dtype_hash(MPI_Datatype type, int cnt, CollChk_hash_t *type_hash)
{
    int             nints, nadds, ntypes, combiner;
    int             *ints; 
    MPI_Aint        *adds; 
    MPI_Datatype    *types;
    CollChk_hash_t  curr_hash, next_hash;
    int             type_cnt;
    int             ii;

    /*  Don't know if this makes sense or not */
    if ( cnt <= 0 ) {
        /* (value,count)=(0,0) => skipping of this (type,cnt) in addition */
        type_hash->value = 0;
        type_hash->count = 0;
        return;
    }

    MPI_Type_get_envelope(type, &nints, &nadds, &ntypes, &combiner);
    if (combiner != MPI_COMBINER_NAMED) {
#if ! defined( HAVE_ALLOCA )
        ints = NULL;
        if ( nints > 0 )
            ints = (int *) malloc(nints * sizeof(int)); 
        adds = NULL;
        if ( nadds > 0 )
            adds = (MPI_Aint *) malloc(nadds * sizeof(MPI_Aint)); 
        types = NULL;
        if ( ntypes > 0 )
            types = (MPI_Datatype *) malloc(ntypes * sizeof(MPI_Datatype));
#else
        ints = NULL;
        if ( nints > 0 )
            ints = (int *) alloca(nints * sizeof(int)); 
        adds = NULL;
        if ( nadds > 0 )
            adds = (MPI_Aint *) alloca(nadds * sizeof(MPI_Aint)); 
        types = NULL;
        if ( ntypes > 0 )
            types = (MPI_Datatype *) alloca(ntypes * sizeof(MPI_Datatype));
#endif

        MPI_Type_get_contents(type, nints, nadds, ntypes, ints, adds, types);
        type_cnt = CollChk_derived_count(0, ints, combiner);
        CollChk_dtype_hash(types[0], type_cnt, &curr_hash);

        /*
            ntypes > 1 only for MPI_COMBINER_STRUCT(_INTEGER)
        */
        for( ii=1; ii < ntypes; ii++) {
            type_cnt = CollChk_derived_count(ii, ints, combiner); 
            CollChk_dtype_hash(types[ii], type_cnt, &next_hash);
            CollChk_hash_add(&curr_hash, &next_hash, &curr_hash);
        }

#if ! defined( HAVE_ALLOCA )
        if ( ints != NULL )
            free( ints );
        if ( adds != NULL )
            free( adds );
        if ( types != NULL )
            free( types );
#endif
    }
    else {
        curr_hash.value = CollChk_basic_value(type);
        curr_hash.count = CollChk_basic_count(type);
    }

    type_hash->value = curr_hash.value;
    type_hash->count = curr_hash.count;
    for ( ii=1; ii < cnt; ii++ ) {
        CollChk_hash_add(type_hash, &curr_hash, type_hash);
    }
}


/*
   Checking if (type,cnt) is the same in all processes within the communicator.
*/
int CollChk_dtype_bcast(MPI_Comm comm, MPI_Datatype type, int cnt, int root,
                        char* call)
{
#if 0
    CollChk_hash_t  local_hash;        /* local hash value */
    CollChk_hash_t  root_hash;         /* root's hash value */
    char            err_str[COLLCHK_STD_STRLEN];
    int             rank, size;        /* rank, size */
    int             are_hashes_equal;  /* go flag, ok flag */

    /* get the rank and size */
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /* get the hash values */
    CollChk_dtype_hash(type, cnt, &local_hash);

    if (rank == root) {
        root_hash.value = local_hash.value;
        root_hash.count = local_hash.count;
    }
    /* broadcast root's datatype hash to all other processes */
    PMPI_Bcast(&root_hash, 2, MPI_UNSIGNED, root, comm);

    /* Compare root's datatype hash to the local hash */
    are_hashes_equal = CollChk_hash_equal( &local_hash, &root_hash );
    if ( !are_hashes_equal )
        sprintf(err_str, "Inconsistent datatype signatures detected "
                         "between rank %d and rank %d.\n", rank, root);
    else
        sprintf(err_str, COLLCHK_NO_ERROR_STR);

    /* Find out if there is unequal hashes in the communicator */
    PMPI_Allreduce( &are_hashes_equal, &are_hashes_equal, 1, MPI_INT,
                    MPI_LAND, comm );

    if ( !are_hashes_equal )
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);

    return MPI_SUCCESS;
#endif
#if defined( DEBUG )
    fprintf( stdout, "CollChk_dtype_bcast()\n" );
#endif
    return CollChk_dtype_scatter(comm, type, cnt, type, cnt, root, 1, call );
}


/*
  The (sendtype,sendcnt) is assumed to be known in root process.
  (recvtype,recvcnt) is known in every process.  The routine checks if
  (recvtype,recvcnt) on each process is the same as (sendtype,sendcnt)
  on process root.
*/
int CollChk_dtype_scatter(MPI_Comm comm,
                          MPI_Datatype sendtype, int sendcnt,
                          MPI_Datatype recvtype, int recvcnt,
                          int root, int are2buffs, char *call)
{
    CollChk_hash_t  root_hash;         /* root's hash value */
    CollChk_hash_t  recv_hash;         /* local hash value */
    char            err_str[COLLCHK_STD_STRLEN];
    int             rank, size;
    int             are_hashes_equal;

#if defined( DEBUG )
    fprintf( stdout, "CollChk_dtype_scatter()\n" );
#endif

    /* get the rank and size */
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /*
       Scatter() only cares root's send datatype signature,
       i.e. ignore not-root's send datatype signatyre
    */
    /* Set the root's hash value */
    if (rank == root)
        CollChk_dtype_hash(sendtype, sendcnt, &root_hash);

    /* broadcast root's datatype hash to all other processes */
    PMPI_Bcast(&root_hash, 2, MPI_UNSIGNED, root, comm);

    /* Compare root_hash with the input/local hash */
    if ( are2buffs ) {
        CollChk_dtype_hash( recvtype, recvcnt, &recv_hash );
        are_hashes_equal = CollChk_hash_equal( &root_hash, &recv_hash );
    }
    else
        are_hashes_equal = 1;

    if ( !are_hashes_equal )
        sprintf(err_str, "Inconsistent datatype signatures detected "
                         "between rank %d and rank %d.\n", rank, root);
    else
        sprintf(err_str, COLLCHK_NO_ERROR_STR);

    /* Find out if there is unequal hashes in the communicator */
    PMPI_Allreduce( &are_hashes_equal, &are_hashes_equal, 1, MPI_INT,
                    MPI_LAND, comm );

    if ( !are_hashes_equal )
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);

    return MPI_SUCCESS;
}

/*
  The vector of (sendtype,sendcnts[]) is assumed to be known in root process.
  (recvtype,recvcnt) is known in every process.  The routine checks if
  (recvtype,recvcnt) on process P is the same as (sendtype,sendcnt[P])
  on process root. 
*/
int CollChk_dtype_scatterv(MPI_Comm comm,
                           MPI_Datatype sendtype, int *sendcnts,
                           MPI_Datatype recvtype, int recvcnt,
                           int root, int are2buffs, char *call)
{
    CollChk_hash_t  *hashes;         /* hash array for (sendtype,sendcnts[]) */
    CollChk_hash_t  root_hash;       /* root's hash value */
    CollChk_hash_t  recv_hash;       /* local hash value */
    char            err_str[COLLCHK_STD_STRLEN];
    int             rank, size, idx;
    int             are_hashes_equal;

#if defined( DEBUG )
    fprintf( stdout, "CollChk_dtype_scatterv()\n" );
#endif

    /* get the rank and size */
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /*
       Scatter() only cares root's send datatype signature[],
       i.e. ignore not-root's send datatype signatyre
    */
    hashes = NULL;
    if ( rank == root ) {
        /* Allocate hash buffer memory */
#if ! defined( HAVE_ALLOCA )
        hashes = (CollChk_hash_t *) malloc( size * sizeof(CollChk_hash_t) );
#else
        hashes = (CollChk_hash_t *) alloca( size * sizeof(CollChk_hash_t) );
#endif
        for ( idx = 0; idx < size; idx++ )
            CollChk_dtype_hash( sendtype, sendcnts[idx], &(hashes[idx]) );
    }

    /* Send the root's hash array to update other processes's root_hash */
    PMPI_Scatter(hashes, 2, MPI_UNSIGNED, &root_hash, 2, MPI_UNSIGNED,
                 root, comm);

    /* Compare root_hash with the input/local hash */
    if ( are2buffs ) {
        CollChk_dtype_hash( recvtype, recvcnt, &recv_hash );    
        are_hashes_equal = CollChk_hash_equal( &root_hash, &recv_hash );
    }
    else
        are_hashes_equal = 1;

    if ( !are_hashes_equal )
        sprintf(err_str, "Inconsistent datatype signatures detected "
                         "between rank %d and rank %d.\n", rank, root);
    else
        sprintf(err_str, COLLCHK_NO_ERROR_STR);

    /* Find out if there is unequal hashes in the communicator */
    PMPI_Allreduce( &are_hashes_equal, &are_hashes_equal, 1, MPI_INT,
                    MPI_LAND, comm );

#if ! defined( HAVE_ALLOCA )
    if ( hashes != NULL )
        free( hashes );
#endif

    if ( !are_hashes_equal )
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);

    return MPI_SUCCESS;
}


/*
   (sendtype,sendcnt) and (recvtype,recvcnt) are known in every process.
   The routine checks if (recvtype,recvcnt) on local process is the same
   as (sendtype,sendcnt) collected from all the other processes.
*/
int CollChk_dtype_allgather(MPI_Comm comm,
                            MPI_Datatype sendtype, int sendcnt,
                            MPI_Datatype recvtype, int recvcnt,
                            int are2buffs, char *call)
{
    CollChk_hash_t  *hashes;      /* hashes from other senders' */
    CollChk_hash_t  send_hash;    /* local sender's hash value */
    CollChk_hash_t  recv_hash;    /* local receiver's hash value */
    char            err_str[COLLCHK_STD_STRLEN];
    char            rank_str[COLLCHK_SM_STRLEN];
    int             *isOK2chks;   /* boolean array, true:sendbuff=\=recvbuff */
    int             *err_ranks;   /* array of ranks that have mismatch hashes */
    int             err_rank_size;
    int             err_str_sz, str_sz;
    int             rank, size, idx;

#if defined( DEBUG )
    fprintf( stdout, "CollChk_dtype_allgather()\n" );
#endif

    /* get the rank and size */
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    CollChk_dtype_hash( sendtype, sendcnt, &send_hash );
    /* Allocate hash buffer memory */
#if ! defined( HAVE_ALLOCA )
    hashes    = (CollChk_hash_t *) malloc( size * sizeof(CollChk_hash_t) );
    err_ranks = (int *) malloc( size * sizeof(int) );
    isOK2chks = (int *) malloc( size * sizeof(int) );
#else
    hashes    = (CollChk_hash_t *) alloca( size * sizeof(CollChk_hash_t) );
    err_ranks = (int *) alloca( size * sizeof(int) );
    isOK2chks = (int *) alloca( size * sizeof(int) );
#endif

    /* Gather other senders' datatype hashes as local hash arrary */
    PMPI_Allgather(&send_hash, 2, MPI_UNSIGNED, hashes, 2, MPI_UNSIGNED, comm);
    PMPI_Allgather(&are2buffs, 1, MPI_INT, isOK2chks, 1, MPI_INT, comm);

    /* Compute the local datatype hash value */
    CollChk_dtype_hash( recvtype, recvcnt, &recv_hash );

    /* Compare the local datatype hash with other senders' datatype hashes */
    /*
       The checks are more exhaustive and redundant tests on all processes,
       but matches what user expects
    */
    err_rank_size = 0;
    for ( idx = 0; idx < size; idx++ ) {
        if ( isOK2chks[idx] ) {
            if ( ! CollChk_hash_equal( &recv_hash, &(hashes[idx]) ) )
                err_ranks[ err_rank_size++ ] = idx;
        }
    }

    if ( err_rank_size > 0 ) {
        str_sz = sprintf(err_str, "Inconsistent datatype signatures detected "
                                  "between local rank %d and remote ranks,",
                                  rank);
        /* all string size variables, *_sz, does not include NULL character */
        err_str_sz = str_sz;
        for ( idx = 0; idx < err_rank_size; idx++ ) {
            str_sz = sprintf(rank_str, " %d", err_ranks[idx] );
            /* -3 is reserved for "..." */
            if ( str_sz + err_str_sz < COLLCHK_STD_STRLEN-3 ) {
                strcat(err_str, rank_str);
                err_str_sz = strlen( err_str );
            }
            else {
                strcat(err_str, "..." );
                break;
            }
        }
    }
    else
        sprintf(err_str, COLLCHK_NO_ERROR_STR);

    /* Find out the total number of unequal hashes in the communicator */
    PMPI_Allreduce( &err_rank_size, &err_rank_size, 1, MPI_INT,
                    MPI_SUM, comm );

#if ! defined( HAVE_ALLOCA )
    if ( hashes != NULL )
        free( hashes );
    if ( err_ranks != NULL )
        free( err_ranks );
    if ( isOK2chks != NULL )
        free( isOK2chks );
#endif

    if ( err_rank_size > 0 )
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);

    return MPI_SUCCESS;
}


/*
  The vector of (recvtype,recvcnts[]) is assumed to be known locally.
  The routine checks if (recvtype,recvcnts[]) on local process is the same as
  (sendtype,sendcnts[]) collected from all the other processes.
*/
int CollChk_dtype_allgatherv(MPI_Comm comm,
                             MPI_Datatype sendtype, int sendcnt,
                             MPI_Datatype recvtype, int *recvcnts,
                             int are2buffs, char *call)
{
    CollChk_hash_t  *hashes;      /* hash array for (sendtype,sendcnt) */
    CollChk_hash_t  send_hash;    /* local sender's hash value */
    CollChk_hash_t  recv_hash;    /* local receiver's hash value */
    char            err_str[COLLCHK_STD_STRLEN];
    char            rank_str[COLLCHK_SM_STRLEN];
    int             *isOK2chks;   /* boolean array, true:sendbuff=\=recvbuff */
    int             *err_ranks;   /* array of ranks that have mismatch hashes */
    int             err_rank_size;
    int             err_str_sz, str_sz;
    int             rank, size, idx;

#if defined( DEBUG )
    fprintf( stdout, "CollChk_dtype_allgatherv()\n" );
#endif

    /* get the rank and size */
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /* Allocate hash buffer memory */
#if ! defined( HAVE_ALLOCA )
    hashes    = (CollChk_hash_t *) malloc( size * sizeof(CollChk_hash_t) );
    err_ranks = (int *) malloc( size * sizeof(int) );
    isOK2chks = (int *) malloc( size * sizeof(int) );
#else
    hashes    = (CollChk_hash_t *) alloca( size * sizeof(CollChk_hash_t) );
    err_ranks = (int *) alloca( size * sizeof(int) );
    isOK2chks = (int *) alloca( size * sizeof(int) );
#endif

    CollChk_dtype_hash( sendtype, sendcnt, &send_hash );

    /* Gather other senders' datatype hashes as local hash array */
    PMPI_Allgather(&send_hash, 2, MPI_UNSIGNED, hashes, 2, MPI_UNSIGNED, comm);
    PMPI_Allgather(&are2buffs, 1, MPI_INT, isOK2chks, 1, MPI_INT, comm);

    /* Compare the local datatype hash with other senders' datatype hashes */
    /*
       The checks are more exhaustive and redundant tests on all processes,
       but matches what user expects
    */
    err_rank_size = 0;
    for ( idx = 0; idx < size; idx++ ) {
        if ( isOK2chks[idx] ) {
            CollChk_dtype_hash( recvtype, recvcnts[idx], &recv_hash );
            if ( ! CollChk_hash_equal( &recv_hash, &(hashes[idx]) ) )
                err_ranks[ err_rank_size++ ] = idx;
        }
    }

    if ( err_rank_size > 0 ) {
        str_sz = sprintf(err_str, "Inconsistent datatype signatures detected "
                                  "between local rank %d and remote ranks,",
                                  rank);
        /* all string size variables, *_sz, does not include NULL character */
        err_str_sz = str_sz;
        for ( idx = 0; idx < err_rank_size; idx++ ) {
            str_sz = sprintf(rank_str, " %d", err_ranks[idx] );
            /* -3 is reserved for "..." */
            if ( str_sz + err_str_sz < COLLCHK_STD_STRLEN-3 ) {
                strcat(err_str, rank_str);
                err_str_sz = strlen( err_str );
            }
            else {
                strcat(err_str, "..." );
                break;
            }
        }
    }
    else
        sprintf(err_str, COLLCHK_NO_ERROR_STR);

    /* Find out the total number of unequal hashes in the communicator */
    PMPI_Allreduce( &err_rank_size, &err_rank_size, 1, MPI_INT,
                    MPI_SUM, comm );

#if ! defined( HAVE_ALLOCA )
    if ( hashes != NULL )
        free( hashes );
    if ( err_ranks != NULL )
        free( err_ranks );
    if ( isOK2chks != NULL )
        free( isOK2chks );
#endif

    if ( err_rank_size > 0 )
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);

    return MPI_SUCCESS;
}


/*
  The vector of (recvtype,recvcnts[]) is assumed to be known locally.
  The routine checks if (recvtype,recvcnts[]) on local process is the same as
  (sendtype,sendcnts[]) collected from all the other processes.
*/
int CollChk_dtype_alltoallv(MPI_Comm comm,
                            MPI_Datatype sendtype, int *sendcnts,
                            MPI_Datatype recvtype, int *recvcnts,
                            char *call)
{
    CollChk_hash_t  *send_hashes;    /* hash array for (sendtype,sendcnt[]) */
    CollChk_hash_t  *hashes;         /* hash array for (sendtype,sendcnt[]) */
    CollChk_hash_t  recv_hash;       /* local receiver's hash value */
    char            err_str[COLLCHK_STD_STRLEN];
    char            rank_str[COLLCHK_SM_STRLEN];
    int             *err_ranks;
    int             err_rank_size;
    int             err_str_sz, str_sz;
    int             rank, size, idx;

#if defined( DEBUG )
    fprintf( stdout, "CollChk_dtype_alltoallv()\n" );
#endif

    /* get the rank and size */
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /* Allocate hash buffer memory */
#if ! defined( HAVE_ALLOCA )
    send_hashes = (CollChk_hash_t *) malloc( size * sizeof(CollChk_hash_t) );
    hashes      = (CollChk_hash_t *) malloc( size * sizeof(CollChk_hash_t) );
    err_ranks   = (int *) malloc( size * sizeof(int) );
#else
    send_hashes = (CollChk_hash_t *) alloca( size * sizeof(CollChk_hash_t) );
    hashes      = (CollChk_hash_t *) alloca( size * sizeof(CollChk_hash_t) );
    err_ranks   = (int *) alloca( size * sizeof(int) );
#endif

    for ( idx = 0; idx < size; idx++ )
        CollChk_dtype_hash( sendtype, sendcnts[idx], &send_hashes[idx] );

    /* Gather other senders' datatype hashes as local hash array */
    PMPI_Alltoall(send_hashes, 2, MPI_UNSIGNED, hashes, 2, MPI_UNSIGNED, comm);

    /* Compare the local datatype hash with other senders' datatype hashes */
    /*
       The checks are more exhaustive and redundant tests on all processes,
       but matches what user expects
    */
    err_rank_size = 0;
    for ( idx = 0; idx < size; idx++ ) {
        CollChk_dtype_hash( recvtype, recvcnts[idx], &recv_hash );
        if ( ! CollChk_hash_equal( &recv_hash, &(hashes[idx]) ) )
            err_ranks[ err_rank_size++ ] = idx;
    }

    if ( err_rank_size > 0 ) {
        str_sz = sprintf(err_str, "Inconsistent datatype signatures detected "
                                  "between local rank %d and remote ranks,",
                                  rank);
        /* all string size variables, *_sz, does not include NULL character */
        err_str_sz = str_sz;
        for ( idx = 0; idx < err_rank_size; idx++ ) {
            str_sz = sprintf(rank_str, " %d", err_ranks[idx] );
            /* -3 is reserved for "..." */
            if ( str_sz + err_str_sz < COLLCHK_STD_STRLEN-3 ) {
                strcat(err_str, rank_str);
                err_str_sz = strlen( err_str );
            }
            else {
                strcat(err_str, "..." );
                break;
            }
        }
    }
    else
        sprintf(err_str, COLLCHK_NO_ERROR_STR);

    /* Find out the total number of unequal hashes in the communicator */
    PMPI_Allreduce( &err_rank_size, &err_rank_size, 1, MPI_INT,
                    MPI_SUM, comm );

#if ! defined( HAVE_ALLOCA )
    if ( send_hashes != NULL )
        free( send_hashes );
    if ( hashes != NULL )
        free( hashes );
    if ( err_ranks != NULL )
        free( err_ranks );
#endif

    if ( err_rank_size > 0 )
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);

    return MPI_SUCCESS;
}


/*
  The vector of (recvtypes[],recvcnts[]) is assumed to be known locally.
  The routine checks if (recvtypes[],recvcnts[]) on local process is the same as
  (sendtype[],sendcnts[]) collected from all the other processes.
*/
int CollChk_dtype_alltoallw(MPI_Comm comm,
                            MPI_Datatype *sendtypes, int *sendcnts,
                            MPI_Datatype *recvtypes, int *recvcnts,
                            char *call)
{
    CollChk_hash_t  *send_hashes;  /* hash array for (sendtypes[],sendcnt[]) */
    CollChk_hash_t  *hashes;       /* hash array for (sendtypes[],sendcnt[]) */
    CollChk_hash_t  recv_hash;     /* local receiver's hash value */
    char            err_str[COLLCHK_STD_STRLEN];
    char            rank_str[COLLCHK_SM_STRLEN];
    int             *err_ranks;
    int             err_rank_size;
    int             err_str_sz, str_sz;
    int             rank, size, idx;

    /* get the rank and size */
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /* Allocate hash buffer memory */
#if ! defined( HAVE_ALLOCA )
    send_hashes = (CollChk_hash_t *) malloc( size * sizeof(CollChk_hash_t) );
    hashes      = (CollChk_hash_t *) malloc( size * sizeof(CollChk_hash_t) );
    err_ranks   = (int *) malloc( size * sizeof(int) );
#else
    send_hashes = (CollChk_hash_t *) alloca( size * sizeof(CollChk_hash_t) );
    hashes      = (CollChk_hash_t *) alloca( size * sizeof(CollChk_hash_t) );
    err_ranks   = (int *) alloca( size * sizeof(int) );
#endif

    for ( idx = 0; idx < size; idx++ )
        CollChk_dtype_hash( sendtypes[idx], sendcnts[idx], &send_hashes[idx] );

    /* Gather other senders' datatype hashes as local hash array */
    PMPI_Alltoall(send_hashes, 2, MPI_UNSIGNED, hashes, 2, MPI_UNSIGNED, comm);

    /* Compare the local datatype hashes with other senders' datatype hashes */
    /*
       The checks are more exhaustive and redundant tests on all processes,
       but matches what user expects
    */
    err_rank_size = 0;
    for ( idx = 0; idx < size; idx++ ) {
        CollChk_dtype_hash( recvtypes[idx], recvcnts[idx], &recv_hash );
        if ( ! CollChk_hash_equal( &recv_hash, &(hashes[idx]) ) )
            err_ranks[ err_rank_size++ ] = idx;
    }

    if ( err_rank_size > 0 ) {
        str_sz = sprintf(err_str, "Inconsistent datatype signatures detected "
                                  "between local rank %d and remote ranks,",
                                  rank);
        /* all string size variables, *_sz, does not include NULL character */
        err_str_sz = str_sz;
        for ( idx = 0; idx < err_rank_size; idx++ ) {
            str_sz = sprintf(rank_str, " %d", err_ranks[idx] );
            /* -3 is reserved for "..." */
            if ( str_sz + err_str_sz < COLLCHK_STD_STRLEN-3 ) {
                strcat(err_str, rank_str);
                err_str_sz = strlen( err_str );
            }
            else {
                strcat(err_str, "..." );
                break;
            }
        }
    }
    else
        sprintf(err_str, COLLCHK_NO_ERROR_STR);

    /* Find out the total number of unequal hashes in the communicator */
    PMPI_Allreduce( &err_rank_size, &err_rank_size, 1, MPI_INT,
                    MPI_SUM, comm );

#if ! defined( HAVE_ALLOCA )
    if ( send_hashes != NULL )
        free( send_hashes );
    if ( hashes != NULL )
        free( hashes );
    if ( err_ranks != NULL )
        free( err_ranks );
#endif

    if ( err_rank_size > 0 )
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);

    return MPI_SUCCESS;
}
