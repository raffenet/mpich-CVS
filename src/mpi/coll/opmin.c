/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* 
 * In MPI-1, this operation is valid only for  C integer, Fortran integer,
 * and floating point data items (4.9.2 Predefined reduce operations)
 */
void MPIR_MINF ( 
	void *invec, 
	void *inoutvec, 
	int *Len, 
	MPI_Datatype *type )
{
    static const char FCNAME[] = "MPIR_MINF";
    int i, len = *Len;
    
    switch (*type) {
    case MPI_INT: {
        int * restrict a = (int *)inoutvec; 
        int * restrict b = (int *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_MIN(a[i],b[i]);
        break;
    }
#ifdef HAVE_FORTRAN_BINDING
    case MPI_INTEGER: {
        MPI_Fint * restrict a = (MPI_Fint *)inoutvec; 
        MPI_Fint * restrict b = (MPI_Fint *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_MIN(a[i],b[i]);
        break;
    }
#endif
    case MPI_UNSIGNED: {
        unsigned * restrict a = (unsigned *)inoutvec; 
        unsigned * restrict b = (unsigned *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_MIN(a[i],b[i]);
        break;
    }
    case MPI_LONG: {
        long * restrict a = (long *)inoutvec; 
        long * restrict b = (long *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_MIN(a[i],b[i]);
        break;
    }
#if defined(HAVE_LONG_LONG_INT)
    case MPI_LONG_LONG: {
	/* case MPI_LONG_LONG_INT: defined to be the same as long_long */
        long long * restrict a = (long long *)inoutvec; 
        long long * restrict b = (long long *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_MIN(a[i],b[i]);
        break;
    }
#endif
    case MPI_UNSIGNED_LONG: {
        unsigned long * restrict a = (unsigned long *)inoutvec; 
        unsigned long * restrict b = (unsigned long *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_MIN(a[i],b[i]);
        break;
    }
    case MPI_SHORT: {
        short * restrict a = (short *)inoutvec; 
        short * restrict b = (short *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_MIN(a[i],b[i]);
        break;
    }
    case MPI_UNSIGNED_SHORT: {
        unsigned short * restrict a = (unsigned short *)inoutvec; 
        unsigned short * restrict b = (unsigned short *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_MIN(a[i],b[i]);
        break;
    }
    case MPI_CHAR: 
#ifdef HAVE_FORTRAN_BINDING
    case MPI_CHARACTER: 
#endif
    {
        char * restrict a = (char *)inoutvec; 
        char * restrict b = (char *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_MIN(a[i],b[i]);
        break;
    }
    case MPI_UNSIGNED_CHAR: {
        unsigned char * restrict a = (unsigned char *)inoutvec; 
        unsigned char * restrict b = (unsigned char *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_MIN(a[i],b[i]);
        break;
    }
    case MPI_FLOAT: 
#ifdef HAVE_FORTRAN_BINDING
    case MPI_REAL: 
#endif
    {
        float * restrict a = (float *)inoutvec; 
        float * restrict b = (float *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_MIN(a[i],b[i]);
        break;
    }
    case MPI_DOUBLE: 
#ifdef HAVE_FORTRAN_BINDING
    case MPI_DOUBLE_PRECISION: 
#endif
    {
        double * restrict a = (double *)inoutvec; 
        double * restrict b = (double *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_MIN(a[i],b[i]);
        break;
    }
#if defined(HAVE_LONG_DOUBLE)
    case MPI_LONG_DOUBLE: {
        long double * restrict a = (long double *)inoutvec; 
        long double * restrict b = (long double *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_MIN(a[i],b[i]);
        break;
    }
#endif
    default: {
        MPICH_PerThread_t *p;
        MPID_GetPerThread(p);
        p->op_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, MPI_ERR_OP, "**opundefined","**opundefined %s", "MPI_MIN" );
        break;
    }
    }
}


int MPIR_MINF_check_dtype ( MPI_Datatype type )
{
    static const char FCNAME[] = "MPIR_MINF_check_dtype";
    
    switch (type) {
    case MPI_INT: 
#ifdef HAVE_FORTRAN_BINDING
    case MPI_INTEGER: 
#endif
    case MPI_UNSIGNED: 
    case MPI_LONG: 
#if defined(HAVE_LONG_LONG_INT)
    case MPI_LONG_LONG: 
#endif
    case MPI_UNSIGNED_LONG: 
    case MPI_SHORT: 
    case MPI_UNSIGNED_SHORT: 
    case MPI_CHAR: 
#ifdef HAVE_FORTRAN_BINDING
    case MPI_CHARACTER: 
#endif
    case MPI_UNSIGNED_CHAR: 
    case MPI_FLOAT: 
#ifdef HAVE_FORTRAN_BINDING
    case MPI_REAL: 
#endif
    case MPI_DOUBLE: 
#ifdef HAVE_FORTRAN_BINDING
    case MPI_DOUBLE_PRECISION: 
#endif
#if defined(HAVE_LONG_DOUBLE)
    case MPI_LONG_DOUBLE: 
#endif
        return MPI_SUCCESS;
    default: 
        return MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, MPI_ERR_OP, "**opundefined","**opundefined %s", "MPI_MIN" );
    }
}
