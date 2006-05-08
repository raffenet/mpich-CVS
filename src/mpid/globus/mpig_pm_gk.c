/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

/*
 * This file contains an implementation of process management routines that interface with the Globus 2.x GRAM service.
 */

#include "mpidimpl.h"
#include "globus_duroc_runtime.h"
#include "globus_duroc_bootstrap.h"
#include "globus_gram_client.h"
#include "globus_gram_myjob.h"

#define INT_MAX_STRLEN 10

/* inter-subjob and intra-subjob message tags */
#define MPIG_PM_GK_TAG_MAX_SIZE	64

#define MPIG_PM_GK_TAG_SJ_MASTER_TO_PG_MASTER_TOPOLOGY	"sjm2pgm-t"
#define MPIG_PM_GK_TAG_PG_MASTER_TO_SJ_MASTER_TOPOLOGY	"pgm2sjm-t"
#define MPIG_PM_GK_TAG_SJ_MASTER_TO_SJ_SLAVE_TOPOLOGY	"sjm2sjs-t"

#define MPIG_PM_GK_TAG_SJ_SLAVE_TO_SJ_MASTER_DATA	"sjs2sjm-d"
#define MPIG_PM_GK_TAG_SJ_MASTER_TO_SJ_MASTER_DATA	"sjm2sjm-d"
#define MPIG_PM_GK_TAG_SJ_MASTER_TO_SJ_SLAVE_DATA	"sjm2sjs-d"


MPIG_STATIC enum
{
    MPIG_PM_GK_STATE_UNINITIALIZED = 0,
    MPIG_PM_GK_STATE_INITIALIZED,
    MPIG_PM_GK_STATE_GOT_PG_INFO,
    MPIG_PM_GK_STATE_FINALIZED
}
mpig_pm_gk_state = MPIG_PM_GK_STATE_UNINITIALIZED;
MPIG_STATIC int mpig_pm_gk_pg_size;			/* the size of the initial process group (comm world) */
MPIG_STATIC int mpig_pm_gk_pg_rank;			/* the rank of the local process in the initial process group */
MPIG_STATIC const char * mpig_pm_gk_pg_id;		/* the unique identification string of the initial process group */
MPIG_STATIC int mpig_pm_gk_my_sj_size;			/* the size of subjob to which the local process belongs */
MPIG_STATIC int mpig_pm_gk_my_sj_rank;			/* the rank of the local process within its subjob */
MPIG_STATIC int mpig_pm_gk_sj_num;			/* the number of subjobs in the initial process group */
MPIG_STATIC int mpig_pm_gk_sj_index;			/* the id of subjob to which the local process belongs
							   [0..mpig_pm_gk_sj_num-1] */
MPIG_STATIC int * mpig_pm_gk_sj_addrs;			/* duroc addresses for the subjob masters */
#if defined(MPIG_VMPI)
MPIG_STATIC mpig_vmpi_comm_t mpig_pm_gk_vmpi_cw;	/* a duplicate of MPIG_VMPI_COMM_WORLD to be used by this module */
MPIG_STATIC int mpig_pm_gk_vmpi_cw_size;		/* the size of MPIG_VMPI_COMM_WORLD to which the local process belongs */
MPIG_STATIC int mpig_pm_gk_vmpi_cw_rank;		/* the rank of the local process within MPIG_VMPI_COMM_WORLD */
MPIG_STATIC int mpig_pm_gk_vmpi_root;			/* the rank within MPGI_VMPI_COMM_WORLD of the subjob leader */
#endif


MPIG_STATIC int mpig_pm_gk_get_topology(int my_sj_rank, int my_sj_size, int * pg_size, int * pg_rank, int * sj_num, int * sj_index,
    int ** sj_addrs);

MPIG_STATIC int mpig_pm_gk_distribute_byte_array(int pg_size, int pg_rank, int sj_num, int my_sj_size, int my_sj_rank, 
    const int * sj_duroc_addrs, const globus_byte_t * in_buf, int in_buf_len, globus_byte_t ** out_bufs, int * out_bufs_lens);

MPIG_STATIC int mpig_pm_gk_extract_byte_arrays(const globus_byte_t * rbuf, int * nbufs_p, globus_byte_t ** out_bufs,
    int * out_bufs_lens);

#if !defined(MPIG_VMPI)
MPIG_STATIC int mpig_pm_gk_intra_subjob_send(int dest, const char * tag_base, int nbytes, const globus_byte_t * buf);

MPIG_STATIC int mpig_pm_gk_intra_subjob_receive(const char * tag_base, int * nbytes, globus_byte_t ** buf);

MPIG_STATIC int mpig_pm_gk_intra_subjob_bcast(int my_sj_size, int my_sj_rank, const char * tag_base, int * nbytes,
    globus_byte_t ** buf);

/* nbytes and buf are relevant only on the subjob master */
MPIG_STATIC int mpig_pm_gk_intra_subjob_gather(int my_sj_size, int my_sj_rank, const globus_byte_t * in_buf, int in_buf_len,
    const char * tag_base, int * nbytes, globus_byte_t ** buf);
#endif

