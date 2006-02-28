/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

#define MPIG_BC_STR_GROWTH_RATE 1024
#define MPIG_BC_VAL_GROWTH_RATE 128

/*
 * int mpig_bc_create([IN/MOD] bc, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * Paramters:
 *
 * bc [IN/MOD] - business card object to be initialized
 * mpi_errno [IN/OUT] - MPI error code
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_create
void mpig_bc_create(mpig_bc_t * const bc, int * const mpi_errno_p, bool_t * const failed_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_bc_create);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_bc_create);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC,
		       "entering: bc=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) bc, *mpi_errno_p));
    *failed_p = FALSE;
    
    bc->str_begin = NULL;
    bc->str_end = NULL;
    bc->str_size = 0;
    bc->str_left = 0;
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC,
		       "exiting: bc=" MPIG_PTR_FMT ", mpi_errno=0x%08x, failed=%s",
		       (MPIG_PTR_CAST) bc, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_bc_create);
    return;
}
/* int mpig_bc_create() */


/*
 * int mpig_bc_destroy([IN/MOD] bc, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * Paramters:
 *
 * bc [IN/MOD] - business card object to be destroyed, freeing an internal resources, and resetting internal values
 * mpi_errno [IN/OUT] - MPI error code
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_destroy
void mpig_bc_destroy(mpig_bc_t * const bc, int * const mpi_errno_p, bool_t * const failed_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_bc_destroy);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_bc_destroy);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC,
		       "entering: bc=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) bc, *mpi_errno_p));
    *failed_p = FALSE;
    
    MPIU_Free(bc->str_begin);
    bc->str_begin = NULL;
    bc->str_end = NULL;
    bc->str_size = 0;
    bc->str_left = 0;

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC,
		       "exiting: bc=" MPIG_PTR_FMT ", mpi_errno=0x%08x, failed=%s",
		       (MPIG_PTR_CAST) bc, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_bc_destroy);
    return;
}
/* int mpig_bc_destroy() */


/*
 * int mpig_bc_add_contact([IN/MOD] bc, [IN] key, [IN] value, [IN/OUT] mpi_errno, [OUT] failed)
 * 
 * Paramters:
 *
 * bc [IN/MOD] - pointer to the business card in which to add the contact item
 * key [IN] - key under which to store contact item (e.g., "FAX")
 * value [IN] - value of the contact item
 * mpi_errno [IN/OUT] - MPI error code
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_add_contact
void mpig_bc_add_contact(mpig_bc_t * const bc, const char * const key, const char * const value, int * const mpi_errno_p,
			 bool_t * const failed_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int rc;
    MPIG_STATE_DECL(MPID_STATE_mpig_bc_add_contact);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_bc_add_contact);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC,
		       "entering: bc=" MPIG_PTR_FMT ", key=\"%s\", value=\"%s\", mpi_errno=0x%08x",
		       (MPIG_PTR_CAST) bc, key, value, *mpi_errno_p));
    *failed_p = FALSE;

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

	    str_begin = MPIU_Malloc(bc->str_size + MPIG_BC_STR_GROWTH_RATE);
	    MPIU_ERR_CHKANDJUMP1((str_begin == NULL), *mpi_errno_p, MPI_ERR_OTHER, "**nomem",
				 "**nomem %s", "business card contact");

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
	    bc->str_size += MPIG_BC_STR_GROWTH_RATE;
	    bc->str_left += MPIG_BC_STR_GROWTH_RATE;
	}
	else
	{
	    MPIU_ERR_SETANDJUMP1(*mpi_errno_p, MPI_ERR_INTERN, "**internrc", "internrc %d", rc);
	}
    }
    while (rc != MPIU_STR_SUCCESS);
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC,
		       "exiting: bc=" MPIG_PTR_FMT ", mpi_errno=0x%08x, failed=%s",
		       (MPIG_PTR_CAST) bc, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_bc_add_contact);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	*failed_p = TRUE;
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* int mpig_bc_add_contact() */


/*
 * mpig_bc_get_contact([IN] bc, [IN] key, [OUT] value, [OUT] flag, [IN/OUT] mpi_errno, [OUT] failed)
 * 
 * Paramters:
 *
 * bc - [IN] pointer to the business card in which to find the contact item
 * key - [IN] key under which the contact item is stored (e.g., "FAX")
 * value - [OUT] pointer to the memory containing the value of the contact item
 * flag - [OUT] flag indicating if the contact item was found in the business card
 * mpi_errno [IN/OUT] - MPI error code
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_get_contact
void mpig_bc_get_contact(const mpig_bc_t * const bc, const char * const key, char ** const value, int * const flag,
			 int * const mpi_errno_p, bool_t * const failed_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    char * val_str;
    int val_size;
    int rc;
    MPIG_STATE_DECL(MPID_STATE_mpig_get_contact);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_get_contact);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC,
		       "entering: bc=" MPIG_PTR_FMT ", key=\"%s\", mpi_errno=0x%08x",
		       (MPIG_PTR_CAST) bc, key, *mpi_errno_p));
    *failed_p = FALSE;

    val_size = MPIG_BC_VAL_GROWTH_RATE;
    val_str = (char *) MPIU_Malloc(val_size);
    MPIU_ERR_CHKANDJUMP1((val_str == NULL), *mpi_errno_p, MPI_ERR_OTHER, "**nomem", "**nomem %s", "business card contact value");
    
    do
    {
	rc = MPIU_Str_get_string_arg(bc->str_begin, key, val_str, val_size);
	if (rc == MPIU_STR_NOMEM)
	{
	    MPIU_Free(val_str);
	    
	    val_size += MPIG_BC_VAL_GROWTH_RATE;
	    val_str = MPIU_Malloc(val_size);
	    MPIU_ERR_CHKANDJUMP1((val_str == NULL), *mpi_errno_p, MPI_ERR_OTHER, "**nomem", "**nomem %s",
				 "business card contact value");
	}
	else if (rc == MPIU_STR_FAIL)
	{
	    *flag = FALSE;
	    goto fn_return;
	}
	else if (rc != MPIU_STR_SUCCESS)
	{
	    MPIU_ERR_SETANDJUMP1(*mpi_errno_p, MPI_ERR_INTERN, "**internrc", "internrc %d", rc);
	}
    }
    while (rc != MPIU_STR_SUCCESS);

    *flag = TRUE;
    
  fn_return:
    *value = val_str;
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC,
		       "exiting: bc=" MPIG_PTR_FMT ", key=\"%s\", value=\"%s\", valuep=" MPIG_PTR_FMT ", mpi_errno=0x%08x, "
		       "failed=%s", (MPIG_PTR_CAST) bc, key, (flag) ? *value : "", (MPIG_PTR_CAST) *value, *mpi_errno_p,
		       MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_bc_get_contact);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	if (val_str != NULL) MPIU_Free(val_str);
	*flag = FALSE;
	*failed_p = TRUE;
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/*mpig_bc_get_contact() */


