/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"
#include "globus_dc.h"
#include "globus_xio_tcp_driver.h"


#define FCNAME fcname


/**********************************************************************************************************************************
							BEGIN PARAMETERS
**********************************************************************************************************************************/
/*
 * user tunable parameters
 */
#if !defined(MPIG_CM_XIO_EAGER_MAX_SIZE)
#define MPIG_CM_XIO_EAGER_MAX_SIZE (256*1024+3)
#endif

#if !defined(MPIG_CM_XIO_DATA_DENSITY_THRESHOLD)
#define MPIG_CM_XIO_DATA_DENSITY_THRESHOLD (8*1024)
#endif

#if !defined(MPIG_CM_XIO_DATA_SPARSE_BUFFER_SIZE)
#define MPIG_CM_XIO_DATA_SPARSE_BUFFER_SIZE (256*1024-1)
#endif

#if !defined(MPIG_CM_XIO_DATA_TRUNCATION_BUFFER_SIZE)
#define MPIG_CM_XIO_DATA_TRUNCATION_BUFFER_SIZE (15)
#endif

#if !defined(MPIG_CM_XIO_MAX_ACCEPT_ERRORS)
#define MPIG_CM_XIO_MAX_ACCEPT_ERRORS 255
#endif

/*
 *internal parameters
 */
#define MPIG_CM_XIO_PROTO_VERSION 1
#define MPIG_CM_XIO_PROTO_CONNECT_MAGIC "MPIG-CM-XIO-SHAKE-MAGIC-8-BALL\n"
#define MPIG_CM_XIO_PROTO_ACCEPT_MAGIC "MPIG-CM-XIO-MAGIC-8-BALL-SAYS-YES\n"
/**********************************************************************************************************************************
							 END PARAMETERS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
				      BEGIN MISCELLANEOUS MACROS, PROTOTYPES, AND VARIABLES
**********************************************************************************************************************************/
MPIG_STATIC globus_mutex_t mpig_cm_xio_mutex;

MPIG_STATIC const char * mpig_cm_xio_msg_type_get_string(mpig_cm_xio_msg_type_t msg_type);

#define mpig_cm_xio_mutex_create()	globus_mutex_init(&mpig_cm_xio_mutex, NULL)
#define mpig_cm_xio_mutex_destroy()	globus_mutex_destroy(&mpig_cm_xio_mutex)
#define mpig_cm_xio_mutex_lock()					\
{									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS,			\
		       "cm_xio global mutex - acquiring mutex"));	\
    globus_mutex_lock(&mpig_cm_xio_mutex);				\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS,			\
		       "cm_xio local data - mutex acquired"));		\
}
#define mpig_cm_xio_mutex_unlock()					\
{									\
    globus_mutex_unlock(&mpig_cm_xio_mutex);				\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS,			\
		       "cm_xio global mutex - mutex released"));	\
}
#define mpig_cm_xio_mutex_lock_conditional(cond_)	{if (cond_) mpig_cm_xio_mutex_lock();}
#define mpig_cm_xio_mutex_unlock_conditional(cond_)	{if (cond_) mpig_cm_xio_mutex_unlock();}
/**********************************************************************************************************************************
				       END MISCELLANEOUS MACROS, PROTOTYPES, AND VARIABLES
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					BEGIN INCLUSION OF INTERNAL MACROS AND PROTOTYPES
**********************************************************************************************************************************/
#undef MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS
#include "mpig_cm_xio_req.i"
#include "mpig_cm_xio_vc.i"
#include "mpig_cm_xio_conn.i"
#include "mpig_cm_xio_data.i"
#include "mpig_cm_xio_comm.i"
/**********************************************************************************************************************************
					 END INCLUSION OF INTERNAL MACROS AND PROTOTYPES
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					   BEGIN COMMUNICATION MODULE CORE API SECTION
**********************************************************************************************************************************/
/*
 * prototypes for the entries in the CM function table
 */
MPIG_STATIC void mpig_cm_xio_init(int * argc, char *** argv, int * mpi_errro_p, bool_t * failed);

MPIG_STATIC void mpig_cm_xio_finalize(int * mpi_errro_p, bool_t * failed);