/*
 * mpig_pm_gk_init()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_gk_init
int mpig_pm_gk_init(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    enum
    {
	FUNC_STATE_START,
	FUNC_STATE_DUROC_INIT,
	FUNC_STATE_NEXUS_INIT,
	
    } func_state = FUNC_STATE_START;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_gk_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_gk_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state == MPIG_PM_GK_STATE_FINALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_finalized");
    
    /* activate the duroc runtime module, and wait for all other processes to startup */
    rc = globus_module_activate(GLOBUS_DUROC_RUNTIME_MODULE);
    MPIU_ERR_CHKANDJUMP1((rc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|module_activate",
	"**globus|module_activate %s", "DUROC runtime");
    func_state = FUNC_STATE_DUROC_INIT;
    
    globus_duroc_runtime_barrier();

    /* the duroc bootstrap module uses nexus and keeps nexus activated during the entire computation.  unfornately, by default, a
       substantial number of error messages are output by nexus when a remote process terminates unexpectedly.  furthermore,
       nexus_fatal is called causing the local process to terminate.  by registering null error handlers with nexus, we prevent
       the extraneous error messages and prevent our process from terminating unexpectedly. */
    rc = globus_module_activate(GLOBUS_NEXUS_MODULE);
    MPIU_ERR_CHKANDJUMP1((rc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|module_activate",
	"**globus|module_activate %s", "Nexus");
    func_state = FUNC_STATE_NEXUS_INIT;

    nexus_enable_fault_tolerance (NULL, NULL);
    
    /* get the topology info, including the process group size and the rank of this process in that group */
    rc = globus_duroc_runtime_intra_subjob_size(&mpig_pm_gk_my_sj_size);
    MPIU_ERR_CHKANDJUMP((rc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|pm_get_sjsize");
	
    rc = globus_duroc_runtime_intra_subjob_rank(&mpig_pm_gk_my_sj_rank);
    MPIU_ERR_CHKANDJUMP((rc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|pm_get_sjrank");

#   if defined(MPIG_VMPI)
    {
	int vrc;
	int * argc;
	char *** argv;
	int ranks[2];

	globus_module_get_args(&argc, &argv);

	/* initialize the vendor MPI module */
	vrc = mpig_vmpi_init(argc, argv);
	MPIU_ERR_CHKANDJUMP((vrc), mpi_errno, MPI_ERR_OTHER, "**globus|vmpi_init");
	
	vrc = mpig_vmpi_comm_size(MPIG_VMPI_COMM_WORLD, &mpig_pm_gk_vmpi_cw_size);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Comm_size", &mpi_errno);
	
	vrc = mpig_vmpi_comm_rank(MPIG_VMPI_COMM_WORLD, &mpig_pm_gk_vmpi_cw_rank);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Comm_rank", &mpi_errno);

	vrc = mpig_vmpi_comm_dup(MPIG_VMPI_COMM_WORLD, &mpig_pm_gk_vmpi_cw);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Comm_dup", &mpi_errno);
	
	MPIU_Assert(mpig_pm_gk_my_sj_size == mpig_pm_gk_vmpi_cw_size);
	
	ranks[0] = mpig_pm_gk_my_sj_rank;
	ranks[1] = mpig_pm_gk_vmpi_cw_rank;

	vrc = mpig_vmpi_allreduce(ranks, MPIG_VMPI_IN_PLACE, 1, MPIG_VMPI_2INT, MPIG_VMPI_MINLOC, &mpig_pm_gk_vmpi_cw);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Allreduce", &mpi_errno);

	MPIU_Assert(ranks[0] == 0);
	mpig_pm_gk_vmpi_root = ranks[1];
    }
#   endif
    
    rc = mpig_pm_gk_get_topology(mpig_pm_gk_my_sj_size, mpig_pm_gk_my_sj_rank, &mpig_pm_gk_pg_size, &mpig_pm_gk_pg_rank,
	&mpig_pm_gk_sj_num, &mpig_pm_gk_sj_index, &mpig_pm_gk_sj_addrs);
    MPIU_ERR_CHKANDJUMP((rc != MPI_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|pm_get_topology");

    /* MPI-2-FIXME: this needs to be set to some real before we implement MPI-2 functionality */
    mpig_pm_gk_pg_id = "XXX";

    /* we already have all of the PG information, but since implemenations are not required to have the info until after
       mpig_pm_gk_exchange_business_cards() is called, we only set the state to INITIALIZED to catch any usage errors. */
    mpig_pm_gk_state = MPIG_PM_GK_STATE_INITIALIZED;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: mpi_errno=0x%08x", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_gk_init);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    switch(func_state)
    {
	case FUNC_STATE_NEXUS_INIT:
	{
	    globus_module_deactivate(GLOBUS_NEXUS_MODULE);
	}
	case FUNC_STATE_DUROC_INIT:
	{
	    globus_module_deactivate(GLOBUS_DUROC_RUNTIME_MODULE);
	}
	default:
	{
	}
    }
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_pm_gk_init() */


/*
 * mpig_pm_gk_finalize()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_gk_finalize
int mpig_pm_gk_finalize(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_gk_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_gk_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state == MPIG_PM_GK_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state == MPIG_PM_GK_STATE_FINALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_finalized");
    
    mpig_pm_gk_state = MPIG_PM_GK_STATE_FINALIZED;

    /* shutdown the vendor MPI module */
#   if defined(MPIG_VMPI)
    {
	int vrc;
	
	vrc = mpig_vmpi_comm_free(&mpig_pm_gk_vmpi_cw);
	MPIG_ERR_VMPI_CHKANDSTMT(vrc, "MPI_Comm_dup", {;}, &mpi_errno);
	
	vrc = mpig_vmpi_finalize();
	MPIG_ERR_VMPI_CHKANDSTMT(vrc, "MPI_Finalize", {;}, &mpi_errno);
    }
#   endif

    /* deactivate the duroc and nexus modules.  remember, nexus was activated in init to prevent excessive and confusing error
       messages. */
    rc = globus_module_deactivate(GLOBUS_NEXUS_MODULE);
    MPIU_ERR_CHKANDSTMT1((rc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|module_deactivate",
	"**globus|module_deactivate %s", "Nexus");

    rc = globus_module_deactivate(GLOBUS_DUROC_RUNTIME_MODULE);
    MPIU_ERR_CHKANDSTMT1((rc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|module_deactivate",
	"**globus|module_deactivate %s", "DUROC runtime");

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: mpi_errno=0x%08x", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_gk_finalize);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_pm_gk_finalize() */


/*
 * mpig_pm_gk_exchange_business_cards()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_gk_exchange_business_cards
int mpig_pm_gk_exchange_business_cards(mpig_bc_t * bc, mpig_bc_t ** bcs_ptr)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIU_CHKLMEM_DECL(2);
    MPIU_CHKPMEM_DECL(1);
    char * bc_str;
    char ** bc_strs;
    int * bc_lens;
    mpig_bc_t * bcs;
    int p;
    int errors = 0;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_gk_exchange_business_cards);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_gk_exchange_business_cards);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering: bc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) bc));

    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state == MPIG_PM_GK_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state == MPIG_PM_GK_STATE_FINALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_finalized");
    
    mpig_bc_serialize_object(bc, &bc_str, &mpi_errno, &failed);
    MPIU_ERR_CHKANDJUMP((failed), mpi_errno, MPI_ERR_OTHER, "**globus|bc_serialize");
    
    MPIU_CHKLMEM_MALLOC(bc_strs, char **, mpig_pm_gk_pg_size * sizeof(char *), mpi_errno,
	"array of serialized business cards");
    MPIU_CHKLMEM_MALLOC(bc_lens, int *, mpig_pm_gk_pg_size * sizeof(int), mpi_errno, "array of business cards lengths");
    MPIU_CHKPMEM_MALLOC(bcs, mpig_bc_t *, mpig_pm_gk_pg_size * sizeof(mpig_bc_t), mpi_errno, "array of business cards lengths");
    
    mpi_errno = mpig_pm_gk_distribute_byte_array(mpig_pm_gk_pg_size, mpig_pm_gk_pg_rank, mpig_pm_gk_sj_num,
	mpig_pm_gk_my_sj_size, mpig_pm_gk_my_sj_rank, mpig_pm_gk_sj_addrs, (globus_byte_t *) bc_str, (int) strlen(bc_str) + 1,
	(globus_byte_t **) bc_strs, bc_lens);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|pm_distribute_business_cards");

    for (p = 0; p < mpig_pm_gk_pg_size; p++)
    {
	mpig_bc_deserialize_object((char *) bc_strs[p], &bcs[p], &mpi_errno, &failed);
	MPIU_ERR_CHKANDSTMT((failed), mpi_errno, MPI_ERR_OTHER, {errors++;}, "**globus|bc_deserialize");
	MPIU_Free(bc_strs[p]);
    }

    mpig_bc_free_serialized_object(bc_str);

    if (errors > 0) goto fn_fail;
    
    MPIU_CHKPMEM_COMMIT();
    *bcs_ptr = bcs;
    
    mpig_pm_gk_state = MPIG_PM_GK_STATE_GOT_PG_INFO;
    
  fn_return:
    MPIU_CHKLMEM_FREEALL();
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
		       "exiting: bcs=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) *bcs_ptr, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_gk_exchange_business_cards);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIU_CHKPMEM_REAP();
    *bcs_ptr = NULL;
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_pm_gk_exchange_business_cards() */


/*
 * mpig_pm_gk_free_business_cards()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_gk_free_business_cards
int mpig_pm_gk_free_business_cards(mpig_bc_t * bcs)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int p;
    int errors = 0;
    bool_t failed;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_gk_free_business_cards);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_gk_free_business_cards);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering: bcs=" MPIG_PTR_FMT, (MPIG_PTR_CAST) bcs));

    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state == MPIG_PM_GK_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state == MPIG_PM_GK_STATE_FINALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_finalized");
    
    for (p = 0; p < mpig_pm_gk_pg_size; p++)
    {
	mpig_bc_destroy(&bcs[p], &mpi_errno, &failed);
	MPIU_ERR_CHKANDSTMT((failed), mpi_errno, MPI_ERR_OTHER, {errors++;}, "**globus|bc_destroy");
    }
    MPIU_Free(bcs);

    if (errors > 0) goto fn_fail;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: mpi_errno=0x%08x", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_gk_free_business_cards);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_pm_gk_free_business_cards() */


/*
 * mpig_pm_gk_get_pg_size()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_gk_get_pg_size
int mpig_pm_gk_get_pg_size(int * pg_size)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_gk_get_pg_size);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_gk_get_pg_size);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state == MPIG_PM_GK_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state == MPIG_PM_GK_STATE_FINALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_finalized");
    
    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state != MPIG_PM_GK_STATE_GOT_PG_INFO), mpi_errno, MPI_ERR_INTERN, "**globus|pm_no_pg_size");
    *pg_size = mpig_pm_gk_pg_size;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
	"exiting: pg_size=%d, mpi_errno=0x%08x", (mpi_errno) ? -1 : *pg_size, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_gk_get_pg_size);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_pm_gk_get_pg_size() */


/*
 * mpig_pm_gk_get_pg_rank()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_gk_get_pg_rank
int mpig_pm_gk_get_pg_rank(int * pg_rank)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_gk_get_pg_rank);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_gk_get_pg_rank);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state == MPIG_PM_GK_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state == MPIG_PM_GK_STATE_FINALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_finalized");
    
    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state != MPIG_PM_GK_STATE_GOT_PG_INFO), mpi_errno, MPI_ERR_INTERN, "**globus|pm_no_pg_rank");
    *pg_rank = mpig_pm_gk_pg_rank;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
		       "exiting: pg_rank=%d, mpi_errno=0x%08x", (mpi_errno) ? -1 : *pg_rank, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_gk_get_pg_rank);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_pm_gk_get_pg_rank() */


/*
 * mpig_pm_gk_get_pg_id()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_gk_get_pg_id
int mpig_pm_gk_get_pg_id(const char ** pg_id)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_gk_get_pg_id);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_gk_get_pg_id);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state == MPIG_PM_GK_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state == MPIG_PM_GK_STATE_FINALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_finalized");
    
    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state != MPIG_PM_GK_STATE_GOT_PG_INFO), mpi_errno, MPI_ERR_INTERN, "**globus|pm_no_pg_id");
    *pg_id = mpig_pm_gk_pg_id;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
		       "exiting: pg_id=%s, mpi_errno=0x%08x", (mpi_errno) ? "(null)" : *pg_id, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_gk_get_pg_id);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_pm_gk_get_pg_id() */


/*
 * mpig_pm_gk_get_app_num()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_gk_get_app_num
int mpig_pm_gk_get_app_num(int * const app_num)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_gk_get_app_num);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_gk_get_app_num);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state == MPIG_PM_GK_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state == MPIG_PM_GK_STATE_FINALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_finalized");
    
    MPIU_ERR_CHKANDJUMP((mpig_pm_gk_state != MPIG_PM_GK_STATE_GOT_PG_INFO), mpi_errno, MPI_ERR_INTERN, "**globus|pm_no_pg_id");
    *app_num = mpig_pm_gk_sj_index;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
	"exiting: app_num=%d, mpi_errno=0x%08x", (mpi_errno) ? -1 : *app_num, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_gk_get_app_num);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_pm_gk_get_app_num() */


/*
 * mpig_gm_gk_get_topology(my_sj_size, my_sj_rank, pg_size, pg_rank, sj_num, my_rsl_index, sj_addrs)
 *
 * NOTE: this routine _MUST_ be called by _EVERY_ process, each supplying my_sj_rank and my_sj_size.  the subjob master is the
 * process whre my_sj_rank == 0; all other processes are subjob slaves.
 *
 * this routine fills in the remaining args:
 *
 * pg_size - total number of processes in the process group
 *
 * my_pg_rank - my rank in [0, pg_size-1]
 *
 * sj_num - total number of subjobs
 *
 * my_rsl_index = the subjob index to which the local process belongs, where 0 <= pg_sj_index <= pg_sj_num.  the index is
 * specified in the RSL using the environment variable GLOBUS_DUROC_SUBJOB_INDEX.
 *
 * sj_duroc_addrs - an array containing the duroc addresses of each subjob master.  the array only contains the addresses for the
 * other subjob masters, and thus the length of the array is sj_num - 1.  the array is only valid for subjob master processes.
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_gk_get_topology
MPIG_STATIC int mpig_pm_gk_get_topology(int my_sj_size, int my_sj_rank, int * const pg_size, int * const my_pg_rank,
    int * const sj_num, int * const my_rsl_index, int ** const sj_duroc_addrs)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    static int call_count = 0;
    char tag[MPIG_PM_GK_TAG_MAX_SIZE];
    globus_byte_t sbuf[GRAM_MYJOB_MAX_BUFFER_LENGTH];
    globus_byte_t * rbuf;
    int rbuf_len;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(3);
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_gk_get_topology);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_gk_get_topology);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering: my_sj_size=%d, my_sj_rank=%d",
	my_sj_size, my_sj_rank));

    call_count += 1;

    if (my_sj_rank != 0)
    {
	/* subjob slave */
#       if defined(MPIG_VMPI)
        {
	    int vrc;
	    int v[4];

	    /* this algorithm assumes that process zero in the subjob is the same as process zero in the vendor MPI_COMM_WORLD.
	       this is a reasonable assumption since mpig_pm_gk_init() sets the subjob rank using the rank from the vendor
	       MPI_COMM_WORLD. */
	    vrc = mpig_vmpi_bcast((void *) v, 4, MPIG_VMPI_INT, mpig_pm_gk_vmpi_root, &mpig_pm_gk_vmpi_cw);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Bcast", &mpi_errno);

	    /*
	     * v[0] - the number of processes in the process group
	     * v[1] - the first process group rank for the subjob
	     * v[2] - the number of subjobs in the process group
	     * v[3] - the index of the subjob to which the local process belongs as defined in the rsl
	     */
	    *pg_size = v[0];
	    *my_pg_rank = v[1] + mpig_pm_gk_vmpi_cw_rank;
	    *sj_num = v[2];
	    *my_rsl_index = v[3];
        }
#       else
        {
	    sprintf(tag, "%s%d", MPIG_PM_GK_TAG_SJ_MASTER_TO_SJ_SLAVE_TOPOLOGY, call_count);
	    mpi_errno = mpig_pm_gk_intra_subjob_receive(tag, &rbuf_len, &rbuf);
	    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s", "mpig_pm_gk_intra_subjob_receive");
	    sscanf((char *) rbuf, "%d %d %d %d", pg_size, my_pg_rank, sj_num, my_rsl_index);
	    MPIU_Free(rbuf);
        }
#       endif
    }
    else
    {
	/* subjob master */
	int sj0_duroc_index;
	int my_duroc_index;
	int my_duroc_addr;
	char * my_rsl_index_str;
	int sj_pg_rank;  /* the first process group rank of the current subjob */
	int sj_index;

	globus_duroc_runtime_inter_subjob_structure(&my_duroc_addr, sj_num, sj_duroc_addrs);
	
	/* finding index of master subjob 0, he is the one with the lowest address */
	sj0_duroc_index = -1;
	my_duroc_index = 0;
	for (sj_index = 0; sj_index < *sj_num; sj_index ++)
	{
	    if ((sj0_duroc_index == -1 && (*sj_duroc_addrs)[sj_index] < my_duroc_addr) ||
		(sj0_duroc_index != -1  && (*sj_duroc_addrs)[sj_index] < (*sj_duroc_addrs)[sj0_duroc_index]))
	    {
		sj0_duroc_index = sj_index;
	    }
	    if ((*sj_duroc_addrs)[sj_index] < my_duroc_addr)
	    {
		my_duroc_index++;
	    }
	}
	
	/* globus_duroc_runtime_inter_subjob_structure reports the number of REMOTE subjobs (those *other* than the subjob for
	   which the local process is the master).  to get the TOTAL number of subjobs in this run the value reported by
	   globus_duroc_runtime_inter_subjob_structure must be incremented by one */
	(*sj_num) ++;

	/* XXX: should not exit here ... should set globus-like rc. */
	my_rsl_index_str = getenv("GLOBUS_DUROC_SUBJOB_INDEX");
	if (my_rsl_index_str == NULL)
	{
	    globus_libc_fprintf(stderr, "ERROR: required environment variable GLOBUS_DUROC_SUBJOB_INDEX not set.\n");
	    globus_libc_fprintf(stderr, "       Each subjob in envoking RSL must have GLOBUS_DUROC_SUBJOB_INDEX\n");
	    globus_libc_fprintf(stderr, "       set to rank (0, 1, 2, ...) of subjob as it appears in the envoking RSL.\n");
	    exit(1);
	}
	
	*my_rsl_index = atoi(my_rsl_index_str);
	if (*my_rsl_index < 0 || *my_rsl_index >= *sj_num)
	{
	    globus_libc_fprintf(stderr, "ERROR: env variable GLOBUS_DUROC_SUBJOB_INDEX %d must be >= 0 and\n", *my_rsl_index);
	    globus_libc_fprintf(stderr, "       less than the number of subjobs %d for this run.\n", *sj_num);
	    exit(1);
	}

	if (my_duroc_index != 0)
	{
	    /* this process is a subjob master, but NOT for subjob 0.  send my subjob information to the master of subjob 0,
	       which will aggregate the information from all subjobs.  once all subjobs have checked in, the size of the process
	       group and the rank of the subjob master within the process group will be received. */
	    sprintf(tag, "%s%d", MPIG_PM_GK_TAG_SJ_MASTER_TO_PG_MASTER_TOPOLOGY, call_count);
	    sprintf((char *) sbuf, "%d %d %d", my_duroc_index, *my_rsl_index, my_sj_size);
	    globus_duroc_runtime_inter_subjob_send((*sj_duroc_addrs)[sj0_duroc_index], tag, (int) strlen((char *) sbuf) + 1, sbuf);

	    sprintf(tag, "%s%d", MPIG_PM_GK_TAG_PG_MASTER_TO_SJ_MASTER_TOPOLOGY, call_count);
	    globus_duroc_runtime_inter_subjob_receive(tag, &rbuf_len, &rbuf);
	    sscanf((char *) rbuf, "%d %d", pg_size, &sj_pg_rank);
	    globus_libc_free(rbuf);
	}
	else
	{
	    /* this process is the master of subjob 0.  this process will receive information from all other subjob masters,
	       compute the size of the process group size and the rank of each subjob master within the process group, and then
	       send that information to each of the subjob masters. */
	    
	    /* all vectors below are of length sj_num and indexed by the duroc_index */
	    int * rsl_indices;
	    int * sj_sizes;
	    int * pg_ranks;

	    /* FIXME: exiting on these failed mallocs is not right thing to do.  should set error rc and return. fix that later
	       when we upgrade to globus errors objects and result codes. */
	    MPIU_CHKLMEM_MALLOC(rsl_indices, int *, *sj_num * sizeof(int), mpi_errno, "array of RSL indices");
	    MPIU_CHKLMEM_MALLOC(sj_sizes, int *, *sj_num * sizeof(int), mpi_errno, "array of subjob sizes");
	    MPIU_CHKLMEM_MALLOC(pg_ranks, int *, *sj_num * sizeof(int), mpi_errno, "array of PG ranks for subjob masters");

	    /* sj_addrs must be sorted so that an incoming rsl_index and sj_size can be associated with the destination in
	       sj_addrs using the associated duroc_index (received below) */
	    for (sj_index = 1; sj_index < *sj_num - 1; sj_index++)
	    {
		int sj_index2;
		int temp;
			
		for (sj_index2 = sj_index; sj_index2 > 0; sj_index2--)
		{
		    if ((*sj_duroc_addrs)[sj_index2] < (*sj_duroc_addrs)[sj_index2 - 1])
		    {
			temp = (*sj_duroc_addrs)[sj_index2];
			(*sj_duroc_addrs)[sj_index2] = (*sj_duroc_addrs)[sj_index2 - 1];
			(*sj_duroc_addrs)[sj_index2 - 1] = temp;
		    }
		}
	    }

	    /* rsl_indices[] and sj_sizes[] are indexed by the duroc subjob index number of the subjob master.  the index of the
	       master subjob is alwasys zero */
	    rsl_indices[0] = *my_rsl_index;
	    sj_sizes[0] = my_sj_size;

	    for (sj_index = 1; sj_index < *sj_num; sj_index++)
	    {
		int duroc_index;
		int rsl_index;
		int sj_size;
		/*
		 * receiving 3 integers from other subjob masters
		 *    duroc_index (used to index rsl_indices[] and sj_sizes[])
		 *    rsl_index
		 *    sj_size
		 */
		sprintf(tag, "%s%d", MPIG_PM_GK_TAG_SJ_MASTER_TO_PG_MASTER_TOPOLOGY, call_count);
		globus_duroc_runtime_inter_subjob_receive(tag, &rbuf_len, &rbuf);
		sscanf((char *) rbuf, "%d %d %d", &duroc_index, &rsl_index, &sj_size);
		rsl_indices[duroc_index] = rsl_index;
		sj_sizes[duroc_index] = sj_size;
		globus_libc_free(rbuf);
	    }

	    /* calculate size of the process group and the rank of each subjob master within the process group.  the formula is
	       as follows: for each subjob sj, pg_rank[sj] = sum of sj_size[n] for all n in which rsl_indices[n] is less than
	       rsl_indices[sj]. */
	    for (sj_index = 0, *pg_size = 0; sj_index < *sj_num; sj_index++)
	    {
		int sj_index2;
		
		(*pg_size) += sj_sizes[sj_index];
		pg_ranks[sj_index] = 0;
		for (sj_index2 = 0; sj_index2 < *sj_num; sj_index2++)
		{
		    if (rsl_indices[sj_index] > rsl_indices[sj_index2])
		    {
			pg_ranks[sj_index] += sj_sizes[sj_index2];
		    }
		}
	    }
	    sj_pg_rank = pg_ranks[0];

	    /* sending other subjob masters pg_size and their g_rank */
	    for (sj_index = 0; sj_index < *sj_num - 1; sj_index++)
	    {
		sprintf(tag, "%s%d", MPIG_PM_GK_TAG_PG_MASTER_TO_SJ_MASTER_TOPOLOGY, call_count);
		sprintf((char *) sbuf, "%d %d", *pg_size, pg_ranks[sj_index + 1]);
		globus_duroc_runtime_inter_subjob_send((*sj_duroc_addrs)[sj_index], tag, (int) strlen((char *) sbuf) + 1, sbuf);
	    }
	    
	    /* endif master of subjob 0 */
	}

	/* all subjob masters sending pg_size and their pg_rank to their slaves */
#       if defined(MPIG_VMPI)
	{
	    int v[4];
	    int vrc;

	    /*
	     * v[0] - the number of processes in the process group
	     * v[1] - the first process group rank for the subjob
	     * v[2] - the number of subjobs in the process group
	     * v[3] - the index of the subjob to which the local process belongs
	     */
	    v[0] = *pg_size;
	    v[1] = sj_pg_rank;
	    v[2] = *sj_num;
	    v[3] = *my_rsl_index;

	    MPIU_Assert(mpig_pm_gk_vmpi_root == mpig_pm_gk_vmpi_cw_rank);
	    
	    vrc = mpig_vmpi_bcast(v, 4, MPIG_VMPI_INT, mpig_pm_gk_vmpi_root, &mpig_pm_gk_vmpi_cw);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Bcast", &mpi_errno);

	    *my_pg_rank = sj_pg_rank + mpig_pm_gk_vmpi_cw_rank;
	}
#       else
        {
	    for (sj_index = 1; sj_index < my_sj_size; sj_index++)
	    {
		sprintf(tag, "%s%d", MPIG_PM_GK_TAG_SJ_MASTER_TO_SJ_SLAVE_TOPOLOGY, call_count);
		sprintf((char *) sbuf, "%d %d %d %d", *pg_size, sj_pg_rank + sj_index, *sj_num, *my_rsl_index);
		mpi_errno = mpig_pm_gk_intra_subjob_send(sj_index, tag, (int) strlen((char *) sbuf) + 1, sbuf);
		MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s", "mpig_pm_gk_intra_subjob_send");
	    }

	    *my_pg_rank = sj_pg_rank;
        }
#       endif
    } /* endif */

  fn_return:
    MPIU_CHKLMEM_FREEALL();
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
	"exiting: pg_size=%d, my_pg_rank=%d, sj_num=%d, my_rsl_index=%d", *pg_size, *my_pg_rank, *sj_num, *my_rsl_index));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_gk_get_topology);
    return mpi_errno;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
} /* mpig_pm_gk_get_topology() */


/*
 * mpig_pm_gk_distribute_byte_array()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_gk_distribute_byte_array
MPIG_STATIC int mpig_pm_gk_distribute_byte_array(const int pg_size, const int pg_rank, const int sj_num, const int my_sj_size,
    const int my_sj_rank, const int * const sj_duroc_addrs, const globus_byte_t * const in_buf, const int in_buf_len,
    globus_byte_t ** const out_bufs, int * const out_bufs_lens)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    static int call_count = 0;
    char tag[MPIG_PM_GK_TAG_MAX_SIZE];
    int i;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(3);
    MPIU_CHKPMEM_DECL(1);
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_gk_distribute_byte_array);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_gk_distribute_byte_array);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
	"entering: pg_size=%d, pg_rank=%d, sj_num=%d, my_sj_size=%d my_sj_rank=%d, in_buf_len=%d",
	pg_size, pg_rank, sj_num, my_sj_size, my_sj_rank, in_buf_len));

    call_count += 1;
    
    /* initialize the array of output buffers */
    for (i = 0; i < pg_size; i ++)
    {
	out_bufs[i] = (globus_byte_t *) NULL;
	out_bufs_lens[i] = 0;
    } /* endfor */

    if (my_sj_rank)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PM, "process is a subjob slave"));
	/* subjob slave */
#       if defined(MPIG_VMPI)
        {
	    int vrc;

	    vrc = mpig_vmpi_gather(&in_buf_len, 1, MPIG_VMPI_INT, (void *) NULL, 1, MPIG_VMPI_INT, mpig_pm_gk_vmpi_root,
		&mpig_pm_gk_vmpi_cw);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Gather", &mpi_errno);

	    vrc = mpig_vmpi_gatherv((char *) in_buf, in_buf_len, MPIG_VMPI_BYTE, (void *) NULL, (int *) NULL, (int *) NULL,
		MPIG_VMPI_BYTE, mpig_pm_gk_vmpi_root, &mpig_pm_gk_vmpi_cw);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Gatherv", &mpi_errno);
        }
#       else /* !defined(MPIG_VMPI) */
        {
	    globus_byte_t * msg_buf;
	    globus_byte_t stack_msg_buf[GRAM_MYJOB_MAX_BUFFER_LENGTH];

	    /* if the pre-allocated message buffer is too small, then allocate a bigger one */
	    if ((int) sizeof(msg_buf) < 2 * INT_MAX_STRLEN + in_buf_len)
	    {
		MPIU_CHKLMEM_MALLOC(msg_buf, globus_byte_t *, 2 * INT_MAX_STRLEN + in_buf_len, mpi_errno,
		    "big message buffer for slave process data");
	    }
	    else
	    {
		msg_buf = stack_msg_buf;
	    }

	    /* copy my byte array into the message buffer, tagging it with my rank */
	    sprintf((char *) msg_buf, "%d ", pg_rank);
	    sprintf((char *) msg_buf + INT_MAX_STRLEN, "%d ", in_buf_len);
	    memcpy((char *) msg_buf + 2 * INT_MAX_STRLEN, in_buf, (size_t) in_buf_len);
	    
	    /* send my byte array to the subjob master */
	    sprintf(tag, "%s%d", MPIG_PM_GK_TAG_SJ_SLAVE_TO_SJ_MASTER_DATA, call_count);
	    mpi_errno = mpig_pm_gk_intra_subjob_gather(my_sj_size, my_sj_rank, msg_buf, 2 * INT_MAX_STRLEN + in_buf_len, tag,
		(int *) NULL, (globus_byte_t **) NULL);
	    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s", "mpig_pm_gk_intra_subjob_gather");

	    MPIU_CHKLMEM_FREEALL();
	}
#       endif /* MPIG_VMPI defined or not defined */
	
	/* receiving all other byte arrays from my master */
	i = 0; 
	while (i < pg_size)
	{
	    globus_byte_t * rbuf;
	    int nbufs;
	    
	    /* globus_libc_fprintf(stderr, "%d: mpig_pm_gk_distribute_byte_array: subjob slave: top of while loop i %d pg_size"
	       " %d\n", MPID_MyWorldRank, i, pg_size); */

#           if defined(MPIG_VMPI)
            {
		int bsize;
		int vrc;

		vrc = mpig_vmpi_bcast(&bsize, 1, MPIG_VMPI_INT, mpig_pm_gk_vmpi_root, &mpig_pm_gk_vmpi_cw);
		MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Bcast", &mpi_errno);

		MPIU_CHKPMEM_MALLOC(rbuf, globus_byte_t *, bsize, mpi_errno, "buffer for receiving intra-subjob data");

		vrc = mpig_vmpi_bcast((void *) rbuf, bsize, MPIG_VMPI_BYTE, mpig_pm_gk_vmpi_root, &mpig_pm_gk_vmpi_cw);
		MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Bcast", &mpi_errno);
	    }
#           else /* !defined(MPIG_VMPI) */
            {
		int rbuf_len;
		
		sprintf(tag, "%s%d", MPIG_PM_GK_TAG_SJ_MASTER_TO_SJ_SLAVE_DATA, call_count);
		/* globus_libc_fprintf(stderr, "%d: mpig_pm_gk_distribute_byte_array: subjob slave: before intra_subjob_bcast\n",
		                       MPID_MyWorldRank); */
		mpi_errno = mpig_pm_gk_intra_subjob_bcast(my_sj_size, my_sj_rank, tag, &rbuf_len, &rbuf);
		MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s",
		    "mpig_pm_gk_intra_subjob_bcast");
		MPIU_CHKPMEM_REGISTER(rbuf);
		/* globus_libc_fprintf(stderr, "%d: mpig_pm_gk_distribute_byte_array: subjob slave: after intra_subjob_bcast\n",
		                       MPID_MyWorldRank); */
	    }
#           endif

	    mpig_pm_gk_extract_byte_arrays(rbuf, &nbufs, out_bufs, out_bufs_lens);
	    MPIU_CHKPMEM_COMMIT();
	    MPIU_Free(rbuf);
	    i += nbufs;
	} /* end while (i < pg_size) */
    }
    else 
    {
	/* subjob master */
	globus_byte_t * my_sj_buf;
	int my_sj_buf_len;
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PM, "process is a subjob master"));

