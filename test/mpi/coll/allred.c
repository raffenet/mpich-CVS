#include <math.h>
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

/* Run this test on 2, 4, 6, 8, or 10 processes only */

int count, errcnt = 0, gerr = 0, size, rank;
MPI_Comm comm;

void a1()
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = i*size; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_SUM\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b1()
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = i*size; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_SUM, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_SUM\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c1()
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = i*size; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_SUM, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_SUM\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d1()
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = i*size; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_SUM, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_SUM\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e1()
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = i*size; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_SUM, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_SUM\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f1()
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = i*size; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_SUM, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_SUM\n", rank );
    free( in );
    free( out );
    free( sol );
}


void g1()
{
    float *in, *out, *sol;
    int  i, fnderr=0;
    in = (float *)malloc( count * sizeof(float) );
    out = (float *)malloc( count * sizeof(float) );
    sol = (float *)malloc( count * sizeof(float) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = i*size; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_FLOAT, MPI_SUM, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_FLOAT and op MPI_SUM\n", rank );
    free( in );
    free( out );
    free( sol );
}


void h1()
{
    double *in, *out, *sol;
    int  i, fnderr=0;
    in = (double *)malloc( count * sizeof(double) );
    out = (double *)malloc( count * sizeof(double) );
    sol = (double *)malloc( count * sizeof(double) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = i*size; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_DOUBLE, MPI_SUM, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_DOUBLE and op MPI_SUM\n", rank );
    free( in );
    free( out );
    free( sol );
}


void a2()
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = (i > 0) ? (int)(pow((double)(i),(double)size)+0.1) : 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_PROD, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_PROD\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b2()
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = (i > 0) ? (int)(pow((double)(i),(double)size)+0.1) : 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_PROD, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_PROD\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c2()
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = (i > 0) ? (int)(pow((double)(i),(double)size)+0.1) : 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_PROD, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_PROD\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d2()
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = (i > 0) ? (int)(pow((double)(i),(double)size)+0.1) : 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_PROD, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_PROD\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e2()
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = (i > 0) ? (int)(pow((double)(i),(double)size)+0.1) : 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_PROD, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_PROD\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f2()
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = (i > 0) ? (int)(pow((double)(i),(double)size)+0.1) : 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_PROD, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_PROD\n", rank );
    free( in );
    free( out );
    free( sol );
}


void g2()
{
    float *in, *out, *sol;
    int  i, fnderr=0;
    in = (float *)malloc( count * sizeof(float) );
    out = (float *)malloc( count * sizeof(float) );
    sol = (float *)malloc( count * sizeof(float) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = (i > 0) ? (int)(pow((double)(i),(double)size)+0.1) : 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_FLOAT, MPI_PROD, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_FLOAT and op MPI_PROD\n", rank );
    free( in );
    free( out );
    free( sol );
}


void h2()
{
    double *in, *out, *sol;
    int  i, fnderr=0;
    in = (double *)malloc( count * sizeof(double) );
    out = (double *)malloc( count * sizeof(double) );
    sol = (double *)malloc( count * sizeof(double) );
    for (i=0; i<count; i++) { *(in + i) = i; *(sol + i) = (i > 0) ? (int)(pow((double)(i),(double)size)+0.1) : 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_DOUBLE, MPI_PROD, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_DOUBLE and op MPI_PROD\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a3()
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = (size - 1 + i); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_MAX, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_MAX\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b3()
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = (size - 1 + i); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_MAX, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_MAX\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c3()
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = (size - 1 + i); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_MAX, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_MAX\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d3()
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = (size - 1 + i); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_MAX, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_MAX\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e3()
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = (size - 1 + i); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_MAX, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_MAX\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f3()
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = (size - 1 + i); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_MAX, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_MAX\n", rank );
    free( in );
    free( out );
    free( sol );
}


void g3()
{
    float *in, *out, *sol;
    int  i, fnderr=0;
    in = (float *)malloc( count * sizeof(float) );
    out = (float *)malloc( count * sizeof(float) );
    sol = (float *)malloc( count * sizeof(float) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = (size - 1 + i); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_FLOAT, MPI_MAX, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_FLOAT and op MPI_MAX\n", rank );
    free( in );
    free( out );
    free( sol );
}


void h3()
{
    double *in, *out, *sol;
    int  i, fnderr=0;
    in = (double *)malloc( count * sizeof(double) );
    out = (double *)malloc( count * sizeof(double) );
    sol = (double *)malloc( count * sizeof(double) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = (size - 1 + i); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_DOUBLE, MPI_MAX, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_DOUBLE and op MPI_MAX\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a4()
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_MIN, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_MIN\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b4()
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_MIN, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_MIN\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c4()
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_MIN, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_MIN\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d4()
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_MIN, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_MIN\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e4()
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_MIN, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_MIN\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f4()
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_MIN, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_MIN\n", rank );
    free( in );
    free( out );
    free( sol );
}


void g4()
{
    float *in, *out, *sol;
    int  i, fnderr=0;
    in = (float *)malloc( count * sizeof(float) );
    out = (float *)malloc( count * sizeof(float) );
    sol = (float *)malloc( count * sizeof(float) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_FLOAT, MPI_MIN, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_FLOAT and op MPI_MIN\n", rank );
    free( in );
    free( out );
    free( sol );
}


void h4()
{
    double *in, *out, *sol;
    int  i, fnderr=0;
    in = (double *)malloc( count * sizeof(double) );
    out = (double *)malloc( count * sizeof(double) );
    sol = (double *)malloc( count * sizeof(double) );
    for (i=0; i<count; i++) { *(in + i) = (rank + i); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_DOUBLE, MPI_MIN, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_DOUBLE and op MPI_MIN\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a5()
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b5()
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c5()
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d5()
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e5()
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f5()
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a6()
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b6()
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c6()
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d6()
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e6()
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f6()
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_LOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_LOR\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a7()
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_LXOR\n", rank);
    free( in );
    free( out );
    free( sol );
}


void b7()
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c7()
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d7()
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e7()
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f7()
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1); *(sol + i) = (size > 1); 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a8()
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b8()
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c8()
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d8()
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e8()
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f8()
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a9()
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}

void b9()
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c9()
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d9()
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e9()
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f9()
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_LXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_LXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a10()
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b10()
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c10()
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d10()
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e10()
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f10()
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = (rank & 0x1); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a11()
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 1; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b11()
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 1; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c11()
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 1; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d11()
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 1; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e11()
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 1; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f11()
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = 1; *(sol + i) = 1; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_LAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_LAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void a12()
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = rank & 0x3; *(sol + i) = (size < 3) ? size - 1 : 0x3; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_BOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_BOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b12()
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = rank & 0x3; *(sol + i) = (size < 3) ? size - 1 : 0x3; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_BOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_BOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c12()
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = rank & 0x3; *(sol + i) = (size < 3) ? size - 1 : 0x3; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_BOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_BOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d12()
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = rank & 0x3; *(sol + i) = (size < 3) ? size - 1 : 0x3; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_BOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_BOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e12()
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = rank & 0x3; *(sol + i) = (size < 3) ? size - 1 : 0x3; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_BOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_BOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f12()
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = rank & 0x3; *(sol + i) = (size < 3) ? size - 1 : 0x3; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_BOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_BOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void g12()
{
    unsigned char *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned char *)malloc( count * sizeof(unsigned char) );
    out = (unsigned char *)malloc( count * sizeof(unsigned char) );
    sol = (unsigned char *)malloc( count * sizeof(unsigned char) );
    for (i=0; i<count; i++) { *(in + i) = rank & 0x3; *(sol + i) = (size < 3) ? size - 1 : 0x3; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_BYTE, MPI_BOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_BYTE and op MPI_BOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void a13()
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : ~0); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b13()
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : ~0); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c13()
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : ~0); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d13()
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : ~0); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e13()
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : ~0); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f13()
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : ~0); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void g13()
{
    unsigned char *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned char *)malloc( count * sizeof(unsigned char) );
    out = (unsigned char *)malloc( count * sizeof(unsigned char) );
    sol = (unsigned char *)malloc( count * sizeof(unsigned char) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : ~0); *(sol + i) = i; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_BYTE, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_BYTE and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void a14()
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : 0); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b14()
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : 0); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c14()
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : 0); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d14()
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : 0); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e14()
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : 0); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f14()
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = (rank == size-1 ? i : 0); *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_BAND, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_BAND\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a15()
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1)*0xf0 ; *(sol + i) = (size > 1)*0xf0 ; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b15()
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1)*0xf0 ; *(sol + i) = (size > 1)*0xf0 ; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c15()
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1)*0xf0 ; *(sol + i) = (size > 1)*0xf0 ; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d15()
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1)*0xf0 ; *(sol + i) = (size > 1)*0xf0 ; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e15()
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1)*0xf0 ; *(sol + i) = (size > 1)*0xf0 ; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f15()
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = (rank == 1)*0xf0 ; *(sol + i) = (size > 1)*0xf0 ; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a16()
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b16()
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c16()
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d16()
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e16()
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f16()
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = 0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void a17()
{
    int *in, *out, *sol;
    int  i, fnderr=0;
    in = (int *)malloc( count * sizeof(int) );
    out = (int *)malloc( count * sizeof(int) );
    sol = (int *)malloc( count * sizeof(int) );
    for (i=0; i<count; i++) { *(in + i) = ~0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_INT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_INT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void b17()
{
    long *in, *out, *sol;
    int  i, fnderr=0;
    in = (long *)malloc( count * sizeof(long) );
    out = (long *)malloc( count * sizeof(long) );
    sol = (long *)malloc( count * sizeof(long) );
    for (i=0; i<count; i++) { *(in + i) = ~0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_LONG, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void c17()
{
    short *in, *out, *sol;
    int  i, fnderr=0;
    in = (short *)malloc( count * sizeof(short) );
    out = (short *)malloc( count * sizeof(short) );
    sol = (short *)malloc( count * sizeof(short) );
    for (i=0; i<count; i++) { *(in + i) = ~0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_SHORT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void d17()
{
    unsigned short *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned short *)malloc( count * sizeof(unsigned short) );
    out = (unsigned short *)malloc( count * sizeof(unsigned short) );
    sol = (unsigned short *)malloc( count * sizeof(unsigned short) );
    for (i=0; i<count; i++) { *(in + i) = ~0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_SHORT, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_SHORT and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void e17()
{
    unsigned *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned *)malloc( count * sizeof(unsigned) );
    out = (unsigned *)malloc( count * sizeof(unsigned) );
    sol = (unsigned *)malloc( count * sizeof(unsigned) );
    for (i=0; i<count; i++) { *(in + i) = ~0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}


void f17()
{
    unsigned long *in, *out, *sol;
    int  i, fnderr=0;
    in = (unsigned long *)malloc( count * sizeof(unsigned long) );
    out = (unsigned long *)malloc( count * sizeof(unsigned long) );
    sol = (unsigned long *)malloc( count * sizeof(unsigned long) );
    for (i=0; i<count; i++) { *(in + i) = ~0; *(sol + i) = 0; 
    *(out + i) = 0; }
    MPI_Allreduce( in, out, count, MPI_UNSIGNED_LONG, MPI_BXOR, comm );
    for (i=0; i<count; i++) { if (*(out + i) != *(sol + i)) {errcnt++; fnderr++;}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_UNSIGNED_LONG and op MPI_BXOR\n", rank );
    free( in );
    free( out );
    free( sol );
}



void a18()
{
    struct int_test { int a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct int_test *)malloc( count * sizeof(struct int_test) );
    out = (struct int_test *)malloc( count * sizeof(struct int_test) );
    sol = (struct int_test *)malloc( count * sizeof(struct int_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = (size - 1 + i); (sol + i)->b = (size-1);
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_2INT, MPI_MAXLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_2INT and op MPI_MAXLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


void b18()
{
    struct long_test { long a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct long_test *)malloc( count * sizeof(struct long_test) );
    out = (struct long_test *)malloc( count * sizeof(struct long_test) );
    sol = (struct long_test *)malloc( count * sizeof(struct long_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = (size - 1 + i); (sol + i)->b = (size-1);
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_LONG_INT, MPI_MAXLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG_INT and op MPI_MAXLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


void c18()
{
    struct short_test { short a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct short_test *)malloc( count * sizeof(struct short_test) );
    out = (struct short_test *)malloc( count * sizeof(struct short_test) );
    sol = (struct short_test *)malloc( count * sizeof(struct short_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = (size - 1 + i); (sol + i)->b = (size-1);
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_SHORT_INT, MPI_MAXLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT_INT and op MPI_MAXLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


void d18()
{
    struct float_test { float a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct float_test *)malloc( count * sizeof(struct float_test) );
    out = (struct float_test *)malloc( count * sizeof(struct float_test) );
    sol = (struct float_test *)malloc( count * sizeof(struct float_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = (size - 1 + i); (sol + i)->b = (size-1);
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_FLOAT_INT, MPI_MAXLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_FLOAT_INT and op MPI_MAXLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


void e18()
{
    struct double_test { double a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct double_test *)malloc( count * sizeof(struct double_test) );
    out = (struct double_test *)malloc( count * sizeof(struct double_test) );
    sol = (struct double_test *)malloc( count * sizeof(struct double_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = (size - 1 + i); (sol + i)->b = (size-1);
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_DOUBLE_INT, MPI_MAXLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_DOUBLE_INT and op MPI_MAXLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}



void a19()
{
    struct int_test { int a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct int_test *)malloc( count * sizeof(struct int_test) );
    out = (struct int_test *)malloc( count * sizeof(struct int_test) );
    sol = (struct int_test *)malloc( count * sizeof(struct int_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = i; (sol + i)->b = 0;
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_2INT, MPI_MINLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_2INT and op MPI_MINLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


void b19()
{
    struct long_test { long a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct long_test *)malloc( count * sizeof(struct long_test) );
    out = (struct long_test *)malloc( count * sizeof(struct long_test) );
    sol = (struct long_test *)malloc( count * sizeof(struct long_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = i; (sol + i)->b = 0;
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_LONG_INT, MPI_MINLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_LONG_INT and op MPI_MINLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


void c19()
{
    struct short_test { short a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct short_test *)malloc( count * sizeof(struct short_test) );
    out = (struct short_test *)malloc( count * sizeof(struct short_test) );
    sol = (struct short_test *)malloc( count * sizeof(struct short_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = i; (sol + i)->b = 0;
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_SHORT_INT, MPI_MINLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_SHORT_INT and op MPI_MINLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


void d19()
{
    struct float_test { float a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct float_test *)malloc( count * sizeof(struct float_test) );
    out = (struct float_test *)malloc( count * sizeof(struct float_test) );
    sol = (struct float_test *)malloc( count * sizeof(struct float_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = i; (sol + i)->b = 0;
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_FLOAT_INT, MPI_MINLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_FLOAT_INT and op MPI_MINLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


void e19()
{
    struct double_test { double a; int b; } *in, *out, *sol;
    int  i,fnderr=0;
    in = (struct double_test *)malloc( count * sizeof(struct double_test) );
    out = (struct double_test *)malloc( count * sizeof(struct double_test) );
    sol = (struct double_test *)malloc( count * sizeof(struct double_test) );
    for (i=0; i<count; i++) { (in + i)->a = (rank + i); (in + i)->b = rank;
    (sol + i)->a = i; (sol + i)->b = 0;
    (out + i)->a = 0; (out + i)->b = -1; }
    MPI_Allreduce( in, out, count, MPI_DOUBLE_INT, MPI_MINLOC, comm );
    for (i=0; i<count; i++) { if ((out + i)->a != (sol + i)->a ||
	(out + i)->b != (sol + i)->b) {
	    errcnt++; fnderr++; 
	    fprintf( stderr, "(%d) Expected (%d,%d) got (%d,%d)\n", rank,
		(int)((sol + i)->a),
		(sol+i)->b, (int)((out+i)->a), (out+i)->b );
	}}
    if (fnderr) fprintf( stderr, 
	"(%d) Error for type MPI_DOUBLE_INT and op MPI_MINLOC (%d of %d wrong)\n",
	rank, fnderr, count );
    free( in );
    free( out );
    free( sol );
}


int main( int argc, char **argv )
{
    MPI_Init( &argc, &argv );

    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    count = 10;
    comm = MPI_COMM_WORLD;

    /* Test sum */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_SUM...\n" );
#endif

    a1();
    b1();
    c1();
    d1();
    e1();
    f1();
    g1();
    h1();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_SUM\n", errcnt, rank );
    errcnt = 0;


    /* Test product */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_PROD...\n" );
#endif

    a2();
    b2();
    c2();
    d2();
    e2();
    f2();
    g2();
    h2();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_PROD\n", errcnt, rank );
    errcnt = 0;


    /* Test max */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_MAX...\n" );
#endif

    a3();
    b3();
    c3();
    d3();
    e3();
    f3();
    g3();
    h3();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_MAX\n", errcnt, rank );
    errcnt = 0;

    /* Test min */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_MIN...\n" );
#endif

    a4();
    b4();
    c4();
    d4();
    e4();
    f4();
    g4();
    h4();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_MIN\n", errcnt, rank );
    errcnt = 0;


    /* Test LOR */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_LOR...\n" );
#endif

    a5();
    b5();
    c5();
    d5();
    e5();
    f5();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_LOR(1)\n", errcnt, rank );
    errcnt = 0;


    a6();
    b6();
    c6();
    d6();
    e6();
    f6();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_LOR(0)\n", errcnt, rank );
    errcnt = 0;

    /* Test LXOR */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_LXOR...\n" );
#endif

    a7();
    b7();
    c7();
    d7();
    e7();
    f7();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_LXOR(1)\n", errcnt, rank );
    errcnt = 0;


    a8();
    b8();
    c8();
    d8();
    e8();
    f8();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_LXOR(0)\n", errcnt, rank );
    errcnt = 0;


    a9();
    b9();
    c9();
    d9();
    e9();
    f9();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_LXOR(1-0)\n", errcnt, rank );
    errcnt = 0;

    /* Test LAND */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_LAND...\n" );
#endif

    a10();
    b10();
    c10();
    d10();
    e10();
    f10();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_LAND(0)\n", errcnt, rank );
    errcnt = 0;


    a11();
    b11();
    c11();
    d11();
    e11();
    f11();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_LAND(1)\n", errcnt, rank );
    errcnt = 0;

    /* Test BOR */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_BOR...\n" );
#endif

    a12();
    b12();
    c12();
    d12();
    e12();
    f12();
    g12();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_BOR(1)\n", errcnt, rank );
    errcnt = 0;

    /* Test BAND */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_BAND...\n" );
#endif

    a13();
    b13();
    c13();
    d13();
    e13();
    f13();
    g13();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_BAND(1)\n", errcnt, rank );
    errcnt = 0;


    a14();
    b14();
    c14();
    d14();
    e14();
    f14();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_BAND(0)\n", errcnt, rank );
    errcnt = 0;

    /* Test BXOR */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_BXOR...\n" );
#endif

    a15();
    b15();
    c15();
    d15();
    e15();
    f15();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_BXOR(1)\n", errcnt, rank );
    errcnt = 0;


    a16();
    b16();
    c16();
    d16();
    e16();
    f16();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_BXOR(0)\n", errcnt, rank );
    errcnt = 0;


    a17();
    b17();
    c17();
    d17();
    e17();
    f17();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_BXOR(1-0)\n", errcnt, rank );
    errcnt = 0;


    /* Test Maxloc */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_MAXLOC...\n" );
#endif

    a18();
    b18();
    c18();
    d18();
    e18();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_MAXLOC\n", errcnt, rank );
    errcnt = 0;


    /* Test minloc */
#ifdef DEBUG
    if (rank == 0) printf( "Testing MPI_MINLOC...\n" );
#endif


    a19();
    b19();
    c19();
    d19();
    e19();

    gerr += errcnt;
    if (errcnt > 0)
	printf( "Found %d errors on %d for MPI_MINLOC\n", errcnt, rank );
    errcnt = 0;

    if (gerr > 0) {
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	printf( "Found %d errors overall on %d\n", gerr, rank );
    }

    {
	int toterrs;
	MPI_Allreduce( &gerr, &toterrs, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
	if (rank == 0) {
	    if (toterrs) {
		printf( " Found %d errors\n", toterrs );
	    }
	    else {
		printf( " No Errors\n" );
	    }
	    fflush( stdout );
	}
    }

    MPI_Finalize( );
    return 0;
}
