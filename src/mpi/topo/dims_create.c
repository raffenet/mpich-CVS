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
typedef struct Factors { int val, cnt; } Factors;
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Dims_create PMPI_Dims_create

/* Return the factors of n and their multiplicity in factors; the number of 
   distinct factors is the return value and the total number of factors,
   including multiplicities, is returned in ndivisors */
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
PMPI_LOCAL int factor( int, Factors [], int * );

PMPI_LOCAL int factor( int n, Factors factors[], int *ndivisors )
{
    int n_tmp, n_root;
    int i, nfactors=0, nall=0;
    int cnt;

    /* Start from an approximate of the square root of n, by first finding
       the power of 2 at least as large as n.  The approximate root is then
       2 to the 1/2 this power */
    n_tmp  = n;
    n_root = 0;
    while (n_tmp) {
	n_root ++;
	n_tmp >>= 1;
    }
    n_root = 1 << (n_root / 2);

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
	    /* --BEGIN ERROR HANDLING-- */
	    if (nfactors + 1 == MAX_FACTORS) {
		/* Time to panic.  This should not happen, since the
		   smallest number that could exceed this would
		   be the product of the first 10 primes that are
		   greater than one, which is 6469693230 */
		return nfactors;
	    }
	    /* --END ERROR HANDLING-- */
	    factors[nfactors].val = primes[i];
	    factors[nfactors++].cnt = cnt;
	    nall += cnt;
	}
    }
    /* If nfactors == 0, n was a prime, so return that */
    if (nfactors == 0) {
	nfactors = 1;
        nall = 1;
        factors[0].val = n;
        factors[0].cnt = 1;
    }
    else if (n > 1) {
	/* We need one more factor (a single prime > n_root) */
	factors[nfactors].val = n;
	factors[nfactors++].cnt   = 1;
	nall++;
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
 dimension.  A value of 0 indicates that 'MPI_Dims_create' should fill in a
 suitable value.

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
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_DIMS_CREATE);

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
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_DIMS,
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
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_DIMS, 
						  "**dimspartition", 0 );
		MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_DIMS_CREATE);
		return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
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
       In addition, useful values of nnodes for most MPI programs will be
       of the order 10-10000, and powers of two will be common.
    */

    /* Get the factors */
    nfactors = factor( nnodes, factors, &ndivisors );

    /* Divide into 3 major cases:
       1. Fewer divisors than needed dimensions.  Just use all of the
          factors up, setting the remaining dimensions to 1
       2. Only one distinct factor (typically 2) but with greater 
          multiplicity.  Give each dimension a nearly equal size
       3. Other.  There are enough factors to divide among the dimensions.
          This is done in an ad hoc fashion
    */

/* DEBUG
    printf( "factors are (%d of them) with %d divisors\n", nfactors, ndivisors );
    for (j=0; j<nfactors; j++) {
	printf( "val = %d repeated %d\n", factors[j].val, factors[j].cnt );
    }
*/
    /* Distribute the factors among the dimensions */
    if (ndivisors <= dims_needed) {
	/* Just use the factors as needed */
	j = 0;
	for (i=0; i<ndims; i++) {
	    if (dims[i] == 0) {
		dims[i] = factors[j].val;
		if (--factors[j].cnt == 0) {
		    if (++j >= nfactors) break;
		}
	    }
	}
	/* Any remaining dims are set to one */
	for (i++;i<ndims; i++) {
	    dims[i] = 1;
	}
    }
    else {
	/* We must combine some of the factors */
	/* This is what the fancy code is for in the MPICH-1 code.
	   If the number of distinct factors is 1 (e.g., a power of 2),
	   then this code can be much simpler */
	/* NOT DONE */
	/* FIXME */
	if (nfactors == 1) {
	    /* Special case for k**n, such as powers of 2 */
	    int factor = factors[0].val;
	    int cnt    = factors[0].cnt; /* Numver of factors left */
	    int cnteach = ( cnt + dims_needed - 1 )/ dims_needed;
	    int factor_each;
	    
	    factor_each = factor;
	    for (i=1; i<cnteach; i++) factor_each *= factor;

	    for (i=0; i<ndims; i++) {
		if (dims[i] == 0) {
		    if (cnt > cnteach) {
			dims[i] = factor_each;
			cnt -= cnteach;
		    }
		    else if (cnt > 0) {
			factor_each = factor;
			for (j=1; j<cnt; j++) 
			    factor_each *= factor;
			dims[i] = factor_each;
			cnt = 0;
		    }
		    else {
			dims[i] = 1;
		    }
		}
	    }
	}	    
	else {
	    /* Here is the general case.  For now, we do the following
	       simple approach:
	       target_size = nnodes / ndims needed.
	       Accumulate factors, starting from the bottom,
	       until the target size is met or exceeded.
	       Put all of the remaining factors into the last dimension
	       (recompute target_size with each step, since we may
	       miss the target by a wide margin.

	       A much more sophisticated code would try to balance
	       the number of nodes assigned to each dimension, possibly
	       in concert with guidelines from the device about "good"
	       sizes
	    */
	    int nodes_needed = nnodes;
	    int target_size = nodes_needed / dims_needed;
	    int factor;
	    j = 0;
	    for (i=0; i<ndims; i++) {
		if (dims[i] == 0) {
		    if (dims_needed == 1) {
			/* Dump all of the remaining factors into this
			   entry */
			factor = 1;
			while (j < nfactors) {
			    factor *= factors[j].val;
			    if (--factors[j].cnt == 0) j++;
			}
		    }
		    else {
			/* Get the current target size */
			factor = 1;
			while (j < nfactors && factor < target_size) {
			    factor *= factors[j].val;
			    if (--factors[j].cnt == 0) j++;
			}
		    }
		dims[i] = factor;
		nodes_needed /= factor;
		dims_needed--;
		if (dims_needed)
		    target_size = nodes_needed / dims_needed;
		}
	    }
	}
    }

    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_DIMS_CREATE);
    return MPI_SUCCESS;
}
