/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*

  Exercise communicator routines.

  This C version derived from a Fortran test program from ....

 */
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

/* #define DEBUG */
int test_communicators ( void );
int copy_fn ( MPI::Comm, int, void *, void *, void *, int * );
int delete_fn ( MPI::Comm, int, void *, void * );
#ifdef DEBUG
#define FFLUSH fflush(stdout);
#else
#define FFLUSH
#endif

int main( int argc, char **argv )
{
    int errs = 0;
    MTest_Init( &argc, &argv );
    
    errs = test_communicators();
    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}

int copy_fn( MPI::Comm oldcomm, int keyval, void *extra_state,
	     void *attribute_val_in, void *attribute_val_out, int *flag)
{
    /* Note that if (sizeof(int) < sizeof(void *), just setting the int
       part of attribute_val_out may leave some dirty bits
    */
    *(MPI::Aint *)attribute_val_out = (MPI::Aint)attribute_val_in;
    *flag = 1;
    return MPI::SUCCESS;
}

int delete_fn( MPI::Comm comm, int keyval, void *attribute_val, 
	       void *extra_state)
{
    int world_rank;
    MPI::Comm_rank( MPI::COMM_WORLD, &world_rank );
    if ((MPI::Aint)attribute_val != (MPI::Aint)world_rank) {
	printf( "incorrect attribute value %d\n", *(int*)attribute_val );
	MPI::Abort(MPI::COMM_WORLD, 1005 );
    }
    return MPI::SUCCESS;
}

