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
void MPIR_Datatype_init_names( void ) 
{
    /* For each predefined type, ensure that there is a corresponding
       object and that the object's name is set */
    MPIR_Datatype_setname( MPI_CHAR, "char" );
    MPIR_Datatype_setname( MPI_UNSIGNED_CHAR, "unsigned char" );
    MPIR_Datatype_setname( MPI_BYTE, "byte" );
    MPIR_Datatype_setname( MPI_WCHAR_T, "wchar_t" );
    MPIR_Datatype_setname( MPI_SHORT, "short" );
    MPIR_Datatype_setname( MPI_UNSIGNED_SHORT, "unsigned short" );
    MPIR_Datatype_setname( MPI_INT, "int" );
    MPIR_Datatype_setname( MPI_UNSIGNED, "unsigned" );
    MPIR_Datatype_setname( MPI_LONG, "long" );
    MPIR_Datatype_setname( MPI_UNSIGNED_LONG, "unsigned long" );
    MPIR_Datatype_setname( MPI_FLOAT, "float" );
    MPIR_Datatype_setname( MPI_DOUBLE, "double" );
    MPIR_Datatype_setname( MPI_LONG_DOUBLE, "long double" );
    MPIR_Datatype_setname( MPI_LONG_LONG_INT, "long long int" );
    MPIR_Datatype_setname( MPI_LONG_LONG, "long long" );
    MPIR_Datatype_setname( MPI_PACKED, "packed" );
    MPIR_Datatype_setname( MPI_LB, "lb" );
    MPIR_Datatype_setname( MPI_UB, "ub" );
    MPIR_Datatype_setname( MPI_FLOAT_INT, "float_int" );
    MPIR_Datatype_setname( MPI_DOUBLE_INT, "double_int" );
    MPIR_Datatype_setname( MPI_LONG_INT, "long_int" );
    MPIR_Datatype_setname( MPI_SHORT_INT, "short_int" );
    MPIR_Datatype_setname( MPI_2INT, "2int" );
    MPIR_Datatype_setname( MPI_LONG_DOUBLE_INT, "long_double_int" );
    /* Fortran */
    MPIR_Datatype_setname( MPI_COMPLEX, "COMPLEX" );
    MPIR_Datatype_setname( MPI_DOUBLE_COMPLEX, "DOUBLE COMPLEX" );
    MPIR_Datatype_setname( MPI_LOGICAL, "LOGICAL" );
    MPIR_Datatype_setname( MPI_REAL, "REAL" );
    MPIR_Datatype_setname( MPI_DOUBLE_PRECISION, "DOUBLE PRECISION" );
    MPIR_Datatype_setname( MPI_INTEGER, "INTEGER" );
    MPIR_Datatype_setname( MPI_2INTEGER, "2INTEGER" );
    MPIR_Datatype_setname( MPI_2COMPLEX, "2COMPLEX" );
    MPIR_Datatype_setname( MPI_2DOUBLE_COMPLEX, "2DOUBLE COMPLEX" );
    MPIR_Datatype_setname( MPI_2REAL, "2REAL" );
    MPIR_Datatype_setname( MPI_2DOUBLE_PRECISION, "2DOUBLE PRECISION" );
    MPIR_Datatype_setname( MPI_CHARACTER, "CHARACTER" );
}

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
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */
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
	MPID_Common_thread_lock();
	{
	    if (!setup) {
		/* Initialize all of the predefined names */
		MPIR_Datatype_init_names();
		setup = 1;
	    }
	}
	MPID_Common_thread_unlock();
    }
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_GET_NAME);
    return MPI_SUCCESS;
}
