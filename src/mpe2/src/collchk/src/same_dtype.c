/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

void CollChk_hash_add(unsigned int alpha, unsigned int n,
                      unsigned int beta, unsigned int m,
                      unsigned int *hash_val, unsigned int *hash_cnt)
{
    unsigned int t1, t2, cirleft;
    // Doing circular left shift of beta and save it in x
    t1 = beta >> (sizeof(unsigned int)-n);
    t2 = beta << n;
    cirleft  = t1 | t2;
    
    *hash_val = alpha^cirleft;
    *hash_cnt = n+m;
}


int CollChk_get_val(MPI_Datatype dt)
{
    switch (dt) {
        case MPI_CHAR :
            return 1;
        case MPI_SIGNED_CHAR :
            return 2;
        case MPI_UNSIGNED_CHAR :
            return 3;
        case MPI_SHORT :
            return 4;
        case MPI_UNSIGNED_SHORT :
            return 5;
        case MPI_INT :
            return 6;
        case MPI_LONG :
            return 7;
        case MPI_UNSIGNED_LONG :
            return 8;
        case MPI_FLOAT :
            return 9;
        case MPI_DOUBLE :
            return 10;
        case MPI_LONG_DOUBLE :
            return 11;
        case MPI_WCHAR :
            return 12;
        case MPI_BYTE :
            return 13;
        case MPI_LONG_LONG_INT :
            return 14;
        default :
            return 0;
    }
}


int CollChk_get_cnt(int idx, int *ints, int combiner)
{
    int ii, tot_cnt;
    int dim_A, dim_B;
    
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
            tot_cnt = 0;
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
            tot_cnt = 0;
            for ( ii=dim_A; ii<=dim_B; ii++ ) {
                tot_cnt += ints[ ii ];
            }
            return tot_cnt;
        case MPI_COMBINER_DARRAY :
            tot_cnt = 0;
            for ( ii=3; ii<=ints[2]+2; ii++ ) {
                tot_cnt += ints[ ii ];
            }
            return tot_cnt;
    }
    return tot_cnt;
}


void CollChk_hash_dtype(MPI_Datatype dt, int cnt,
                        unsigned int *hash_val, unsigned int *hash_cnt) {
    int nints, nadds, ntypes, combiner;
    int *ints; 
    MPI_Aint *adds; 
    MPI_Datatype *types;
    unsigned int curr_val, curr_cnt;
    unsigned int next_val, next_cnt;
    int type_cnt;
    int ii;
    
    /*  Don't know if this makes sense or not */
    if ( cnt <= 0 ) {
        *hash_val = 0;
        *hash_cnt = 0;
        return;
    }

    MPI_Type_get_envelope(dt, &nints, &nadds, &ntypes, &combiner);
    if (combiner != MPI_COMBINER_NAMED) {
        ints = NULL;
        if ( nints > 0 )
            ints = (int *) (malloc(nints * sizeof(int))); 
        adds = NULL;
        if ( nadds > 0 )
            adds = (MPI_Aint *) (malloc(nadds * sizeof(MPI_Aint))); 
        types = NULL;
        if ( ntypes > 0 )
            types = (MPI_Datatype *) malloc(ntypes * sizeof(MPI_Datatype));

        MPI_Type_get_contents(dt, nints, nadds, ntypes, ints, adds, types);
        type_cnt = CollChk_get_cnt(0, ints, combiner);
        CollChk_hash_dtype(types[0], type_cnt, &curr_val, &curr_cnt);

        /*
            ntypes > 1 only for MPI_COMBINER_STRUCT(_INTEGER)
        */
        for( ii=1; ii < ntypes; ii++) {
            type_cnt = CollChk_get_cnt(ii, ints, combiner); 
            CollChk_hash_dtype(types[ii], type_cnt, &next_val, &next_cnt);
            CollChk_hash_add(curr_val, curr_cnt, next_val, next_cnt,
                             &curr_val, &curr_cnt);
        }

        if ( ints != NULL )
            free( ints );
        if ( adds != NULL )
            free( adds );
        if ( types != NULL )
            free( types );
    }
    else {
        curr_val = CollChk_get_val(dt);
        curr_cnt = 1;
    }

    *hash_val = curr_val;
    *hash_cnt = curr_cnt;
    for ( ii=1; ii < cnt; ii++ ) {
        CollChk_hash_add(*hash_val, *hash_cnt, curr_val, curr_cnt,
                         hash_val, hash_cnt);
    }
}