#       if defined(MPIG_VMPI)
        {
	    globus_byte_t * msg_buf;
	    globus_byte_t * sj_buf_cur_ptr;
	    int * rcounts;
	    int * displs;
	    int vrc;

	    MPIU_CHKLMEM_MALLOC(rcounts, int *, my_sj_size * sizeof(int), mpi_errno, "size of slave data");
	    MPIU_CHKLMEM_MALLOC(displs, int *, my_sj_size * sizeof(int), mpi_errno, "buffer displacements for slave data");

	    /* acquire the size of the byte arrays arriving from the slave processes in my subjob */
	    vrc = mpig_vmpi_gather(&in_buf_len, 1, MPIG_VMPI_INT, rcounts, 1, MPIG_VMPI_INT, mpig_pm_gk_vmpi_root,
		&mpig_pm_gk_vmpi_cw);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Gather", &mpi_errno);

	    /* compute the buffer displacement for the byte arrays arriving from each process */
	    my_sj_buf_len = displs[0] = 0;
	    for (i = 0; i < my_sj_size; i ++)
	    {
		my_sj_buf_len += rcounts[i];
		if (i)
		{
		    displs[i] = displs[i - 1] + rcounts[i - 1];
		}
	    }

	    /* acquire the byte arrays from the slave processes in my subjob */
	    MPIU_CHKLMEM_MALLOC(msg_buf, globus_byte_t *, my_sj_buf_len, mpi_errno, "temp buffer for subjob data");

	    vrc = mpig_vmpi_gatherv((void *) in_buf, in_buf_len, MPIG_VMPI_BYTE, msg_buf, rcounts, displs, MPIG_VMPI_BYTE,
		mpig_pm_gk_vmpi_root, &mpig_pm_gk_vmpi_cw);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Gatherv", &mpi_errno);

	    /* construct the header for a message containing the data for my subjob so that it may be sent to the process group
	       master */
	    my_sj_buf_len += INT_MAX_STRLEN + my_sj_size * 2 * INT_MAX_STRLEN;
	    MPIU_CHKPMEM_MALLOC(my_sj_buf, globus_byte_t *, my_sj_buf_len, mpi_errno, "buffer for subjob data");

	    sprintf((char *) my_sj_buf, "%d ", my_sj_size);
	    sprintf((char *) my_sj_buf + INT_MAX_STRLEN, "%d ", pg_rank);
	    sprintf((char *) my_sj_buf + 2 * INT_MAX_STRLEN,"%d ", in_buf_len);

	    /* add my byte array to the message */
	    memcpy(my_sj_buf+ 3 * INT_MAX_STRLEN,  in_buf, (size_t) in_buf_len);

	    /* add the byte arrays of the other processes to the message.  include the ranks and sizes of the byte arrays so that
	       message can be unpacked later. */
	    sj_buf_cur_ptr = my_sj_buf + 3 * INT_MAX_STRLEN + in_buf_len;
	    for (i = 1; i < my_sj_size; i ++)
	    {
		sprintf((char *) sj_buf_cur_ptr, "%d ", pg_rank + i);
		sprintf((char *) sj_buf_cur_ptr + INT_MAX_STRLEN, "%d ", rcounts[i]);
		memcpy((char *) sj_buf_cur_ptr + 2 * INT_MAX_STRLEN, msg_buf + displs[i], (size_t) rcounts[i]);
		sj_buf_cur_ptr += 2 * INT_MAX_STRLEN + rcounts[i];
	    }

	    /* free the temporary allocations (rcounts, displs, and msg_buf) */
	    MPIU_CHKLMEM_FREEALL();
	}
