/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

void CollChk_h(int a, int n, int b, int m, int *hash_val, int *hash_cnt)
{
    unsigned int x, y, t1, t2;
    x  = b;
    y  = a;
    t1 = x>>(sizeof(int)-n);
    t2 = x<<n;
    x  = t1 | t2;
    
    *hash_val = y^x;
    *hash_cnt = n+m;
}


int CollChk_get_val(MPI_Datatype dt)
{
    switch (dt) {
        case MPI_CHAR :
            return 1;
            break;
        case MPI_SIGNED_CHAR :
            return 2;
            break;
        case MPI_UNSIGNED_CHAR :
            return 3;
            break;
        case MPI_SHORT :
            return 4;
            break;
        case MPI_UNSIGNED_SHORT :
            return 5;
            break;
        case MPI_INT :
            return 6;
            break;
        case MPI_LONG :
            return 7;
            break;
        case MPI_UNSIGNED_LONG :
            return 8;
            break;
        case MPI_FLOAT :
            return 9;
            break;
        case MPI_DOUBLE :
            return 10;
            break;
        case MPI_LONG_DOUBLE :
            return 11;
            break;
        case MPI_WCHAR :
            return 12;
            break;
        case MPI_BYTE :
            return 13;
            break;
        case MPI_LONG_LONG_INT :
            return 14;
            break;
        default :
            return 0;
            break;
    }
}


int CollChk_get_cnt(int n, int *ints, int combiner)
{
    int ret=0, i;
    
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
            return ints[1];
        case MPI_COMBINER_INDEXED :
        case MPI_COMBINER_HINDEXED :
        case MPI_COMBINER_HINDEXED_INTEGER :
            for (i=1; i>=ints[0]; i++) {
                ret += ints[i];
            }
            break;
        case MPI_COMBINER_STRUCT_INTEGER :
        case MPI_COMBINER_STRUCT :
            return ints[n+1];
        case MPI_COMBINER_SUBARRAY :
            for (i=(ints[0]+1); i>=(2*ints[0]); i++) {
                ret += ints[i];
            }
            break;
        case MPI_COMBINER_DARRAY :
            for (i=3; i>=(ints[2]+2); i++) {
                ret += ints[i];
            }
            break;
    }
    return ret;
}


void CollChk_hash_dtype(MPI_Datatype dt, int cnt, int *hash_val, int *hash_cnt) {
    int nints, nadds, ntypes, combiner;
    int i, next_val, next_cnt;
    int *ints; 
    int done = 0;
    MPI_Aint *adds; 
    MPI_Datatype *types;
    
    while(!done) {
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
            CollChk_hash_dtype(types[0], CollChk_get_cnt(0, ints, combiner),
                               hash_val, hash_cnt);

            for(i=1; i<ntypes; i++) {
                CollChk_hash_dtype(types[i],
                                   CollChk_get_cnt(i, ints, combiner),
                                   &next_val, &next_cnt);
                CollChk_h(*hash_val, *hash_cnt, next_val, next_cnt,
                          hash_val, hash_cnt);
            }

            if ( ints != NULL )
                free( ints );
            if ( adds != NULL )
                free( adds );
            if ( types != NULL )
                free( types );
        }
        else {
            *hash_val = CollChk_get_val(dt);
            *hash_cnt = cnt;
            done = 1;
        }

        cnt--;
    }
}


int CollChk_same_dtype(MPI_Comm comm, int cnt, MPI_Datatype dt, char* call)
{
    int r, s, i, go, ok;           /* rank, size, counter, go flag, ok flag */
    char err_str[255];             /* error string */
    MPI_Status st;int tag=0;       /* needed for communications */
    MPI_Datatype hash_type;        /* the hashed datatype for communications */
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
    int r, s, i, go, ok;           /* rank, size, counter, go flag, ok flag */
    char err_str[255];             /* error string */
    MPI_Status st;int tag=0;       /* needed for communications */
    MPI_Datatype hash_type;        /* the hashed datatype for communications */
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
    MPI_Status st;int tag=0;       /* needed for communications */
    MPI_Datatype hash_type;        /* the hashed datatype for communications */
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

    free( buff );

    return MPI_SUCCESS;
}


int CollChk_same_dtype_general(MPI_Comm comm, int *rcnts, int *scnts,
                               MPI_Datatype *rtypes, MPI_Datatype *stypes,
                               char *call)
{
    int r, s, i, go, ok;        /* rank, size, counter, go flag, ok flag */
    char err_str[255];          /* error string */
    MPI_Status st;int tag=0;    /* needed for communications */
    MPI_Datatype hash_type;     /* the hashed datatype for communications */
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