MPIG_STATIC void mpig_cm_xio_add_contact_info(struct mpig_bc * bc, int * mpi_errro_p, bool_t * failed);

MPIG_STATIC void mpig_cm_xio_extract_contact_info(struct mpig_vc * vc, int * mpi_errro_p, bool_t * failed);

MPIG_STATIC void mpig_cm_xio_select_module(struct mpig_vc * vc, bool_t * selected, int * mpi_errro_p, bool_t * failed);

MPIG_STATIC void mpig_cm_xio_get_vc_compatability(const mpig_vc_t * vc1, const mpig_vc_t * vc2, unsigned levels_in,
    unsigned * const levels_out, int * mpi_errno_p, bool_t * failed_p);


/*
 * communication module virtual table
 */
const mpig_cm_vtable_t mpig_cm_xio_vtable =
{
    MPIG_CM_TYPE_XIO,
    "XIO",
    mpig_cm_xio_init,
    mpig_cm_xio_finalize,
    mpig_cm_xio_add_contact_info,
    mpig_cm_xio_extract_contact_info,
    mpig_cm_xio_select_module,
    mpig_cm_xio_get_vc_compatability,
    mpig_cm_vtable_last_entry
};


/*
 * mpig_cm_xio_init([IN/OUT] argc, [IN/OUT] argv, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_init
MPIG_STATIC void mpig_cm_xio_init(int * const argc, char *** const argv, int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t grc;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: mpi_errno=" MPIG_ERRNO_FMT, *mpi_errno_p));

    *failed_p = FALSE;
    
    MPIU_Assert(sizeof(mpig_cm_xio_msghan_funcs) != MPIG_CM_XIO_MSG_TYPE_LAST + 1);

    mpig_cm_xio_mutex_create();

    /* initialize the request completion queue */
    mpig_cm_xio_rcq_init(mpi_errno_p, &failed);
    MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|rcq_init")

    /* activate globus XIO module */
    grc = globus_module_activate(GLOBUS_XIO_MODULE);
    MPIU_ERR_CHKANDJUMP1((grc), *mpi_errno_p, MPI_ERR_OTHER, "**globus|module_activate", "**globus|module_activate %s", "XIO");

    /* initialize the connection management subsystem */
    mpig_cm_xio_conn_init(mpi_errno_p, &failed);
    MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|conn_init");
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=" MPIG_ERRNO_FMT ", failed=%s",
	*mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_init);
    return;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	*failed_p = TRUE;
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_init() */


/*
 * mpig_cm_xio_finalize([IN/OUT] mpi_errno, [OUT] failed)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_finalize
MPIG_STATIC void mpig_cm_xio_finalize(int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t grc;
    bool_t failed;
    int errors = 0;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: mpi_errno=" MPIG_ERRNO_FMT, *mpi_errno_p));

    *failed_p = FALSE;
    
    /* shutdown the connection management subsystem */
    mpig_cm_xio_conn_finalize(mpi_errno_p, &failed);
    MPIU_ERR_CHKANDSTMT((failed), *mpi_errno_p, MPI_ERR_OTHER, {errors++;}, "**globus|cm_xio|conn_finalize");
    
    /* deactivate the globus XIO module */
    grc = globus_module_deactivate(GLOBUS_XIO_MODULE);
    MPIU_ERR_CHKANDSTMT2((grc), *mpi_errno_p, MPI_ERR_OTHER, {errors++;}, "**globus|module_deactivate",
			 "**globus|module_deactivate %s %s", "XIO", globus_error_print_chain(globus_error_peek(grc)));

    /* shutdown the request completion queue */
    mpig_cm_xio_rcq_finalize(mpi_errno_p, &failed);
    MPIU_ERR_CHKANDSTMT((failed), *mpi_errno_p, MPI_ERR_OTHER, {errors++;}, "**globus|cm_xio|rcq_finalize");
    
    mpig_cm_xio_mutex_destroy();

    if (errors > 0) goto fn_fail;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=" MPIG_ERRNO_FMT ", failed=%s",
	*mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_finalize);
    return;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	*failed_p = TRUE;
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_finalize() */