#       else
	{
	    /* construct a message containing my data, and gather the data from all other processes in my subjob */
	    globus_byte_t * msg_buf;
	    int msg_buf_len = 3 * INT_MAX_STRLEN + in_buf_len;

	    MPIU_CHKLMEM_MALLOC(msg_buf, globus_byte_t *, msg_buf_len, mpi_errno, "buffer for local data message");

	    sprintf((char *) msg_buf, "%d ", my_sj_size);
	    sprintf((char *) msg_buf + INT_MAX_STRLEN, "%d ", pg_rank);
	    sprintf((char *) msg_buf + 2 * INT_MAX_STRLEN, "%d ", in_buf_len);
	    memcpy(msg_buf + 3 * INT_MAX_STRLEN, in_buf, (size_t) in_buf_len);

	    sprintf(tag, "%s%d", MPIG_PM_GK_TAG_SJ_SLAVE_TO_SJ_MASTER_DATA, call_count);

	    mpi_errno = mpig_pm_gk_intra_subjob_gather(my_sj_size, my_sj_rank, msg_buf, msg_buf_len, tag,
		&my_sj_buf_len, &my_sj_buf);
	    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s", "mpig_pm_gk_intra_subjob_gather");

	    /* add the subjob data buffer to the list of allocated memory to be reaped if a failure occurs */
	    MPIU_CHKPMEM_REGISTER(my_sj_buf);
	    
	    /* free the temporary memory allocations (msg_buf) */
	    MPIU_CHKLMEM_FREEALL();
	}