int CollChk_same_dtype(MPI_Comm comm, int cnt, MPI_Datatype dt, char* call)
{
    int r, s, i, go, ok;           /* rank, size, counter, go flag, ok flag */
    char err_str[255];             /* error string */
    MPI_Status st;int tag=0;       /* needed for communications */
    CollChk_hash_struct hs, buff;  /* local hash value, global hash value */

    /* get the rank and size */
    MPI_Comm_rank(comm, &r);
    MPI_Comm_size(comm, &s);

    /* get the hash values */
    CollChk_hash_dtype(dt, cnt, &(hs.hash_val), &(hs.hash_cnt));

    /* initialize the error string */
    sprintf(err_str, "no error");

    if (r == 0) {
        buff = hs;
        /* send 0s datatype hash to all other processes */
        PMPI_Bcast(&buff, 2, MPI_INT, 0, comm);

        /* check if all processes are ok to continue */
        go = 1; /* set the go flag */
        for (i=1; i<s; i++) {
            MPI_Recv(&ok, 1, MPI_INT, i, tag, comm, &st);
            /* if the process is not ok unset the go flag */
            if (!ok) go = 0;
        }

        /* broadcast the go flag to the other processes */
        PMPI_Bcast(&go, 1, MPI_INT, 0, comm);
    }
    else {
        /* get the datatype hash from 0 */
        PMPI_Bcast(&buff, 2, MPI_INT, 0, comm);

        /* check the hash from the local hash */
        if ((buff.hash_val != hs.hash_val) || (buff.hash_cnt != hs.hash_cnt)) {
            /* at this point the datatype parameter is inconsistant */
            /* print an error message and send an unset ok flag to 0 */
            ok = 0;
            sprintf(err_str, "Datatype Signature used is Inconsistent with "
                             "Rank 0s.\n");
            MPI_Send(&ok, 1, MPI_INT, 0, tag, comm);
        }
        else {
            /* at this point the datatype parameter is consistant  */
            /* send a set ok flag to 0 */
            ok = 1;
            MPI_Send(&ok, 1, MPI_INT, 0, tag, comm);
        }

        /* recieve the go flag from 0 */
        PMPI_Bcast(&go, 1, MPI_INT, 0, comm);
    }

    /* if the go flag is not set exit else return */
    if (!go) {
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);
    }

    return MPI_SUCCESS;
}


int CollChk_same_dtype_vector(MPI_Comm comm, int root, int cnt,
                              int *rootcnts, MPI_Datatype dt, char *call)
{
    int r, s, i, go;               /* rank, size, counter, go flag */
    char err_str[255];             /* error string */
    CollChk_hash_struct hs, *buff; /* local hash value, global hash value */
    CollChk_hash_struct curr;      /* the current hash being checked */

    /* get the rank and size */
    MPI_Comm_rank(comm, &r);
    MPI_Comm_size(comm, &s);

    /* get the hash values */
    CollChk_hash_dtype(dt, cnt, &(hs.hash_val), &(hs.hash_cnt));
    /* allocate buffer memory */
    buff = (CollChk_hash_struct *) malloc(s*sizeof(CollChk_hash_struct));
    /* initialize the error string */
    sprintf(err_str, "no error");

    if (r == root) {
        /* gather the signatures to the root process */
        PMPI_Gather(&buff, 2, MPI_INT, &hs, 2, MPI_INT, root, comm);

        /* check thier values */
        go = 1;
        for (i=1; i<s; i++) {
            CollChk_hash_dtype(dt, rootcnts[i],
                               &(curr.hash_val), &(curr.hash_cnt));
            if(    (curr.hash_val != buff[i].hash_val)
                || (curr.hash_cnt != buff[i].hash_cnt) ) {
                go = 0;
                sprintf(err_str, "The Data Type Signature on process %d "
                                 "is not consistant with the root's (%d)",
                                 i, root);
                break;
            }
        }

        /* broadcast the go flag to the other processes */
        PMPI_Bcast(&go, 1, MPI_INT, 0, comm);
    }
    else {
        /* send the local hash value to the root process */
        PMPI_Gather(&buff, 2, MPI_INT, &hs, 2, MPI_INT, root, comm);

        /* recieve the go flag from the root process */
        PMPI_Bcast(&go, 1, MPI_INT, 0, comm);
    }

    /* if the go flag is not set exit else return */
    if (!go) {
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);
    }

    free( buff );

    return MPI_SUCCESS;
}