/*
 * mpig_cm_xio_add_contact_info([IN/MOD] bc, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_add_contact_info
MPIG_STATIC void mpig_cm_xio_add_contact_info(mpig_bc_t * const bc, int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    char uint_str[10];
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_add_contact_info);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_add_contact_info);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: mpi_errno=" MPIG_ERRNO_FMT, *mpi_errno_p));
    
    *failed_p = FALSE;
    
    MPIU_Snprintf(uint_str, (size_t) 10, "%u", (unsigned) MPIG_CM_XIO_PROTO_VERSION);
    mpig_bc_add_contact(bc, "CM_XIO_PROTO_VERSION", uint_str, mpi_errno_p, &failed);
    MPIU_ERR_CHKANDJUMP1((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|bc_add_contact",
			 "**globus|bc_add_contact %s", "CM_XIO_PROTO_VERSION");

    mpig_bc_add_contact(bc, "CM_XIO_CONTACT_STRING", mpig_cm_xio_server_cs, mpi_errno_p, &failed);
    MPIU_ERR_CHKANDJUMP1((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|bc_add_contact",
			 "**globus|bc_add_contact %s", "CM_XIO_CONTACT_STRING");

    MPIU_Snprintf(uint_str, (size_t) 10, "%u", (unsigned) GLOBUS_DC_FORMAT_LOCAL);
    mpig_bc_add_contact(bc, "CM_XIO_DC_FORMAT", uint_str, mpi_errno_p, &failed);
    MPIU_ERR_CHKANDJUMP1((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|bc_add_contact",
			 "**globus|bc_add_contact %s", "CM_XIO_DC_FORMAT");

    if (MPIG_MY_ENDIAN == MPIG_ENDIAN_LITTLE)
    {
	mpig_bc_add_contact(bc, "CM_XIO_DC_ENDIAN", "little", mpi_errno_p, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|bc_add_contact",
			     "**globus|bc_add_contact %s", "CM_XIO_DC_ENDIAN");
    }
    else
    {
	mpig_bc_add_contact(bc, "CM_XIO_DC_ENDIAN", "big", mpi_errno_p, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|bc_add_contact",
			     "**globus|bc_add_contact %s", "CM_XIO_DC_ENDIAN");
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=" MPIG_ERRNO_FMT ", failed=%s",
	*mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_add_contact_info);
    return;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	*failed_p = TRUE;
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_add_contact_info() */


/*
 * mpig_cm_xio_extract_contact_info([IN/MOD] vc, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_extract_contact_info
MPIG_STATIC void mpig_cm_xio_extract_contact_info(mpig_vc_t * const vc, int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_extract_contact_info);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_extract_contact_info);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: vc=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	(MPIG_PTR_CAST) vc, *mpi_errno_p));

    *failed_p = FALSE;

    /* NOTE: all of the contact information for the XIO module is stored in the CM union, so it must not be stored in the VC
       until the decision has been made for the CM to take responsibility for the VC.  therefore, all extraction of information
       from the business card is done in mpig_cm_xio_select_module(). */

    /* set the topology information.  NOTE: this may seem a bit wacky since the WAN, LAN and SUBJOB levels are set even if the
       XIO module is not responsible for the VC; however, the topology information is defined such that a level set if it is
       _possible_ for the module to perform the communication regardless of whether it does so or not. */
    vc->ci.topology_levels |= MPIG_TOPOLOGY_LEVEL_WAN_MASK | MPIG_TOPOLOGY_LEVEL_LAN_MASK | MPIG_TOPOLOGY_LEVEL_SUBJOB_MASK;
    if (vc->ci.topology_num_levels <= MPIG_TOPOLOGY_LEVEL_SUBJOB)
    {
	vc->ci.topology_num_levels = MPIG_TOPOLOGY_LEVEL_SUBJOB + 1;
    }
		
    /*  fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: vc=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT ", failed=%s",
	(MPIG_PTR_CAST) vc, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_extract_contact_info);
    return;
}
/* mpig_cm_xio_extract_contact_info() */