#       endif

	mpig_pm_gk_extract_byte_arrays(my_sj_buf, (int *) NULL, out_bufs, out_bufs_lens);

#       if defined(MPIG_VMPI)
        {
	    int vrc;

	    /* send the size of my subjob's data buffer holding the byte arrays of all processes in the subjob */
	    vrc = mpig_vmpi_bcast(&my_sj_buf_len, 1, MPIG_VMPI_INT, mpig_pm_gk_vmpi_root, &mpig_pm_gk_vmpi_cw);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Bcast", &mpi_errno);

	    /* broadcast the subjob data buffer to all processes in the subjob */
	    vrc = mpig_vmpi_bcast(my_sj_buf, my_sj_buf_len, MPIG_VMPI_BYTE, mpig_pm_gk_vmpi_root, &mpig_pm_gk_vmpi_cw);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Bcast", &mpi_errno);
	}
#       else
        {
	    /* sending inter-subjob message for MY subjob to all my slaves */
	    sprintf(tag, "%s%d", MPIG_PM_GK_TAG_SJ_MASTER_TO_SJ_SLAVE_DATA, call_count);
	    /* globus_libc_fprintf(stderr, "%d: mpig_pm_gk_distribute_byte_array: subjob master: before intra_subjob_bcast our subjob\n",
	                           MPID_MyWorldRank); */
	    mpi_errno = mpig_pm_gk_intra_subjob_bcast(my_sj_size, my_sj_rank, tag, &my_sj_buf_len, &my_sj_buf);
	    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s", "mpig_pm_gk_intra_subjob_bcast");

	    /* globus_libc_fprintf(stderr, "%d: mpig_pm_gk_distribute_byte_array: subjob master: after intra_subjob_bcast our subjob\n",
	                           MPID_MyWorldRank); */
	}
