/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_set_name */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_set_name = PMPI_Type_set_name
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_set_name  MPI_Type_set_name
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_set_name as PMPI_Type_set_name
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_set_name PMPI_Type_set_name

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_set_name

/*@
   MPI_Type_set_name - set datatype name

   Arguments:
+  MPI_Datatype type - datatype
-  char *type_name - type name

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_set_name(MPI_Datatype type, char *type_name)
{
    static const char FCNAME[] = "MPI_Type_set_name";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    static int setup = 0;
    MPID_MPI_STATE_DECLS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_SET_NAME);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( type, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If datatype_ptr is not value, it will be reset to null */
	    MPIR_ERRTEST_ARGNULL(type_name,"type_name", mpi_errno);
	    if (!mpi_errno) {
		int slen = strlen( type_name );
		if (slen >= MPI_MAX_OBJECT_NAME) {
		    mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG, 
		      "**typenamelen", "**typenamelen %d", slen );
		}
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_SET_NAME);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* If this is the first call, initialize all of the predefined names.
       Note that type_get_name must also make the same call */
    if (!setup) { 
	MPIR_Datatype_init_names();
	setup = 1;
    }

    /* Include the null in MPI_MAX_OBJECT_NAME */
    strncpy( datatype_ptr->name, type_name, MPI_MAX_OBJECT_NAME );
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_SET_NAME);
    return MPI_SUCCESS;
}
