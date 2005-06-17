/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

#define mpig_bc_STR_GROWTH_RATE 1024
#define mpig_bc_VAL_GROWTH_RATE 128

/*
 * int mpig_bc_create([IN/OUT] bc)
 *
 * Paramters:
 *
 * bc [IN/OUT] - pointer the business card object to be initialized
 *
 * Return: MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_create
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_bc_create(mpig_bc_t * bc)
{
    int mpi_errno = MPI_SUCCESS;
    
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    bc->str_begin = NULL;
    bc->str_end = NULL;
    bc->str_size = 0;
    bc->str_left = 0;
    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;
}
/* int mpig_bc_create([IN/OUT] bc) */


/*
 * int mpig_bc_destroy([IN] bc)
 *
 * Paramters:
 *
 * bc [IN] - pointer to the business card object to be destroyed, freeing an internal resources, and resetting internal values
 *
 * Return: MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_destroy
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_bc_destroy(mpig_bc_t * bc)
{
    int mpi_errno = MPI_SUCCESS;
    
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    MPIU_Free(bc->str_begin);
    bc->str_begin = NULL;
    bc->str_end = NULL;
    bc->str_size = 0;
    bc->str_left = 0;

    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;
}
/* int mpig_bc_destroy([IN] bc) */


/*
 * int mpig_bc_add_contact([IN] bc, [IN] key, [IN] value)
 * 
 * Paramters:
 *
 * bc [IN] - pointer to the business card in which to add the contact item
 * key [IN] - key under which to store contact item (e.g., "FAX")
 * value [IN] - value of the contact item
 *
 * Return: MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_add_contact
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_bc_add_contact(mpig_bc_t * bc, const char * key, char * value)
{
    int rc;
    int mpi_errno = MPI_SUCCESS;

    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    do
    {
	char * str_end = bc->str_end;
	int str_left = bc->str_left;

	rc = (str_left > 0) ? MPIU_Str_add_string_arg(&str_end, &str_left, key, value) : MPIU_STR_NOMEM;
	if (rc == MPIU_STR_SUCCESS)
	{
	    bc->str_end = str_end;
	    bc->str_left = bc->str_left;
	}
	else if (rc == MPIU_STR_NOMEM)
	{
	    char * str_begin;

	    str_begin = MPIU_Malloc(bc->str_size + mpig_bc_STR_GROWTH_RATE);
	    MPIU_ERR_CHKANDJUMP1((str_begin == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "business card contact");

	    if (bc->str_begin != NULL)
	    { 
		MPIU_Strncpy(str_begin, bc->str_begin, (size_t) bc->str_size);
	    }
	    else
	    {
		str_begin[0] = '\0';
	    }
	    bc->str_begin = str_begin;
	    bc->str_end = bc->str_begin + (bc->str_size - bc->str_left);
	    bc->str_size += mpig_bc_STR_GROWTH_RATE;
	    bc->str_left += mpig_bc_STR_GROWTH_RATE;
	}
	else
	{
	    MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_INTERN, "**internrc", "internrc %d", rc);
	}
    }
    while (rc != MPIU_STR_SUCCESS);
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* int mpig_bc_add_contact([IN] bc, [IN] key, [IN] value) */