#       endif

	/* send message containing the byte arrays for all processes in my subjob to the other subjob masters */
	sprintf(tag, "%s%d", MPIG_PM_GK_TAG_SJ_MASTER_TO_SJ_MASTER_DATA, call_count);
	for (i = 0; i < sj_num - 1; i++)
	{
	    globus_duroc_runtime_inter_subjob_send(sj_duroc_addrs[i], tag, my_sj_buf_len, my_sj_buf);
	}

	MPIU_CHKPMEM_COMMIT();
	MPIU_Free(my_sj_buf);

	/* receiving subjob byte arrays from other subjob masters */
	for (i = 0; i < sj_num - 1; i++)
	{
	    globus_byte_t * sj_buf;
	    int sj_buf_len;
	    
/* globus_libc_fprintf(stderr, "%d: mpig_pm_gk_distribute_byte_array: subjob master: top of for loop i %d sj_size-1 %d\n",
   MPID_MyWorldRank, i, sj_size-1); */
	    sprintf(tag, "%s%d", MPIG_PM_GK_TAG_SJ_MASTER_TO_SJ_MASTER_DATA, call_count);
	    globus_duroc_runtime_inter_subjob_receive(tag, &sj_buf_len, &sj_buf);

	    /* broadcast subjob byte arrays to all my slaves processes in my subjob*/
#           if defined(MPIG_VMPI)
	    {
		int vrc;

		vrc = mpig_vmpi_bcast(&sj_buf_len, 1, MPIG_VMPI_INT, mpig_pm_gk_vmpi_root, &mpig_pm_gk_vmpi_cw);
		MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Bcast", &mpi_errno);

		vrc = mpig_vmpi_bcast(sj_buf, sj_buf_len, MPIG_VMPI_BYTE, mpig_pm_gk_vmpi_root, &mpig_pm_gk_vmpi_cw);
		MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Bcast", &mpi_errno);
	    }
#           else
	    {
		sprintf(tag, "%s%d", MPIG_PM_GK_TAG_SJ_MASTER_TO_SJ_SLAVE_DATA, call_count);
		/* globus_libc_fprintf(stderr, "%d: mpig_pm_gk_distribute_byte_array: subjob master: before intra_subjob_bcast"
		   " other subjob\n", MPID_MyWorldRank); */
		mpi_errno = mpig_pm_gk_intra_subjob_bcast(my_sj_size, my_sj_rank, tag, &sj_buf_len, &sj_buf);
		MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s",
		    "mpig_pm_gk_intra_subjob_bcast");
		/* globus_libc_fprintf(stderr, "%d: mpig_pm_gk_distribute_byte_array: subjob master: after intra_subjob_bcast"
		   " other subjob\n", MPID_MyWorldRank); */
	    }
#           endif

	    mpig_pm_gk_extract_byte_arrays(sj_buf, (int *) NULL, out_bufs, out_bufs_lens);
	    
	    globus_libc_free(sj_buf);
	} /* end for all other subjob masters */
    } /* end if subjob slave or master */

/* globus_libc_fprintf(stderr, "%d: exit mpig_pm_gk_distribute_byte_array: sj_rank %d %d bytes\n", MPID_MyWorldRank, sj_rank, in_buf_len); */

  fn_return:
    MPIU_CHKLMEM_FREEALL();
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: mpi_errno=%d", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_gk_distribute_byte_array);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	MPIU_CHKPMEM_REAP();
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_gk_distribute_byte_array() */


