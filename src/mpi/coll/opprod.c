/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* 
 * In MPI-1, this operation is valid only for  C integer, Fortran integer,
 * floating point, and complex data items (4.9.2 Predefined reduce operations)
 */
#define MPIR_LPROD(a,b) ((a)*(b))

void MPIR_PROD ( 
    void *invec, 
    void *inoutvec, 
    int *Len, 
    MPI_Datatype *type )
{
    int i, len = *Len;
    
#ifdef HAVE_FORTRAN_BINDING
    typedef struct { 
        float re;
        float im; 
    } s_complex;

    typedef struct { 
        double re;
        double im; 
    } d_complex;
#endif

    switch (*type) {
    case MPI_INT: {
        int * restrict a = (int *)inoutvec; 
        int * restrict b = (int *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LPROD(a[i],b[i]);
        break;
    }
#ifdef HAVE_FORTRAN_BINDING
    case MPI_INTEGER: {
        MPI_Fint * restrict a = (MPI_Fint *)inoutvec; 
        MPI_Fint * restrict b = (MPI_Fint *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LPROD(a[i],b[i]);
        break;
    }
#endif
    case MPI_UNSIGNED: {
        unsigned * restrict a = (unsigned *)inoutvec; 
        unsigned * restrict b = (unsigned *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LPROD(a[i],b[i]);
        break;
    }
    case MPI_LONG: {
        long * restrict a = (long *)inoutvec; 
        long * restrict b = (long *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LPROD(a[i],b[i]);
        break;
    }
#if defined(HAVE_LONG_LONG_INT)
    case MPI_LONG_LONG: {
	/* case MPI_LONG_LONG_INT: defined to be the same as long_long */
        long long * restrict a = (long long *)inoutvec; 
        long long * restrict b = (long long *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LPROD(a[i],b[i]);
        break;
    }
#endif
    case MPI_UNSIGNED_LONG: {
        unsigned long * restrict a = (unsigned long *)inoutvec; 
        unsigned long * restrict b = (unsigned long *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LPROD(a[i],b[i]);
        break;
    }
    case MPI_SHORT: {
        short * restrict a = (short *)inoutvec; 
        short * restrict b = (short *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LPROD(a[i],b[i]);
        break;
    }
    case MPI_UNSIGNED_SHORT: {
        unsigned short * restrict a = (unsigned short *)inoutvec; 
        unsigned short * restrict b = (unsigned short *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LPROD(a[i],b[i]);
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
            a[i] = MPIR_LPROD(a[i],b[i]);
        break;
    }
    case MPI_UNSIGNED_CHAR: {
        unsigned char * restrict a = (unsigned char *)inoutvec; 
        unsigned char * restrict b = (unsigned char *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LPROD(a[i],b[i]);
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
            a[i] = MPIR_LPROD(a[i],b[i]);
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
            a[i] = MPIR_LPROD(a[i],b[i]);
        break;
    }
#if defined(HAVE_LONG_DOUBLE)
    case MPI_LONG_DOUBLE: {
        long double * restrict a = (long double *)inoutvec; 
        long double * restrict b = (long double *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LPROD(a[i],b[i]);
        break;
    }
#endif
#ifdef HAVE_FORTRAN_BINDING
    case MPI_COMPLEX: {
        s_complex * restrict a = (s_complex *)inoutvec; 
        s_complex * restrict b = (s_complex *)invec;
        for ( i=0; i<len; i++ ) {
            s_complex c;
            c.re = a[i].re; c.im = a[i].im;
            a[i].re = c.re*b[i].re - c.im*b[i].im;
            a[i].im = c.im*b[i].re + c.re*b[i].im;
        }
        break;
    }
    case MPI_DOUBLE_COMPLEX: {
        d_complex * restrict a = (d_complex *)inoutvec; 
        d_complex * restrict b = (d_complex *)invec;
        for ( i=0; i<len; i++ ) {
            d_complex c;
            c.re = a[i].re; c.im = a[i].im;
            a[i].re = c.re*b[i].re - c.im*b[i].im;
            a[i].im = c.im*b[i].re + c.re*b[i].im;
        }
        break;
    }
#endif
    default: {
        MPICH_PerThread_t *p;
        MPID_GetPerThread(p);
        p->op_errno = MPIR_Err_create_code( MPI_ERR_OP, "**opundefined","**opundefined %s", "MPI_PROD" );
        break;
    }
    }
}