/*
 * mpig_cm_xio_select_module([IN/MOD] vc, [OUT] selected, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_select_module
MPIG_STATIC void mpig_cm_xio_select_module(mpig_vc_t * const vc, bool_t * const selected, int * const mpi_errno_p,
    bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_bc_t * bc;
    char * version_str = NULL;
    char * contact_str = NULL;
    char * format_str = NULL;
    char * endian_str = NULL;
    int format;
    int version;
    mpig_endian_t endian;
    bool_t found;
    int rc;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_select_module);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_select_module);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: vc=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	(MPIG_PTR_CAST) vc, *mpi_errno_p));

    *failed_p = FALSE;
    *selected = FALSE;
    
    bc = mpig_vc_get_bc(vc);

    if (mpig_vc_get_cm_type(vc) == MPIG_CM_TYPE_UNDEFINED)
    {
	/* Get protocol version number and check that it is compatible with this module */
	mpig_bc_get_contact(bc, "CM_XIO_PROTO_VERSION", &version_str, &found, mpi_errno_p, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|bc_get_contact",
			     "**globus|bc_get_contact %s", "CM_XIO_PROTO_VERSION");
	if (!found) goto fn_return;

	rc = sscanf(version_str, "%d", &version);
	MPIU_ERR_CHKANDJUMP((rc != 1), *mpi_errno_p, MPI_ERR_INTERN, "**keyval");
	if (version != MPIG_CM_XIO_PROTO_VERSION) goto fn_return;

	/* Get format of basic datatypes */
	mpig_bc_get_contact(bc, "CM_XIO_DC_FORMAT", &format_str, &found, mpi_errno_p, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|bc_get_contact",
			     "**globus|bc_get_contact %s", "CM_XIO_DC_FORMAT");
	if (!found) goto fn_return;
	
	rc = sscanf(format_str, "%d", &format);
	MPIU_ERR_CHKANDJUMP((rc != 1), *mpi_errno_p, MPI_ERR_INTERN, "**keyval");

	/* Get endianess of remote system */
	mpig_bc_get_contact(bc, "CM_XIO_DC_ENDIAN", &endian_str, &found, mpi_errno_p, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|bc_get_contact",
			     "**globus|bc_get_contact %s", "CM_XIO_DC_ENDIAN");
	if (!found) goto fn_return;

	endian = (strcmp(endian_str, "little") == 0) ? MPIG_ENDIAN_LITTLE : MPIG_ENDIAN_BIG;
	
	/* initialize CM XIO fields in the VC */
	mpig_cm_xio_vc_construct(vc);
	vc->cm.xio.endian = endian;
	vc->cm.xio.df = format;

	/* adjust the PG reference to account for the newly activated VC */
	mpig_pg_inc_ref_count(mpig_vc_get_pg(vc));
    }

    if (mpig_vc_get_cm_type(vc) == MPIG_CM_TYPE_XIO)
    {
	char * cs;
	
	/* Get the contact string */
	mpig_bc_get_contact(bc, "CM_XIO_CONTACT_STRING", &contact_str, &found, mpi_errno_p, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|bc_get_contact",
			     "**globus|bc_get_contact %s", "CM_XIO_CONTACT_STRING");
	if (!found) goto fn_return;

	/* add the contact string to the VC */
	cs = MPIU_Strdup(contact_str);
	MPIU_ERR_CHKANDJUMP1((cs == NULL), *mpi_errno_p, MPI_ERR_OTHER, "**nomem", "**nomem %s", "XIO contact string");
	mpig_cm_xio_vc_set_contact_string(vc, cs);

	/* set the selected flag to indicate that the XIO communication module has accepted responsibility for the VC */
	*selected = TRUE;
    }

  fn_return:
    if (version_str != NULL) mpig_bc_free_contact(version_str);
    if (contact_str != NULL) mpig_bc_free_contact(contact_str);
    if (format_str != NULL) mpig_bc_free_contact(format_str);
    if (endian_str != NULL) mpig_bc_free_contact(endian_str);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: vc=" MPIG_PTR_FMT ", selected=%s, mpi_errno=" MPIG_ERRNO_FMT
	", failed=%s", (MPIG_PTR_CAST) vc, MPIG_BOOL_STR(*selected), *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_select_module);
    return;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	*failed_p = TRUE;
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_select_module() */