/*
 * mpig_bc_free_contact([IN] value)
 *
 * Parameters:
 *
 * value [IN] - pointer to value previous returned by mpig_bc_get_contact()
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_free_contact
void mpig_bc_free_contact(char * const value)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_bc_free_contact);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_bc_free_contact);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC,
		       "entering: valuep=" MPIG_PTR_FMT ", value=\"%s\"", (MPIG_PTR_CAST) value, value));
    
    MPIU_Free(value);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC,
		       "exiting: valuep=" MPIG_PTR_FMT, (MPIG_PTR_CAST) value));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_bc_free_contact);
    return;
}
/* int mpig_bc_free_contact() */


/*
 * int mpig_bc_serialize_object([IN] bc, [OUT] str, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * Parameters:
 *
 * bc - [IN] pointer to the business card object to be serialized
 * value [OUT] - pointer to the string containing the serialized business card object
 * mpi_errno [IN/OUT] - MPI error code
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_serialize_object
void mpig_bc_serialize_object(mpig_bc_t * const bc, char ** const str, int * const mpi_errno_p, bool_t * const failed_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_bc_serialize_object);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_bc_serialize_object);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC,
		       "exiting: bc=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) bc, *mpi_errno_p));
    *failed_p = FALSE;
    
    *str = MPIU_Strdup(bc->str_begin);
    MPIU_ERR_CHKANDJUMP1((*str == NULL), *mpi_errno_p, MPI_ERR_OTHER, "**nomem", "**nomem %s", "serialized business card");

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC,
		       "exiting: bc=" MPIG_PTR_FMT ", str=" MPIG_PTR_FMT ", mpi_errno=0x%08x, failed=%s",
		       (MPIG_PTR_CAST) bc, (MPIG_PTR_CAST) *str, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_bc_serialize_object);
    return;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	*failed_p = TRUE;
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_bc_serialize_object() */


/*
 * void mpig_bc_free_serialized_object([IN] str)
 *
 * Paramters: str [IN] - pointer to the string previously returned by mpig_bc_serialized_object()
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_free_serialized_object
void mpig_bc_free_serialized_object(char * const str)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_bc_free_serialized_object);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_bc_free_serialized_object);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC,
		       "entering: str=" MPIG_PTR_FMT, (MPIG_PTR_CAST) str));
    
    MPIU_Free(str);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC,
		       "exiting: str=" MPIG_PTR_FMT, (MPIG_PTR_CAST) str));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_bc_free_serialized_object);
    return;
}
/* void mpig_bc_free_serialized_object() */


/*
 * int mpig_bc_deserialize_object([IN] str, [OUT] bc, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * Parameters:
 *
 * str - [IN] string containing a serialzed business card object
 * bc [OUT] - pointer to an uninitialized business card object in which to place the object
 * mpi_errno [IN/OUT] - MPI error code
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_deserialize_object
void mpig_bc_deserialize_object(const char * const str, mpig_bc_t * const bc, int * const mpi_errno_p, bool_t * const failed_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int str_len;
    MPIG_STATE_DECL(MPID_STATE_mpig_bc_deserialize_object);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_bc_deserialize_object);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC,
		       "entering: bc=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) bc, *mpi_errno_p));
    *failed_p = FALSE;

    str_len = strlen(str);
    
    bc->str_begin = MPIU_Strdup(str);
    MPIU_ERR_CHKANDJUMP1((bc->str_begin == NULL), *mpi_errno_p, MPI_ERR_OTHER, "**nomem", "**nomem %s", "business card string");

    /*
     * str_end should point at the terminating NULL, not the last character, so one (1) is not subtracted.  likewise, one (1) is
     * added to str_size.
     */
    bc->str_end = bc->str_begin + str_len;
    bc->str_size = str_len + 1;
    bc->str_left = 0;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC,
		       "exiting: bc=" MPIG_PTR_FMT ", mpi_errno=0x%08x, failed=%s",
		       (MPIG_PTR_CAST) bc, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    return;
    
  fn_fail:
    bc->str_begin = NULL;
    bc->str_end = NULL;
    bc->str_size = 0;
    bc->str_left = 0;
    *failed_p = TRUE;
    goto fn_return;
}
/* int mpig_bc_deserialize_object([IN] str, [OUT] bc) */