int test_communicators( void )
{
    MPI::Comm dup_comm_world, lo_comm, rev_comm, dup_comm, 
	split_comm, world_comm;
    MPI::Group world_group, lo_group, rev_group;
    void *vvalue;
    int ranges[1][3];
    int flag, world_rank, world_size, rank, size, n, key_1, key_3;
    int color, key, result;
    int errs = 0;
    /*      integer n, ,
	    .        key_2
	    
    */
    MPI::Aint value;

    MPI::Comm_rank( MPI::COMM_WORLD, &world_rank );
    MPI::Comm_size( MPI::COMM_WORLD, &world_size );
#ifdef DEBUG
    if (world_rank == 0) {
	printf( "*** Communicators ***\n" ); fflush(stdout);
    }
#endif

    MPI::Comm_dup( MPI::COMM_WORLD, &dup_comm_world );

    /*
      Exercise Comm_create by creating an equivalent to dup_comm_world
      (sans attributes) and a half-world communicator.
    */

#ifdef DEBUG
    if (world_rank == 0) {
	printf( "    Comm_create\n" ); fflush(stdout);
    }
#endif

    MPI::Comm_group( dup_comm_world, &world_group );
    MPI::Comm_create( dup_comm_world, world_group, &world_comm );
    MPI::Comm_rank( world_comm, &rank );
    if (rank != world_rank) {
	errs++;
	printf( "incorrect rank in world comm: %d\n", rank );
	MPI::Abort(MPI::COMM_WORLD, 3001 );
    }

    n = world_size / 2;

    ranges[0][0] = 0;
    ranges[0][1] = (world_size - n) - 1;
    ranges[0][2] = 1;

#ifdef DEBUG
    printf( "world rank = %d before range incl\n", world_rank );FFLUSH;
#endif
    MPI::Group_range_incl(world_group, 1, ranges, &lo_group );
#ifdef DEBUG
    printf( "world rank = %d after range incl\n", world_rank );FFLUSH;
#endif
    MPI::Comm_create(world_comm, lo_group, &lo_comm );
#ifdef DEBUG
    printf( "world rank = %d before group free\n", world_rank );FFLUSH;
#endif
    MPI::Group_free( &lo_group );

#ifdef DEBUG
    printf( "world rank = %d after group free\n", world_rank );FFLUSH;
#endif

    if (world_rank < (world_size - n)) {
	MPI::Comm_rank(lo_comm, &rank );
	if (rank == MPI::UNDEFINED) {
	    errs++;
	    printf( "incorrect lo group rank: %d\n", rank ); fflush(stdout);
	    MPI::Abort(MPI::COMM_WORLD, 3002 );
	}
	else {
	    /* printf( "lo in\n" );FFLUSH; */
	    MPI::Barrier(lo_comm );
	    /* printf( "lo out\n" );FFLUSH; */
	}
    }
    else {
	if (lo_comm != MPI::COMM_NULL) {
	    errs++;
	    printf( "incorrect lo comm:\n" ); fflush(stdout);
	    MPI::Abort(MPI::COMM_WORLD, 3003 );
	}
    }

#ifdef DEBUG
    printf( "worldrank = %d\n", world_rank );FFLUSH;
#endif
    MPI::Barrier(world_comm);

#ifdef DEBUG
    printf( "bar!\n" );FFLUSH;
#endif
    /*
      Check Comm_dup by adding attributes to lo_comm & duplicating
    */
#ifdef DEBUG
    if (world_rank == 0) {
	printf( "    Comm_dup\n" );
	fflush(stdout);
    }
#endif
    
    if (lo_comm != MPI::COMM_NULL) {
	value = 9;
	MPI::Keyval_create(copy_fn,     delete_fn,   &key_1, &value );
	value = 8;
	/*     MPI::Keyval_create(MPI::DUP_FN,  MPI::NULL_DELETE_FN,
	                         &key_2, &value );  */
	value = 7;
	MPI::Keyval_create(MPI::NULL_COPY_FN, MPI::NULL_DELETE_FN,
			  &key_3, &value ); 

	/* This may generate a compilation warning; it is, however, an
	   easy way to cache a value instead of a pointer */
	/* printf( "key1 = %x key3 = %x\n", key_1, key_3 ); */
	MPI::Attr_put(lo_comm, key_1, (void *)world_rank );
	/*         MPI::Attr_put(lo_comm, key_2, world_size ) */
	MPI::Attr_put(lo_comm, key_3, (void *)0 );
	
	MPI::Comm_dup(lo_comm, &dup_comm );

	/* Note that if sizeof(int) < sizeof(void *), we can't use
	   (void **)&value to get the value we passed into Attr_put.  To avoid 
	   problems (e.g., alignment errors), we recover the value into 
	   a (void *) and cast to int. Note that this may generate warning
	   messages from the compiler.  */
	MPI::Attr_get(dup_comm, key_1, (void **)&vvalue, &flag );
	value = (MPI::Aint)vvalue;
	
	if (! flag) {
	    errs++;
	    printf( "dup_comm key_1 not found on %d\n", world_rank );
	    fflush( stdout );
	    MPI::Abort(MPI::COMM_WORLD, 3004 );
	}
	
	if (value != world_rank) {
	    errs++;
	    printf( "dup_comm key_1 value incorrect: %ld\n", (long)value );
	    fflush( stdout );
	    MPI::Abort(MPI::COMM_WORLD, 3005 );
	}

	/*         MPI::Attr_get(dup_comm, key_2, (int *)&value, &flag ); */
	/*
	  if (! flag) {
	     printf( "dup_comm key_2 not found\n" );
	     fflush( stdout );
             MPI::Abort(MPI::COMM_WORLD, 3006 );
           }

	  if (value != world_size) {
             printf( "dup_comm key_2 value incorrect: %d\n", value );
	     fflush( stdout );
             MPI::Abort(MPI::COMM_WORLD, 3007 );
	   }
	*/
	MPI::Attr_get(dup_comm, key_3, (void **)&vvalue, &flag );
	value = (int)vvalue;
	if (flag) {
	    errs++;
	    printf( "dup_comm key_3 found!\n" );
	    fflush( stdout );
	    MPI::Abort(MPI::COMM_WORLD, 3008 );
	}
	MPI::Keyval_free(&key_1 );
	/* 
	   c        MPI::Keyval_free(&key_2 )
	*/
	MPI::Keyval_free(&key_3 );
    }
    /* 
       Split the world into even & odd communicators with reversed ranks.
    */
#ifdef DEBUG
    if (world_rank == 0) {
	printf( "    Comm_split\n" );
	fflush(stdout);
    }
#endif
    
    color = world_rank % 2;
    key   = world_size - world_rank;
    
    MPI::Comm_split(dup_comm_world, color, key, &split_comm );
    MPI::Comm_size(split_comm, &size );
    MPI::Comm_rank(split_comm, &rank );
    if (rank != ((size - world_rank/2) - 1)) {
	errs++;
	printf( "incorrect split rank: %d\n", rank ); fflush(stdout);
	MPI::Abort(MPI::COMM_WORLD, 3009 );
    }
    
    MPI::Barrier(split_comm );
    /*
      Test each possible Comm_compare result
    */
#ifdef DEBUG
    if (world_rank == 0) {
	printf( "    Comm_compare\n" );
	fflush(stdout);
    }
#endif
    
    MPI::Comm_compare(world_comm, world_comm, &result );
    if (result != MPI::IDENT) {
	errs++;
	printf( "incorrect ident result: %d\n", result );
	MPI::Abort(MPI::COMM_WORLD, 3010 );
    }
    
    if (lo_comm != MPI::COMM_NULL) {
	MPI::Comm_compare(lo_comm, dup_comm, &result );
	if (result != MPI::CONGRUENT) {
	    errs++;
            printf( "incorrect congruent result: %d\n", result );
            MPI::Abort(MPI::COMM_WORLD, 3011 );
	}
    }
    
    ranges[0][0] = world_size - 1;
    ranges[0][1] = 0;
    ranges[0][2] = -1;

    MPI::Group_range_incl(world_group, 1, ranges, &rev_group );
    MPI::Comm_create(world_comm, rev_group, &rev_comm );

#ifdef DEBUG
    printf ("[%d] reverse group\n", world_rank );
    MPITEST_Group_print( rev_group );
    printf ("[%d] world group\n", world_rank );
    MPITEST_Group_print( world_group );
#endif

    MPI::Comm_compare(world_comm, rev_comm, &result );
    if (result != MPI::SIMILAR && world_size != 1) {
	errs++;
	printf( "incorrect similar result: %d\n", result );
	MPI::Abort(MPI::COMM_WORLD, 3012 );
    }
    
    if (lo_comm != MPI::COMM_NULL) {
	MPI::Comm_compare(world_comm, lo_comm, &result );
	if (result != MPI::UNEQUAL && world_size != 1) {
	    errs++;
	    printf( "incorrect unequal result: %d\n", result );
	    MPI::Abort(MPI::COMM_WORLD, 3013 );
	}
    }
    /*
      Free all communicators created
    */
#ifdef DEBUG
    if (world_rank == 0) 
	printf( "    Comm_free\n" );
#endif
    
    MPI::Comm_free( &world_comm );
    MPI::Comm_free( &dup_comm_world );
    
    MPI::Comm_free( &rev_comm );
    MPI::Comm_free( &split_comm );
    
    MPI::Group_free( &world_group );
    MPI::Group_free( &rev_group );
    
    if (lo_comm != MPI::COMM_NULL) {
        MPI::Comm_free( &lo_comm );
        MPI::Comm_free( &dup_comm );
    }
    
    return errs;
}

