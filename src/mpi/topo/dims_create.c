/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Dims_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Dims_create = PMPI_Dims_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Dims_create  MPI_Dims_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Dims_create as PMPI_Dims_create
#endif
/* -- End Profiling Symbol Block */

/* Because we store factors with their multiplicities, a small array can
   store all of the factors for a large number (grows *faster* than n 
   factorial). */
#define MAX_FACTORS 10

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines.  You can use USE_WEAK_SYMBOLS to see if MPICH is
   using weak symbols to implement the MPI routines. */
typedef struct { int val, cnt; } Factors;
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Dims_create PMPI_Dims_create

/* Return the factors of n and their multiplicity in factors; the number of 
   factors is the return value  */
#define NUM_PRIMES 168
  static int primes[NUM_PRIMES] = 
	   {2,    3,    5,    7,   11,   13,   17,   19,   23,   29, 
	   31,   37,   41,   43,   47,   53,   59,   61,   67,   71, 
	   73,   79,   83,   89,   97,  101,  103,  107,  109,  113, 
	  127,  131,  137,  139,  149,  151,  157,  163,  167,  173, 
	  179,  181,  191,  193,  197,  199,  211,  223,  227,  229, 
	  233,  239,  241,  251,  257,  263,  269,  271,  277,  281, 
	  283,  293,  307,  311,  313,  317,  331,  337,  347,  349, 
	  353,  359,  367,  373,  379,  383,  389,  397,  401,  409, 
	  419,  421,  431,  433,  439,  443,  449,  457,  461,  463, 
	  467,  479,  487,  491,  499,  503,  509,  521,  523,  541, 
	  547,  557,  563,  569,  571,  577,  587,  593,  599,  601, 
	  607,  613,  617,  619,  631,  641,  643,  647,  653,  659, 
	  661,  673,  677,  683,  691,  701,  709,  719,  727,  733, 
	  739,  743,  751,  757,  761,  769,  773,  787,  797,  809, 
	  811,  821,  823,  827,  829,  839,  853,  857,  859,  863, 
	  877,  881,  883,  887,  907,  911,  919,  929,  937,  941, 
	  947,  953,  967,  971,  977,  983,  991,  997};
PMPI_LOCAL int factor( int n, Factors *factors, int *ndivisors )
{
    int n_tmp, n_root;
    int i, nfactors, nall=0;
    int cnt;

    /* Start from an approximate of the square root of n, by first finding
       the power of 2 at least as large as n.  The approximate root is then
       2 to the 1/2 this power */
    n_tmp  = n;
    n_root = 0;
    while (n_tmp) {
	n_root ++;
	n_tmp <<= 1;
    }
    n_root = 1 >> (n_root / 2);

    /* Find the prime number that less than that value and try dividing
       out the primes.  */
    for (i=0; i<NUM_PRIMES; i++) {
	if (primes[i] > n_root) break;
    }

    /* For each prime, divide out as many as possible */
    for (;i>=0;i--) {
	cnt = 0;
	while ( (n %  primes[i]) == 0) {
	    cnt ++;
	    n = n / primes[i];
	}
	if (cnt > 0) {
	    factors[nfactors].val = primes[i];
	    factors[nfactors++].cnt = cnt;
	    nall += cnt;
	}
    }
    *ndivisors = nall;
    return nfactors;
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Dims_create

/*@
    MPI_Dims_create - Creates a division of processors in a cartesian grid

 Input Parameters:
+ nnodes - number of nodes in a grid (integer) 
- ndims - number of cartesian dimensions (integer) 

 In/Out Parameter:   
. dims - integer array of size  'ndims' specifying the number of nodes in each 
dimension  

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Dims_create(int nnodes, int ndims, int *dims)
{
    static const char FCNAME[] = "MPI_Dims_create";
    int mpi_errno = MPI_SUCCESS;
    Factors factors[MAX_FACTORS];
    int i, j;
    int dims_needed, dims_product, nfactors, ndivisors;
    MPID_MPI_STATE_DECLS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_DIMS_CREATE);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_ARGNEG(nnodes,"nnodes",mpi_errno);
	    MPIR_ERRTEST_ARGNEG(ndims,"ndims",mpi_errno);
	    MPIR_ERRTEST_ARGNULL(dims,"dims",mpi_errno);
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_DIMS_CREATE);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Find the number of unspecified dimensions in dims and the product
       of the positive values in dims */
    dims_needed  = 0;
    dims_product = 1;
    for (i=0; i<ndims; i++) {
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    if (dims[i] < 0) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG,
						  "**argarrayneg", 
						  "**argarrayneg %s %d %d", 
						  "dims", i, dims[i] );
		MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_DIMS_CREATE);
		return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
	if (dims[i] == 0) dims_needed ++;
	else dims_product *= dims[i];
    }

    /* Can we factor nnodes by dims_product? */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    if ( (nnodes / dims_product ) * dims_product != nnodes ) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG, 
						  "**dimspartition", 0 );
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    nnodes /= dims_product;

    /* Now, factor nnodes into dims_needed components.  We'd like these
       to match the underlying machine topology as much as possible.  In the
       absence of information about the machine topology, we can try to 
       make the factors a close to each other as possible.  

       The MPICH 1 version used donated code that was quite sophisticated
       and complex.  However, since it didn't take the system topology
       into account, it was more sophisticated that was perhaps warranted.
       In addition, useful values of nnodes for MPI programs will be
       of the order 10-10000, and powers of two will be common.
    */

    /* Get the factors */
    nfactors = factor( nnodes, factors, &ndivisors );

    /* Distribute the factors among the dimensions */
    if (ndivisors <= dims_needed) {
	/* Just use the factors as needed */
	j = 0;
	for (i=0; i<ndims; i++) {
	    if (dims[i] == 0) {
		dims[i] = factors[j].val;
		if (--factors[j].cnt == 0) j++;
	    }
	}
    }
    else {
	/* We must combine some of the factors */
	/* This is what the fancy code is for in the MPICH-1 code.
	   If the number of distinct factors is 1 (e.g., a power of 2),
	   then this code can be much simpler */
	/* NOT DONE */
	;
    }

    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_DIMS_CREATE);
    return MPI_SUCCESS;
}