#undef FUNCNAME
#define FUNCNAME mpig_pm_gk_extract_byte_arrays
MPIG_STATIC int mpig_pm_gk_extract_byte_arrays(const globus_byte_t * const rbuf, int * const nbufs_p /* optional */,
    globus_byte_t ** const out_bufs,
    int * out_buf_lens)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const globus_byte_t * src;
    int nbufs;
    int buf_index;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_gk_extract_byte_arrays);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_gk_extract_byte_arrays);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));
	
    src = rbuf + INT_MAX_STRLEN;
    sscanf((char *) rbuf, "%d ", &nbufs);
    if (nbufs_p)
    {
	*nbufs_p = nbufs;
    }

    for (buf_index = 0; buf_index < nbufs; buf_index++)
    {
	int pg_rank;

	sscanf((char *) src, "%d ", &pg_rank);
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PM, "extracting byte array for rank=%d\n", pg_rank));

	 /* FIME: create a real error code to be returned */
	if (out_bufs[pg_rank])
	{
	    globus_libc_fprintf(stderr, "ERROR(%d): just rcvd second byte array from %d\n", mpig_process.my_pg_rank, pg_rank);
	    exit(1);
	}

	sscanf((char *) src + INT_MAX_STRLEN, "%d ", out_buf_lens+pg_rank);
	
	/* FIME: create a real error code to be returned */
	out_bufs[pg_rank] = (globus_byte_t *) MPIU_Malloc(out_buf_lens[pg_rank]);
	if (out_bufs[pg_rank] == NULL)
	{
	    globus_libc_fprintf(stderr, "ERROR: failed malloc of %d bytes\n", out_buf_lens[pg_rank]);
	    exit(1);
	}

	memcpy(out_bufs[pg_rank], src + 2 * INT_MAX_STRLEN, (size_t) out_buf_lens[pg_rank]);

	src += 2 * INT_MAX_STRLEN + out_buf_lens[pg_rank];
    } /* end: for all byte arrays in rbuf */

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: nbufs=%d, mpi_errno=%d", nbufs, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_gk_extract_byte_arrays);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	/* FIMXE: free any out_bufs already allocated */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_gk_extract_byte_arrays() */


#if !defined(MPIG_VMPI)
#undef FUNCNAME
#define FUNCNAME mpig_pm_gk_intra_subjob_send
MPIG_STATIC int mpig_pm_gk_intra_subjob_send(const int dest, const char * const tag_base, const int nbytes,
    const globus_byte_t * const buf)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    char tag_buf[MPIG_PM_GK_TAG_MAX_SIZE];
    char * tag;
    int chunk;
    /* NICK: This is a hack because globus_duroc_runtime_intra_subjob_send
     *       dictates that the (tag size + message size) must fit into a buffer
     *       the size of GRAM_MYJOB_MAX_BUFFER_LENGTH-10 and they ain't 
     *       likely gonna fix this Globus code ever ... they've moved on 
     *       to Web-services and have abandonded all this DUROC code for good.
     */
    const int max_payload_size = GRAM_MYJOB_MAX_BUFFER_LENGTH - 10 - strlen(tag_base) - 5;
    char send_buf[max_payload_size];
    const globus_byte_t * src;
    int bytes_sent;
    int ncpy;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(1);
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_gk_intra_subjob_send);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_gk_intra_subjob_send);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
	"enter: dest=%d, tag_base=%s, nbytes=%d", dest, tag_base, nbytes));

    /* if the pre-allocated tag buffer is too small, then allocate a bigger one */
    if (strlen(tag_base) + 5 > sizeof(tag))
    {
	MPIU_CHKLMEM_MALLOC(tag, char *, strlen(tag_base) + 5, mpi_errno, "buffer for large tag");
    }
    else
    {
	tag = tag_buf;
    }

    /* sending as much as i can in the first buffer */
    sprintf(send_buf, "%d ", nbytes);
    ncpy = max_payload_size - INT_MAX_STRLEN < nbytes ? max_payload_size - INT_MAX_STRLEN : nbytes;
    memcpy(send_buf + INT_MAX_STRLEN, buf, (size_t) ncpy);

    sprintf(tag, "%s0", tag_base);
    globus_duroc_runtime_intra_subjob_send(dest, tag, INT_MAX_STRLEN + ncpy, (globus_byte_t *) send_buf);

    /* pushing out remaining data */
    bytes_sent = ncpy;
    src = buf + ncpy;
    chunk = 1;
    while (bytes_sent < nbytes)
    {
	ncpy = max_payload_size < nbytes-bytes_sent ? max_payload_size : nbytes - bytes_sent;
	memcpy(send_buf, src, (size_t) ncpy);
	sprintf(tag, "%s%d", tag_base, chunk);
	globus_duroc_runtime_intra_subjob_send(dest, tag, ncpy, (globus_byte_t *) send_buf);

	bytes_sent += ncpy;
	src += ncpy;
	chunk += 1;
    }

  fn_return:
    MPIU_CHKLMEM_FREEALL();
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: mpi_errno=%d", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_gk_intra_subjob_send);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_gk_intra_subjob_send() */


#undef FUNCNAME
#define FUNCNAME mpig_pm_gk_intra_subjob_receive
MPIG_STATIC int mpig_pm_gk_intra_subjob_receive(const char * const tag_base, int * const rcvd_nbytes, globus_byte_t ** const buf)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    char tag_buf[MPIG_PM_GK_TAG_MAX_SIZE];
    char * tag;
    int chunk;
    char rcv_buf[GRAM_MYJOB_MAX_BUFFER_LENGTH];
    int nr;
    globus_byte_t * dest;
    int bytes_rcvd;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(1);
    MPIU_CHKPMEM_DECL(1);
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_gk_intra_subjob_receive);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_gk_intra_subjob_receive);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "enter: tag_base=%s", tag_base));

    /* if the pre-allocated tag buffer is too small, then allocate a bigger one */
    if (strlen(tag_base) + 5 > sizeof(tag))
    {
	MPIU_CHKLMEM_MALLOC(tag, char *, strlen(tag_base) + 5, mpi_errno, "buffer for large tag");
    }
    else
    {
	tag = tag_buf;
    }

    /* receive as the first chuck of the message.  extract the size of the whole message, and allocate a receive buffer big
       enough to hold it */
    sprintf(tag, "%s0", tag_base);
    globus_duroc_runtime_intra_subjob_receive(tag, &nr, (globus_byte_t *) rcv_buf);
    sscanf(rcv_buf, "%d ", rcvd_nbytes);
    MPIU_CHKPMEM_MALLOC(*buf, globus_byte_t *, *rcvd_nbytes, mpi_errno, "receive buffer");

    /* copy the first chuck of the message into the receive buffer */
    memcpy(*buf, rcv_buf + INT_MAX_STRLEN, (size_t) nr - INT_MAX_STRLEN);

    /* receiving remaining data */
    bytes_rcvd = nr - INT_MAX_STRLEN;
    dest = *buf+(nr - INT_MAX_STRLEN);
    chunk = 1;
    while (bytes_rcvd < *rcvd_nbytes)
    {
	sprintf(tag, "%s%d", tag_base, chunk);
	globus_duroc_runtime_intra_subjob_receive(tag, &nr, (globus_byte_t *) rcv_buf);
	memcpy(dest, rcv_buf, (size_t) nr);

	bytes_rcvd += nr;
	dest += nr;
	chunk += 1;
    }

  fn_return:
    MPIU_CHKLMEM_FREEALL();
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
	"exiting: rcvd_nbytes=%d, mpi_errno=%d", *rcvd_nbytes, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_gk_intra_subjob_receive);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	MPIU_CHKPMEM_REAP();
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_gk_intra_subjob_receive() */