/*
 * mpig_cm_xio_get_vc_compatability([IN] vc1, [IN] vc2, [IN] levels_in, [OUT] levels_out, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_get_vc_compatability
MPIG_STATIC void mpig_cm_xio_get_vc_compatability(const mpig_vc_t * const vc1, const mpig_vc_t * const vc2,
    unsigned levels_in, unsigned * const levels_out, int * const mpi_errno_p, bool_t * const failed_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_get_vc_compatability);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_get_vc_compatability);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: vc1=" MPIG_PTR_FMT ", vc2=" MPIG_PTR_FMT ", levels_in=0x%08x, mpi_errno="
	MPIG_ERRNO_FMT, (MPIG_PTR_CAST) vc1, (MPIG_PTR_CAST) vc2, levels_in, *mpi_errno_p));

    *failed_p = FALSE;

    *levels_out = levels_in & MPIG_TOPOLOGY_LEVEL_WAN_MASK;
    
    if (levels_in & MPIG_TOPOLOGY_LEVEL_LAN_MASK)
    {
	if (mpig_vc_get_lan_id(vc1) != NULL && mpig_vc_get_lan_id(vc2) != NULL &&
	    strcmp(mpig_vc_get_lan_id(vc1), mpig_vc_get_lan_id(vc2)) == 0)
	{
	    *levels_out |= MPIG_TOPOLOGY_LEVEL_LAN_MASK;
	}
	else if (mpig_vc_get_lan_id(vc1) == NULL && mpig_vc_get_lan_id(vc2) == NULL &&
	    mpig_vc_get_pg(vc1) == mpig_vc_get_pg(vc2) && mpig_vc_get_app_num(vc1) == mpig_vc_get_app_num(vc2))
	{
	    *levels_out |= MPIG_TOPOLOGY_LEVEL_LAN_MASK;
	}
    }

#if FALSE    
    if (levels_in & MPIG_TOPOLOGY_LEVEL_SAN_MASK)
    {
	/* NOTE: the SAN level is currently unused.  however, should it ever be enabled, one cannot assume XIO is automatically
	   able to communicate over the system area network.  that depends on the SAN and the XIO drivers available.  also, even
	   if an XIO driver could be written, the XIO stream based interface may not be appropriate for the SAN and result in
	   less than optimal performance.  instead, communication over the SAN may be handled via another MPIG communication
	   module. */
    }
#endif
    
    if (levels_in & MPIG_TOPOLOGY_LEVEL_SUBJOB_MASK)
    {
	/* FIXME: for now, the XIO communication module assumes that it can be used to communicate within the subjob.  this might
	   not be the case on all systems. */
	if (mpig_vc_get_pg(vc1) == mpig_vc_get_pg(vc2) && mpig_vc_get_app_num(vc1) == mpig_vc_get_app_num(vc2))
	{
	    *levels_out |= MPIG_TOPOLOGY_LEVEL_SUBJOB_MASK;
	}
    }
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: vc1=" MPIG_PTR_FMT ", vc2=" MPIG_PTR_FMT ", levels_out=0x%08x, "
	"mpi_errno=" MPIG_ERRNO_FMT ", failed=%s", (MPIG_PTR_CAST) vc1, (MPIG_PTR_CAST) vc2, *levels_out,
	*mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_get_vc_compatability);
    return;
}
/* int mpig_cm_xio_get_vc_compatability() */
/**********************************************************************************************************************************
					    END COMMUNICATION MODULE CORE API SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
				    BEGIN COMMUNICATION MODULE PROGRESS ENGINE API FUNCTIONS
**********************************************************************************************************************************/
/*
 * void mpig_cm_xio_pe_wait([IN/OUT] state, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * XXX: this should be rolled together with the rcq_wait so that multiple requests can be completed in a single call.  also,
 * mpig_cm_xio_pe_start() should be used to eliminate returns everytime a message is receive just in case the application
 * might be calling MPI_Probe().  It would likely be better for progress_start() to clear the wakeup_pending flag, but it will
 * come at the cost of an extra lock/unlock on the RCQ mutex.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_pe_wait
void mpig_cm_xio_pe_wait(struct MPID_Progress_state * state, int * mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * req;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_pe_wait);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_pe_wait);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: mpi_errno=0x%08x", *mpi_errno_p));
    *failed_p = FALSE;

    mpig_cm_xio_rcq_deq_wait(&req, mpi_errno_p, &failed);
    MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|rcq_deq_wait");

    if (req != NULL)
    {
	mpig_request_complete(req);
    }
    else
    {
	mpig_pe_notify_unexp_recv();
    }
    
  
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", *mpi_errno_p));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_pe_wait);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	*failed_p = TRUE;
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_pe_wait() */


