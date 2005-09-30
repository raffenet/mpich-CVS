/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

static int MPIDI_Open_port(char *port_name);
/*
 * FIXME: Rather than an ifdef chain these should either fix on a single 
 * approach (which may be appropriate for the ch3 device) or use function
 * pointers in a "dynamic process structure" that are set on first use by
 * any of these routines.  In fact, that can be done in the top-level
 * routines, eliminating the need for one extra layer of routines here.
 *
 * An alternate that avoids function pointers is to #define the appropriate 
 * function names, in a block, as part of the channel and device 
 * initialization.  That keeps the code #ifdef free.
 */

/*@
   MPID_Open_port - short description

   Input Arguments:
.  MPI_Info info - info

   Output Arguments:
.  char *port_name - port name

   Notes:

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER
@*/
#undef FUNCNAME
#define FUNCNAME MPID_Open_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Open_port(MPID_Info *info_ptr, char *port_name)
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_OPEN_PORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_OPEN_PORT);

    /* FIXME: This should not be unreferenced (pass to channel) */
    MPIU_UNREFERENCED_ARG(info_ptr);

#   if defined(MPIDI_CH3_IMPLEMENTS_OPEN_PORT)
    {
	mpi_errno = MPIDI_CH3_Open_port(port_name);
    }
#   elif defined(MPIDI_DEV_IMPLEMENTS_OPEN_PORT)
    {
	mpi_errno = MPIDI_Open_port(port_name);
    }
#   else
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl",
					 "**notimpl %s", FCNAME);
	/* --END ERROR HANDLING-- */
    }
#   endif
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	}
    /* --END ERROR HANDLING-- */
	
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_OPEN_PORT);
    return mpi_errno;
}

/*@
   MPID_Close_port - close port

   Arguments:
.  char *port_name - port name

   Notes:

.N Errors
.N MPI_SUCCESS
@*/
#undef FUNCNAME
#define FUNCNAME MPID_Close_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Close_port(char *port_name)
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_CLOSE_PORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CLOSE_PORT);

#   if defined(MPIDI_CH3_IMPLEMENTS_CLOSE_PORT)
    {
	mpi_errno = MPIDI_CH3_Close_port(port_name);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	}
	/* --END ERROR HANDLING-- */
    }
#   elif defined(MPIDI_DEV_IMPLEMENTS_CLOSE_PORT)
    {
	mpi_errno = MPIDI_Close_port(port_name);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	}
	/* --END ERROR HANDLING-- */
    }
#   else
    {
	MPIU_UNREFERENCED_ARG(port_name);
	/* --BEGIN ERROR HANDLING-- */
	/* FIXME: For now leave this unimplemented without producing an error */
	/*
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl",
					 "**notimpl %s", FCNAME);
	*/
	/* --END ERROR HANDLING-- */
    }
#   endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CLOSE_PORT);
    return mpi_errno;
}

/*
 * FIXME: The routines that use this form of port name should be in the 
 * same place (i.e., the routines to encode and decode the port strings
 * should be in the same file so that their relationship to each other is
 * clear)
 */
#define MPIDI_CH3I_PORT_NAME_TAG_KEY "tag"

/*
 * MPIDI_Open_port()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_Open_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_Open_port(char *port_name)
{
    int mpi_errno = MPI_SUCCESS;
    int len;
    static int port_name_tag=0;   /* this tag is incremented and added to the 
				     business card, which is then returned 
				     as the port name */
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_OPEN_PORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_OPEN_PORT);

    len = MPI_MAX_PORT_NAME;
    mpi_errno = MPIU_Str_add_int_arg(&port_name, &len, 
			MPIDI_CH3I_PORT_NAME_TAG_KEY, port_name_tag++);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %d", mpi_errno);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    /* This works because Get_business_card appends the business cart to the
       input string, with some separator */
    mpi_errno = MPIDI_CH3I_Get_business_card(port_name, len);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_OPEN_PORT);
    return mpi_errno;
}

/*
 * The connect and accept routines use this routine to get the port tag
 * from the port name.
 */
int MPIDI_GetTagFromPort( const char *portname, int *port_name_tag )
{
    int mpi_errno;

    mpi_errno = MPIU_Str_get_int_arg(portname, MPIDI_CH3I_PORT_NAME_TAG_KEY, 
				     port_name_tag);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**argstr_port_name_tag");
    }
    return mpi_errno;
}