/*
 *
 * NOTE: both bcast/gather assumes root is always my_sj_rank=0
 *
 * for non-vMPI environments this function uses globus functions
 * to distribute information (both bcast/gather from/to subjob masters).
 * all such bcasts/gathers are done by configuring the procs within a
 * subjob in a binomial tree.  def of binomial tree:
 *   A binomial tree, B_k (k>=0), is an ordered tree of order k.  The root
 *   has k children (ordered from left to right) where each child is
 *   binomial tree B_(k-1), B_(k-2), ..., B_0.
 * note a B_k tree 
 *     - has 2**k nodes
 *     - height = k
 *     - there are k choose i nodes at dept i
 ****************************************************************************
 * ex
 *     B_0    root            =     0
 *
 *     B_1    root            =     0
 *             |                    | 
 *            B_0                   0
 *
 *     B_2    root            =          0
 *             |                         |
 *        +----+----+               +----+----+
 *        |         |               |         |
 *       B_1       B_0              0         0
 *                                  |      
 *                                  0
 *     B_3    root            =             0
 *             |                            |
 *        +----+----+               +-------+----+
 *        |    |    |               |       |    |
 *       B_2  B_1  B_0              0       0    0
 *                                  |       |      
 *                              +---+---+   |
 *                              |       |   |
 *                              0       0   0
 *                              |
 *                              0
 ****************************************************************************
 *   This uses a fairly basic recursive subdivision algorithm.
 *   The root sends to the process size/2 away; the receiver becomes
 *   a root for a subtree and applies the same process. 
 *
 *   So that the new root can easily identify the size of its
 *   subtree, the (subtree) roots are all powers of two (relative to the root)
 *   If m = the first power of 2 such that 2^m >= the size of the
 *   communicator, then the subtree at root at 2^(m-k) has size 2^k
 *   (with special handling for subtrees that aren't a power of two in size).
 *
 *
 * basically, consider every rank as a binary number.  find the least
 * significant (rightmost) '1' bit (LSB) in each such rank.  so, for each
 * rank you have the following bitmask:
 *
 *                 xxxx100...0
 *                 ^^^^^^^^^^^
 *                 |   |   |
 *         0 or more  LSB  +---- followed by zero or more 0's
 *          leading
 *          bits
 *  or
 *                 0.........0 (root only)
 *
 * each non-root node receives from the src who has the same rank
 * BUT has the LSB=0, so, 4 (100), 2 (01), and 1 (1) all rcv from 0.
 * 5 (101) and 6 (110) both rcv from 4 (100).
 * the sends are ordered from left->right by sending first to the
 * bit to the immediate right of the LSB.  so, 4 (100) sends first
 * to 6 (110) and then to 5 (101).  
 *
 * note that under this scheme the root (0) receives from nobody
 * and all the leaf nodes (sending to nobody) are the odd-numbered nodes.
 *   
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_gk_intra_subjob_bcast
MPIG_STATIC int mpig_pm_gk_intra_subjob_bcast(const int my_sj_size, const int my_sj_rank, const char * const tag_base,
    int * rcvd_nbytes, globus_byte_t ** buf)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mask;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_gk_intra_subjob_bcast);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_gk_intra_subjob_bcast);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
	"enter: my_sj_size=%d, my_sj_rank=%d, tag_base=%s", my_sj_size, my_sj_rank, tag_base));
    
    /*
     * Do subdivision.  There are two phases:
     *
     * 1. Wait for arrival of data.  Because of the power of two nature of the subtree roots, the source of this message is
     *    alwyas the process whose relative rank has the least significant bit CLEARED.  That is, process 4 (100) receives from
     *    process 0, process 7 (111) from process 6 (110), etc.
     *
     * 2. Forward to my subtree
     *
     * NOTE: the process that is the tree root is handled automatically by this code, since it has no bits set.
     */

    /* loop to find LSB */
    for (mask = 0x1; mask < my_sj_size && !(my_sj_rank & mask);  mask <<= 1);

    if (my_sj_rank & mask) 
    {
	/* i am not root */
	mpi_errno = mpig_pm_gk_intra_subjob_receive(tag_base, rcvd_nbytes, buf);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s", "mpig_pm_gk_intra_subjob_receive");
    }

    /*
     *   This process is responsible for all processes that have bits set from
     *   the LSB upto (but not including) mask.  Because of the "not including",
     *   we start by shifting mask back down one.
     */
    mask >>= 1;
    while (mask > 0) 
    {
	if (my_sj_rank + mask < my_sj_size) 
	{
	    mpi_errno = mpig_pm_gk_intra_subjob_send(my_sj_rank + mask, tag_base, *rcvd_nbytes, *buf);
	    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s", "mpig_pm_gk_intra_subjob_send");
	}
	mask >>= 1;
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
	"exiting: rcvd_nbytes=%d, mpi_errno=%d", *rcvd_nbytes, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_gk_intra_subjob_bcast);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_gk_intra_subjob_bcast() */


#undef FUNCNAME
#define FUNCNAME mpig_pm_gk_intra_subjob_gather
MPIG_STATIC int mpig_pm_gk_intra_subjob_gather(const int my_sj_size, const int my_sj_rank, const globus_byte_t * in_buf,
    const int in_buf_len, const char * const tag_base, int * const rcvd_nbytes, globus_byte_t ** const buf)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mask;
    globus_byte_t * my_sj_buf = NULL;
    int my_sj_buf_len;
    globus_byte_t * dest;
    char * tag;
    globus_byte_t * rbuf = NULL;
    int rbuf_len;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(1);
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_gk_intra_subjob_gather);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_gk_intra_subjob_gather);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
	"enter: my_sj_size=%d, my_sj_rank=%d, in_buf_len=%d, tag_base=%s", my_sj_size, my_sj_rank, in_buf_len, tag_base));
    

    MPIU_CHKLMEM_MALLOC(tag, char *, strlen(tag_base) + 10, mpi_errno, "buffer for tag");

    /* 
     * take a guess of how big my_sj_buf needs to be based on my in_buf size and the size of my subjob
     */

    my_sj_buf_len = my_sj_size * in_buf_len + 100;
    my_sj_buf = (globus_byte_t *) MPIU_Malloc(my_sj_buf_len);
    MPIU_ERR_CHKANDJUMP1((my_sj_buf == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "gather buffer");

    memcpy(my_sj_buf,  in_buf, (size_t) in_buf_len);
    dest = my_sj_buf + in_buf_len;

    for (mask = 0x1; mask < my_sj_size && !(my_sj_rank & mask); mask <<= 1)
    {
	if (my_sj_rank + mask < my_sj_size)
	{
	    sprintf(tag, "%s%d", tag_base, my_sj_rank + mask);
	    mpi_errno = mpig_pm_gk_intra_subjob_receive(tag, &rbuf_len, &rbuf);
	    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s", "mpig_pm_gk_intra_subjob_receive");

	    if (my_sj_buf_len - (dest - my_sj_buf) < rbuf_len)
	    {
		/* have to re-alloc */
		int disp = dest-my_sj_buf;

		my_sj_buf_len += my_sj_size * in_buf_len + 100;
		my_sj_buf = (globus_byte_t *) MPIU_Realloc(my_sj_buf, (size_t) my_sj_buf_len);
		MPIU_ERR_CHKANDJUMP1((my_sj_buf == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "gather buffer");

		dest = my_sj_buf + disp;
	    }

	    memcpy(dest, rbuf, (size_t) rbuf_len);
	    dest += rbuf_len;

	    MPIU_Free(rbuf);
	    rbuf = NULL;
	}
    }

    if (my_sj_rank)
    {
	/* subjob slave */
	sprintf(tag, "%s%d", tag_base, my_sj_rank);
	mpi_errno = mpig_pm_gk_intra_subjob_send(my_sj_rank - mask, tag, (int)(dest - my_sj_buf), my_sj_buf);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %s", "mpig_pm_gk_intra_subjob_send");
	MPIU_Free(my_sj_buf);
	my_sj_buf = NULL;
    } 
    else
    {
	/* subjob master */
	*rcvd_nbytes = dest - my_sj_buf;
	*buf = my_sj_buf;
    }

  fn_return:
    MPIU_CHKLMEM_FREEALL();
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
	"exiting: rcvd_nbytes=%d, mpi_errno=%d", *rcvd_nbytes, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_gk_intra_subjob_gather);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	if (rbuf != NULL) MPIU_Free(rbuf);
	if (my_sj_buf != NULL) MPIU_Free(my_sj_buf);
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_gk_intra_subjob_gather() */

#endif /* !defined(MPIG_VMPI) */