/*
 * void mpig_cm_xio_pe_test([IN/OUT] mpi_errno, [OUT] failed)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_pe_test
void mpig_cm_xio_pe_test(int * mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * req;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_pe_test);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_pe_test);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: mpi_errno=0x%08x", *mpi_errno_p));
    *failed_p = FALSE;

    mpig_cm_xio_rcq_deq_test(&req, mpi_errno_p, &failed);
    MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|rcq_deq_test");

    if (req != NULL)
    {
	mpig_request_complete(req);
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=0x%08x", *mpi_errno_p));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_pe_test);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	*failed_p = TRUE;
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_pe_test() */
/**********************************************************************************************************************************
				     END COMMUNICATION MODULE PROGRESS ENGINE API FUNCTIONS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					 BEGIN INCLUSION OF INTERNAL FUNCTION DEFINTIONS
**********************************************************************************************************************************/
#define MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS
#include "mpig_cm_xio_req.i"
#include "mpig_cm_xio_vc.i"
#include "mpig_cm_xio_conn.i"
#include "mpig_cm_xio_data.i"
#include "mpig_cm_xio_comm.i"
/**********************************************************************************************************************************
					  END INCLUSION OF INTERNAL FUNCTION DEFINTIONS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						  BEGIN MISCELLANEOUS FUNCTIONS
**********************************************************************************************************************************/
/*
 * char * mpig_cm_xio_msg_type_get_string([IN/MOD] req)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_msg_type_get_string
MPIG_STATIC const char * mpig_cm_xio_msg_type_get_string(mpig_cm_xio_msg_type_t msg_type)
{
    const char * str;
    
    switch(msg_type)
    {
	case MPIG_CM_XIO_MSG_TYPE_FIRST:
	    str ="MPIG_CM_XIO_MSG_TYPE_FIRST";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_UNDEFINED:
	    str ="MPIG_CM_XIO_MSG_TYPE_UNDEFINED";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_EAGER_DATA:
	    str ="MPIG_CM_XIO_MSG_TYPE_EAGER_DATA";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_RNDV_RTS:
	    str ="MPIG_CM_XIO_MSG_TYPE_RNDV_RTS";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_RNDV_CTS:
	    str ="MPIG_CM_XIO_MSG_TYPE_RNDV_CTS";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_RNDV_DATA:
	    str ="MPIG_CM_XIO_MSG_TYPE_RNDV_DATA";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_SSEND_ACK:
	    str ="MPIG_CM_XIO_MSG_TYPE_SSEND_ACK";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND:
	    str ="MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND_RESP:
	    str ="MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND_RESP";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_OPEN_REQ:
	    str ="MPIG_CM_XIO_MSG_TYPE_OPEN_REQ";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_OPEN_RESP:
	    str ="MPIG_CM_XIO_MSG_TYPE_OPEN_RESP";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_CLOSE:
	    str ="MPIG_CM_XIO_MSG_TYPE_CLOSE";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_LAST:
	    str ="MPIG_CM_XIO_MSG_TYPE_LAST";
	    break;
	default:
	    str = "(unrecognized message type)";
	    break;
    }

    return str;
}
/* mpig_cm_xio_msg_type_get_string() */
/**********************************************************************************************************************************
					       END MISCELLANEOUS HELPER FUNCTIONS
**********************************************************************************************************************************/
