/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_split */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_split = PMPI_Comm_split
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_split  MPI_Comm_split
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_split as PMPI_Comm_split
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_split PMPI_Comm_split

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_split


typedef struct splittype {
    int color, key;
} splittype;

PMPI_LOCAL void MPIU_Sort_inttable( splittype *, int );
#ifndef MPICH_MPI_FROM_PMPI
PMPI_LOCAL void MPIU_Sort_inttable( splittype *keytable, int size )
{
    splittype tmp;
    int i, j;

    /* FIXME Bubble sort */
    for (i=0; i<size; i++) {
	for (j=i+1; j<size; j++) {
	    if (keytable[i].key > keytable[j].key) {
		tmp	    = keytable[i];
		keytable[i] = keytable[j];
		keytable[j] = tmp;
	    }
	}
    }
}
#endif

/*@

MPI_Comm_split - Creates new communicators based on colors and keys

Input Parameters:
+ comm - communicator (handle) 
. color - control of subset assignment (nonnegative integer).  Processes 
  with the same color are in the same new communicator 
- key - control of rank assigment (integer)

Output Parameter:
. newcomm - new communicator (handle) 

Notes:
  The 'color' must be non-negative or 'MPI_UNDEFINED'.

.N Fortran

Algorithm:
.vb
  1. Use MPI_Allgather to get the color and key from each process
  2. Count the number of processes with the same color; create a 
     communicator with that many processes.  If this process has
     'MPI_UNDEFINED' as the color, create a process with a single member.
  3. Use key to order the ranks
  4. Set the VCRs using the ordered key values
.ve
 

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_EXHAUSTED

.seealso: MPI_Comm_free
@*/
int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
    static const char FCNAME[] = "MPI_Comm_split";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL, *newcomm_ptr;
    splittype *table, *keytable;
    int       rank, size, i, new_size, first_entry, *last_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_SPLIT);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_SPLIT);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);

            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SPLIT);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    rank = comm_ptr->rank;
    size = comm_ptr->local_size;

    /* Step 1: Find out what color and keys all of the processes have */
    table = (splittype *) MPIU_Malloc( size * sizeof(splittype) );
    table[rank].color = color;
    table[rank].key   = key;
    
    MPIR_Nest_incr();
    NMPI_Allgather( MPI_IN_PLACE, 2, MPI_INT, table, 2, MPI_INT, comm );
    MPIR_Nest_decr();

    /* Step 2: How many processes have our same color? */
    if (color == MPI_UNDEFINED) {
	/* This process is not in any group */
	new_size = 0;
    }
    else {
	new_size = 0;
	/* Also replace the color value with the index of the *next* value
	   in this set.  The integer first_entry is the index of the 
	   first element */
	last_ptr = &first_entry;
	for (i=0; i<size; i++) {
	    if (table[i].color == color) {
		new_size++;
		*last_ptr = i;
		last_ptr  = &table[i].color;
	    }
	}
    }
    /* We don't need to set the last value to -1 because we loop through
       the list for only the known size of the group */

    /* Step 3: Create the communicator */
    /* Must collectively create the communicator but we
       can recover the storage for color == MPI_UNDEFINED */
    mpi_errno = MPIR_Comm_create( comm_ptr, &newcomm_ptr );
    if (mpi_errno)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_comm_split", "**mpi_comm_split %C %d %d %p", comm, color, key, newcomm);
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SPLIT );
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }
    if (color != MPI_UNDEFINED) {
	newcomm_ptr->remote_size = new_size;
	newcomm_ptr->local_size  = new_size;
	newcomm_ptr->comm_kind   = MPID_INTRACOMM;
    
	/* Step 4: Order the processes by their key values.  Sort the
	   list that is stored in table.  To simplify the sort, we 
	   extract the table into a smaller array and sort that.
	   Also, store in the "color" entry the rank in the input communicator
	   of the entry. */
	keytable = (splittype *) MPIU_Malloc( new_size * sizeof(splittype) );
	for (i=0; i<new_size; i++) {
	    keytable[i].key	  = table[first_entry].key;
	    keytable[i].color = first_entry;
	    first_entry	  = table[first_entry].color;
	}

	/* sort key table.  The "color" entry is the rank of the corresponding
	   process in the input communicator */
	MPIU_Sort_inttable( keytable, new_size );

	MPID_VCRT_Create( new_size, &newcomm_ptr->vcrt );
	MPID_VCRT_Get_ptr( newcomm_ptr->vcrt, &newcomm_ptr->vcr );
	for (i=0; i<new_size; i++) {
	    MPID_VCR_Dup( comm_ptr->vcr[keytable[i].color], 
			  &newcomm_ptr->vcr[i] );
	    if (keytable[i].color == comm_ptr->rank) {
		newcomm_ptr->rank = i;
	    }
	}

	/* Inherit the error handler (if any) */
	newcomm_ptr->errhandler = comm_ptr->errhandler;
	if (comm_ptr->errhandler) {
	    MPIU_Object_add_ref( comm_ptr->errhandler );
	}

	/* Clear other items */
	newcomm_ptr->remote_group = 0;
	newcomm_ptr->local_group  = 0;
	newcomm_ptr->coll_fns	  = 0;
	newcomm_ptr->name[0]	  = 0;

	/* Free all storage */
	MPIU_Free( keytable );
	*newcomm = newcomm_ptr->handle;
    }
    else {
	*newcomm = MPI_COMM_NULL;
	MPIU_Handle_obj_free( &MPID_Comm_mem, newcomm_ptr ); 
    }
    MPIU_Free( table );

    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SPLIT);
    return MPI_SUCCESS;
}