/*
 * mpig_bc_get_contact([IN] bc, [IN] key, [OUT] value, [OUT] flag)
 * 
 * Paramters:
 *
 * bc - [IN] pointer to the business card in which to find the contact item
 * key - [IN] key under which the contact item is stored (e.g., "FAX")
 * value - [OUT] pointer to the memory containing the value of the contact item
 * flag - [OUT] flag indicating if the contact item was found in the business card
 *
 * Return: MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_get_contact
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_bc_get_contact(mpig_bc_t * bc, const char * key, char ** value, int * flag)
{
    char * val_str;
    int val_size;
    int rc;
    int mpi_errno = MPI_SUCCESS;

    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    val_size = mpig_bc_VAL_GROWTH_RATE;
    val_str = (char *) MPIU_Malloc(val_size);
    MPIU_ERR_CHKANDJUMP1((val_str == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "business card contact value");
    
    do
    {
	rc = MPIU_Str_get_string_arg(bc->str_begin, key, val_str, val_size);
	if (rc == MPIU_STR_NOMEM)
	{
	    MPIU_Free(val_str);
	    
	    val_size += mpig_bc_VAL_GROWTH_RATE;
	    val_str = MPIU_Malloc(val_size);
	    MPIU_ERR_CHKANDJUMP1((val_str == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
				 "business card contact value");
	}
	else if (rc == MPIU_STR_FAIL)
	{
	    *flag = FALSE;
	    goto fn_return;
	}
	else if (rc != MPIU_STR_SUCCESS)
	{
	    MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_INTERN, "**internrc", "internrc %d", rc);
	}
    }
    while (rc != MPIU_STR_SUCCESS);

    *flag = TRUE;
    
  fn_return:
    *value = val_str;
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    *flag = FALSE;
    if (val_str != NULL)
    {
	MPIU_Free(val_str);
    }
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/*mpig_bc_get_contact([IN] bc, [IN] key, [OUT] value, [OUT] flag) */


/*
 * int mpig_bc_free_contact([IN] value)
 *
 * Parameters:
 *
 * value [IN] - pointer to value previous returned by mpig_bc_get_contact()
 *
 * Return: (none)
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_free_contact
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
void mpig_bc_free_contact(char * value)
{
    MPIU_Free(value);
}
/* int mpig_bc_free_contact([IN] value) */


/*
 * int mpig_bc_serialize_object([IN] bc, [OUT] str)
 *
 * Parameters:
 *
 * bc - [IN] pointer to the business card object to be serialized
 * value [OUT] - pointer to the string containing the serialized business card object
 *
 * Return: MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_serialize_object
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_bc_serialize_object(mpig_bc_t * bc, char ** str)
{
    int mpi_errno = MPI_SUCCESS;
    
    *str = MPIU_Strdup(bc->str_begin);
    MPIU_ERR_CHKANDSTMT1((*str == NULL), mpi_errno, MPI_ERR_OTHER, {;}, "**nomem", "**nomem %s", "serialized business card");

    return mpi_errno;
}
/* mpig_bc_serialize_object([IN] bc, [OUT] str) */


/*
 * void mpig_bc_free_serialized_object([IN] str)
 *
 * Paramters: str [IN] - pointer to the string previously returned by mpig_bc_serialized_object()
 
 * Return: (none)
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_free_serialized_object
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
void mpig_bc_free_serialized_object(char * str)
{
    MPIU_Free(str);
}
/* void mpig_bc_free_serialized_object([IN] str) */


/*
 * int mpig_bc_deserialize_object([IN] str, [OUT] bc)
 *
 * Parameters:
 *
 * str - [IN] string containing a serialzed business card object
 * bc [OUT] - pointer to an uninitialized business card object in which to place the object
 *
 * Return: MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_serialize_object
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_bc_deserialize_object(const char * str, mpig_bc_t * bc)
{
    int str_len;
    int mpi_errno = MPI_SUCCESS;

    str_len = strlen(str);
    
    bc->str_begin = MPIU_Strdup(str);
    MPIU_ERR_CHKANDJUMP1((bc->str_begin == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "business card string");

    /*
     * str_end should point at the terminating NULL, not the last character, so one (1) is not subtracted.  likewise, one (1) is
     * added to str_size.
     */
    bc->str_end = bc->str_begin + str_len;
    bc->str_size = str_len + 1;
    bc->str_left = 0;

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;
    
  fn_fail:
    bc->str_begin = NULL;
    bc->str_end = NULL;
    bc->str_size = 0;
    bc->str_left = 0;
    goto fn_return;
}
/* int mpig_bc_deserialize_object([IN] str, [OUT] bc) */