int CollChk_same_dtype_vector2(MPI_Comm comm, int *cnts,
                               MPI_Datatype dt, char *call)
{
    int r, s, i, go, ok;           /* rank, size, counter, go flag, ok flag */
    char err_str[255];             /* error string */
    CollChk_hash_struct hs, *buff; /* local hash value, global hash value */
    CollChk_hash_struct curr;      /* the current hash being checked */

    /* get the rank and size */
    MPI_Comm_rank(comm, &r);
    MPI_Comm_size(comm, &s);

    /* get the hash values */
    CollChk_hash_dtype(dt, cnts[r], &(hs.hash_val), &(hs.hash_cnt));
    /* allocate buffer memory */
    buff = (CollChk_hash_struct *) malloc(s*sizeof(CollChk_hash_struct));
    /* initialize the error string */
    sprintf(err_str, "no error");

    /* gather the signatures to the processes */
    PMPI_Allgather(&buff, 2, MPI_INT, &hs, 2, MPI_INT, comm);

    /* check the values */
    ok = 1;
    for (i=0; i<s; i++) {
        CollChk_hash_dtype(dt, cnts[i], &(curr.hash_val), &(curr.hash_cnt));
        if(    (curr.hash_val != buff[i].hash_val)
            || (curr.hash_cnt != buff[i].hash_cnt) ) {
            sprintf(err_str, "The Data Type Signature on process %d "
                             "is not consistent with process %d", i, r);
            ok = 0;
            break;
        }
    }

    /* broadcast the go flag to the other processes */
    PMPI_Bcast(&go, 1, MPI_INT, 0, comm);

    /* if the go flag is not set exit else return */
    if (!go) {
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);
    }

    free( buff );

    return MPI_SUCCESS;
}


int CollChk_same_dtype_general(MPI_Comm comm, int *rcnts, int *scnts,
                               MPI_Datatype *rtypes, MPI_Datatype *stypes,
                               char *call)
{
    int r, s, i, go, ok;        /* rank, size, counter, go flag, ok flag */
    char err_str[255];          /* error string */
    CollChk_hash_struct rhs, shs, *rbuff, *sbuff, curr;

    /* get the rank and size */
    MPI_Comm_rank(comm, &r);
    MPI_Comm_size(comm, &s);

    /* get the hash values */
    CollChk_hash_dtype(rtypes[r], rcnts[r], &(rhs.hash_val), &(rhs.hash_cnt));
    CollChk_hash_dtype(stypes[r], scnts[r], &(shs.hash_val), &(shs.hash_cnt));

    /* allocate buffer memory */
    rbuff = (CollChk_hash_struct *) malloc(s*sizeof(CollChk_hash_struct));
    sbuff = (CollChk_hash_struct *) malloc(s*sizeof(CollChk_hash_struct));
    /* initialize the error string */
    sprintf(err_str, "no error");

    /* gather the signatures to the processes */
    PMPI_Allgather(&rbuff, 2, MPI_INT, &rhs, 2, MPI_INT, comm);
    PMPI_Allgather(&sbuff, 2, MPI_INT, &shs, 2, MPI_INT, comm);

    /* check the values */
    ok = 1;
    for (i=0; i<s; i++) {
        CollChk_hash_dtype(rtypes[i], rcnts[i],
                           &(curr.hash_val), &(curr.hash_cnt));
        if(    (curr.hash_val != sbuff[i].hash_val)
            || (curr.hash_cnt != sbuff[i].hash_cnt)) {
            sprintf(err_str, "The Data Type Signature on process %d "
                             "is not consistant with process %d", i, r);
            ok = 0;
            break;
        }
        CollChk_hash_dtype( stypes[i], scnts[i],
                            &(curr.hash_val), &(curr.hash_cnt) );
        if(    (curr.hash_val != rbuff[i].hash_val)
            || (curr.hash_cnt != rbuff[i].hash_cnt) ) {
            sprintf(err_str, "The Data Type Signature on process %d "
                             "is not consistant with process %d", i, r);
            ok = 0;
            break;
        }
    }

    /* broadcast the go flag to the other processes */
    PMPI_Bcast(&go, 1, MPI_INT, 0, comm);

    /* if the go flag is not set exit else return */
    if (!go) {
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);
    }

    free( rbuff );
    free( sbuff );

    return MPI_SUCCESS;
}
