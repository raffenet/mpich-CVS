/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_get_name */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_get_name = PMPI_Type_get_name
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_get_name  MPI_Type_get_name
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_get_name as PMPI_Type_get_name
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_get_name PMPI_Type_get_name

#endif

/* This routine initializes all of the name fields in the predefined 
   datatypes */
#ifndef MPICH_MPI_FROM_PMPI
/* Include these definitions only with the PMPI version */
typedef struct { MPI_Datatype dtype; const char *name; } mpi_names_t;
static mpi_names_t mpi_names[] = {
    { MPI_CHAR, "char" },
    { MPI_UNSIGNED_CHAR, "unsigned char" },
    { MPI_BYTE, "byte" },
    { MPI_WCHAR_T, "wchar_t" },
    { MPI_SHORT, "short" },
    { MPI_UNSIGNED_SHORT, "unsigned short" },
    { MPI_INT, "int" },
    { MPI_UNSIGNED, "unsigned" },
    { MPI_LONG, "long" },
    { MPI_UNSIGNED_LONG, "unsigned long" },
    { MPI_FLOAT, "float" },
    { MPI_DOUBLE, "double" },
    { MPI_LONG_DOUBLE, "long double" },
    { MPI_LONG_LONG_INT, "long long int" },
    { MPI_LONG_LONG, "long long" },
    { MPI_PACKED, "packed" },
    { MPI_LB, "lb" },
    { MPI_UB, "ub" },
    { MPI_FLOAT_INT, "float_int" },
    { MPI_DOUBLE_INT, "double_int" },
    { MPI_LONG_INT, "long_int" },
    { MPI_SHORT_INT, "short_int" },
    { MPI_2INT, "2int" },
    { MPI_LONG_DOUBLE_INT, "long_double_int" },
    /* Fortran */
    { MPI_COMPLEX, "COMPLEX" },
    { MPI_DOUBLE_COMPLEX, "DOUBLE COMPLEX" },
    { MPI_LOGICAL, "LOGICAL" },
    { MPI_REAL, "REAL" },
    { MPI_DOUBLE_PRECISION, "DOUBLE PRECISION" },
    { MPI_INTEGER, "INTEGER" },
    { MPI_2INTEGER, "2INTEGER" },
    { MPI_2COMPLEX, "2COMPLEX" },
    { MPI_2DOUBLE_COMPLEX, "2DOUBLE COMPLEX" },
    { MPI_2REAL, "2REAL" },
    { MPI_2DOUBLE_PRECISION, "2DOUBLE PRECISION" },
    { MPI_CHARACTER, "CHARACTER" },
    { 0, (char *)0 },  /* Sentinal used to indicate the last element */
};

/* This routine is also needed by type_set_name */

void MPIR_Datatype_init_names( void ) 
{
    int i;
    MPID_Datatype *datatype_ptr = NULL;
    static int setup = 0;
    
    if (setup) return;

    MPID_Common_thread_lock();
    {
	if (!setup) {

	    /* Make sure that the datatypes are initialized */
	    MPIR_Datatype_init();

	    /* For each predefined type, ensure that there is a corresponding
	       object and that the object's name is set */
	    for (i=0; mpi_names[i].name != 0; i++) {
		MPID_Datatype_get_ptr( mpi_names[i].dtype, datatype_ptr );
		if (!datatype_ptr) {
		    fprintf( stderr, "IMPLEMENTATION ERROR for datatype %d\n", 
			     i );
		    continue;
		}
		fprintf( stdout, "mpi_names[%d].name = %x\n", i, (int)mpi_names[i].name ); fflush( stdout );
		strncpy( datatype_ptr->name, mpi_names[i].name, 
			 MPI_MAX_OBJECT_NAME );
	    }
	    setup = 1;
	}
	MPID_Common_thread_unlock();
    }
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_get_name

/*@
   MPI_Type_get_name - get type name

   Arguments:
+  MPI_Datatype datatype - datatype
.  char *type_name - datatype name
-  int *resultlen - length of the result

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_get_name(MPI_Datatype datatype, char *type_name, int *resultlen)
{
    static const char FCNAME[] = "MPI_Type_get_name";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    static int setup = 0;
    MPID_MPI_STATE_DECLS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_GET_NAME);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( datatype, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */
	    MPIR_ERRTEST_ARGNULL(type_name,"type_name", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(resultlen,"resultlen", mpi_errno);
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_GET_NAME);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* If this is the first call, initialize all of the predefined names */
    if (!setup) { 
	MPIR_Datatype_init_names();
	setup = 1;
    }

    /* Include the null in MPI_MAX_OBJECT_NAME */
    strncpy( type_name, datatype_ptr->name, MPI_MAX_OBJECT_NAME );
    *resultlen = strlen( type_name );
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_GET_NAME);
    return MPI_SUCCESS;
}
