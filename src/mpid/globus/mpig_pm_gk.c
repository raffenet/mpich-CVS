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

#define INT_MAX_STRLEN 12

/* inter- and intra-subjob message tags */
#define SUBJOB_MAX_TAG_SIZE		  64
#define SUBJOB_MASTER_TO_SUBJOB0_MASTER_T "subjob mstr to subjob0 mstr topology"
#define SUBJOB0_MASTER_TO_SUBJOB_MASTER_T "subjob0 mstr to subjob mstr topology"
#define SUBJOB_MASTER_TO_SLAVE_T          "subjob mstr to slave topology"
#define SUBJOB_SLAVE_TO_MASTER_D          "subjob slave to master data"
#define SUBJOB_MASTER_TO_SUBJOB_MASTER_D  "subjob master to subjob master data"
#define SUBJOB_MASTER_TO_SLAVE_D          "subjob master to slave data"

MPIG_STATIC enum
{
    MPIG_PM_STATE_UNINITIALIZED = 0,
    MPIG_PM_STATE_INITIALIZED,
    MPIG_PM_STATE_GOT_PG_INFO,
    MPIG_PM_STATE_FINALIZED
}
mpig_pm_state = MPIG_PM_STATE_UNINITIALIZED;
MPIG_STATIC int mpig_pm_my_pg_size;
MPIG_STATIC int mpig_pm_my_pg_rank;
MPIG_STATIC const char * mpig_pm_my_pg_id;
MPIG_STATIC int mpig_pm_my_sj_size;
MPIG_STATIC int mpig_pm_my_sj_rank;
MPIG_STATIC int mpig_pm_sj_num;
MPIG_STATIC int * mpig_pm_sj_addrs;

#if FOO
MPIG_STATIC int get_topology(int sj_rank, int sj_size, int * pg_size, int * pg_rank, int *sj_num, int ** sj_addrs);

MPIG_STATIC int distribute_byte_array(int pg_size, int pg_rank, int sj_size, int sj_rank, int sj_num, int * sj_addrs,
				      globus_byte_t * inbuf, int inbuf_len, globus_byte_t ** outbufs, int * outbufs_lens);

#if !defined(MPIG_VMPI)
MPIG_STATIC void intra_subjob_send(int dest, char * tag_base, int nbytes,char * buf);
MPIG_STATIC void intra_subjob_receive(char * tag_base, int * nbytes,char ** buf);
#endif

MPIG_STATIC void extract_byte_arrays(char * rbuf, int * nbufs_p, globus_byte_t ** outbufs, int * outbufs_lens);

MPIG_STATIC void intra_subjob_bcast(int sj_size,int sj_rank, char *tag_base, int * nbytes, char ** buf);

/* nbytes and buf are relevant only on the subjob master */
MPIG_STATIC void intra_subjob_gather(int sj_size, int sj_rank, char * inbuf, int inbuf_len, char * tag_base,
				     int * nbytes, char ** buf);

#else /* !FOO */

MPIG_STATIC void get_topology(int rank_in_my_subjob, int my_subjob_size, int **subjob_addresses, int *nprocs,
			       int *nsubjobs, int *my_grank);

MPIG_STATIC void distribute_byte_array(globus_byte_t *inbuff, int inbufflen, int rank_in_my_subjob, int my_subjob_size,
				       int *subjob_addresses, int nprocs, int nsubjobs, int my_grank,
				       globus_byte_t **outbuffs, int *outbufflens);

#if !defined(MPIG_VMPI)
#define HEADERLEN 20
MPIG_STATIC void intra_subjob_send(int dest, char *tag_base, int nbytes,char *buff);
MPIG_STATIC void intra_subjob_receive(char *tag_base, int *rcvd_nbytes,char **buff);
#endif

MPIG_STATIC void extract_byte_arrays(char *rbuff, int *nbuffs_p, globus_byte_t **outbuffs, int *outbufflens);

#if !defined(MPIG_VMPI)
MPIG_STATIC void intra_subjob_bcast(int rank_in_my_subjob, int my_subjob_size, char *tag_base, int *rcvd_nbytes, char **buff);
MPIG_STATIC void intra_subjob_gather(int rank_in_my_subjob, int my_subjob_size, char *inbuff, int inbufflen, char *tag_base, 
				int *rcvd_nbytes, char **buff);
#endif /* !defined(MPIG_VMPI) */

#endif /* FOO */


/*
 * mpig_pm_init()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_init
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_pm_init(void)
{
    enum
    {
	FUNC_STATE_START,
	FUNC_STATE_DUROC_INIT,
	FUNC_STATE_NEXUS_INIT,
	
    } func_state = FUNC_STATE_START;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_init);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_init);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_state == MPIG_PM_STATE_FINALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_finalized");
    
    /*
     * Activate the DUROC runtime module, and wait for all other processes to startup
     */
    rc = globus_module_activate(GLOBUS_DUROC_RUNTIME_MODULE);
    MPIU_ERR_CHKANDJUMP1((rc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|module_activate",
			 "**globus|module_activate %s", "DUROC runtime");
    func_state = FUNC_STATE_DUROC_INIT;
    
    globus_duroc_runtime_barrier();

    /*
     * XXX: rephrase...
     *
     * We have to activate the Nexus and disable fault tolerance because the DUROC bootstrap module uses Nexus _AND_ insists on
     * keeping Nexus activated during the entire computation (even though DUROC only uses Nexus when we use DUROC, that is,
     * during initialization).  The problem here is that when a remote proc dies a bunch of Nexus error messages gets generated
     * (because FD's in nexus EP's are being closed) AND our proc is forced to abort because nexus_fatal gets called too.  By
     * registering NULL Nexus error handlers we not only prevent the annoying Nexus error messages, but we also prevent our proc
     * terminating just becase a remote proc terminated.
     */
    rc = globus_module_activate(GLOBUS_NEXUS_MODULE);
    MPIU_ERR_CHKANDJUMP1((rc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|module_activate",
			 "**globus|module_activate %s", "Nexus");
    func_state = FUNC_STATE_NEXUS_INIT;

    nexus_enable_fault_tolerance (NULL, NULL);
    
    /*
     * get the topology info, including the process group size and the rank of this process in that group
     */
    rc = globus_duroc_runtime_intra_subjob_size(&mpig_pm_my_sj_size);
    MPIU_ERR_CHKANDJUMP((rc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|duroc_sjsize");

    rc = globus_duroc_runtime_intra_subjob_rank(&mpig_pm_my_sj_rank);
    MPIU_ERR_CHKANDJUMP((rc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|duroc_sjrank");

#if FOO
    rc = get_topology(mpig_pm_my_sj_size, mpig_pm_my_sj_rank, &mpig_pm_my_pg_size, &mpig_pm_my_pg_rank,
		      &mpig_pm_sj_num, &mpig_pm_sj_addrs);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
#else
    get_topology(mpig_pm_my_sj_rank, mpig_pm_my_sj_size, &mpig_pm_sj_addrs, &mpig_pm_my_pg_size, &mpig_pm_sj_num,
		 &mpig_pm_my_pg_rank);
#endif
    
    /* XXX: this needs to be set to some real before we implement MPI-2 functionality */
    mpig_pm_my_pg_id = "XXX";

    /* We already have all of the PG information, but since implemenations are not required to have the info until after
       mpig_pm_exchange_business_cards() is called, we set the state to INITIALIZED to catch any usage bugs. */
    mpig_pm_state = MPIG_PM_STATE_INITIALIZED;
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_init);
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
/* mpig_pm_init() */


/*
 * mpig_pm_finalize()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_finalize
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_pm_finalize(void)
{
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_finalize);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_finalize);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_state == MPIG_PM_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_state == MPIG_PM_STATE_FINALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_finalized");
    
    mpig_pm_state = MPIG_PM_STATE_FINALIZED;

    /*
     * Deactivate the DUROC and Nexus modules.  Remember, Nexus was activated in init to prevent excessive and confusing error
     * messages.
     */
    rc = globus_module_deactivate(GLOBUS_NEXUS_MODULE);
    MPIU_ERR_CHKANDJUMP1((rc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|module_deactivate",
			 "**globus|module_deactivate %s", "Nexus");

    rc = globus_module_deactivate(GLOBUS_DUROC_RUNTIME_MODULE);
    MPIU_ERR_CHKANDJUMP1((rc != GLOBUS_SUCCESS), mpi_errno, MPI_ERR_OTHER, "**globus|module_deactivate",
			 "**globus|module_deactivate %s", "DUROC runtime");

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_finalize);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_pm_finalize() */


/*
 * mpig_pm_exchange_business_cards()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_exchange_business_cards
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_pm_exchange_business_cards(mpig_bc_t * bc, mpig_bc_t ** bcs_ptr)
{
    MPIU_CHKLMEM_DECL(2);
    MPIU_CHKPMEM_DECL(1);
    char * bc_str;
    globus_byte_t ** bc_strs;
    int * bc_lens;
    mpig_bc_t * bcs;
    int p;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_distribute_byte_array);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_distribute_byte_array);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_state == MPIG_PM_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_state == MPIG_PM_STATE_FINALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_finalized");
    
    mpig_bc_serialize_object(bc, &bc_str);
    MPIU_CHKLMEM_MALLOC(bc_strs, globus_byte_t **, mpig_pm_my_pg_size * sizeof(globus_byte_t *), mpi_errno,
			"array of serialized business cards");
    MPIU_CHKLMEM_MALLOC(bc_lens, int *, mpig_pm_my_pg_size * sizeof(int), mpi_errno, "array of business cards lengths");
    MPIU_CHKPMEM_MALLOC(bcs, mpig_bc_t *, mpig_pm_my_pg_size * sizeof(mpig_bc_t), mpi_errno, "array of business cards lengths");
    
#if FOO
    mpi_errno = distribute_byte_array(mpig_pm_my_pg_size, mpig_pm_my_pg_rank, mpig_pm_my_sj_size, mpig_pm_my_sj_rank,
				      mpig_pm_sj_num, mpig_pm_sj_addrs, inbuf, inbuf_len, outbufs, outbufs_lens);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
#else
    distribute_byte_array(bc_str, strlen(bc_str) + 1, mpig_pm_my_sj_rank, mpig_pm_my_sj_size, mpig_pm_sj_addrs,
			  mpig_pm_my_pg_size, mpig_pm_sj_num, mpig_pm_my_pg_rank, bc_strs, bc_lens);

#endif    

    for (p = 0; p < mpig_pm_my_pg_size; p++)
    {
	mpig_bc_deserialize_object((char *) bc_strs[p], &bcs[p]);
	globus_libc_free(bc_strs[p]);
    }

    mpig_bc_free_serialized_object(bc_str);
    MPIU_CHKPMEM_COMMIT();
    *bcs_ptr = bcs;
    
    mpig_pm_state = MPIG_PM_STATE_GOT_PG_INFO;
    
  fn_return:
    MPIU_CHKLMEM_FREEALL();
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_distribute_byte_array);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIU_CHKPMEM_REAP();
    *bcs_ptr = NULL;
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_pm_exchange_business_cards() */


/*
 * mpig_pm_free_business_cards()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_free_business_cards
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_pm_free_business_cards(mpig_bc_t * bcs)
{
    int p;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_free_business_cards);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_free_business_cards);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_state == MPIG_PM_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_state == MPIG_PM_STATE_FINALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_finalized");
    
    for (p = 0; p < mpig_pm_my_pg_size; p++)
    {
	mpi_errno = mpig_bc_destroy(&bcs[p]);
	if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    }
    globus_libc_free(bcs);

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_free_business_cards);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_pm_free_business_cards() */


/*
 * mpig_pm_get_pg_size()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_get_pg_size
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_pm_get_pg_size(int * pg_size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_get_pg_size);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_get_pg_size);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_state == MPIG_PM_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_state == MPIG_PM_STATE_FINALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_finalized");
    
    MPIU_ERR_CHKANDJUMP((mpig_pm_state != MPIG_PM_STATE_GOT_PG_INFO), mpi_errno, MPI_ERR_INTERN, "**globus|pm_no_pg_size");
    *pg_size = mpig_pm_my_pg_size;
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_get_pg_size);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_pm_get_pg_size() */


/*
 * mpig_pm_get_pg_rank()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_get_pg_rank
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_pm_get_pg_rank(int * pg_rank)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_get_pg_rank);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_get_pg_rank);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_state == MPIG_PM_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_state == MPIG_PM_STATE_FINALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_finalized");
    
    MPIU_ERR_CHKANDJUMP((mpig_pm_state != MPIG_PM_STATE_GOT_PG_INFO), mpi_errno, MPI_ERR_INTERN, "**globus|pm_no_pg_rank");
    *pg_rank = mpig_pm_my_pg_rank;
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_get_pg_rank);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_pm_get_pg_rank() */


/*
 * mpig_pm_get_pg_id()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_get_pg_id
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_pm_get_pg_id(const char ** pg_id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_get_pg_id);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_get_pg_id);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_state == MPIG_PM_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_state == MPIG_PM_STATE_FINALIZED), mpi_errno, MPI_ERR_INTERN, "**globus|pm_finalized");
    
    MPIU_ERR_CHKANDJUMP((mpig_pm_state != MPIG_PM_STATE_GOT_PG_INFO), mpi_errno, MPI_ERR_INTERN, "**globus|pm_no_pg_id");
    *pg_id = mpig_pm_my_pg_id;
    
  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_get_pg_id);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_pm_get_pg_id() */


#if XXX
/*
 * mpig_pm_template()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_template
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_pm_template(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_template);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_template);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_template);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_pm_template() */
#endif

#if FOO
/*
 * get_topology(sj_size, sj_rank, pg_size, pg_rank, sj_num, sj_addrs)
 *
 * MUST be called by EVERY proc, each supplying sj_rank
 * sj_rank==0 -> subjobmaster, else subjobslave
 * and sj_size.
 *
 * fills the remaining args:
 *     pg_size - total number of procs
 *     pg_rank - my rank in [0, pg_size-1]
 *     sj_addrs - malloc'd and filled for OTHER subjobmasters only
 *                        inter_subjob_addr's of other subjobmasters
 *                        my subjob_addr NOT included (so njobs-1)
 *     sj_num - populated for subjobmasters only.
 *                total number of procs
 */

/* XXX: should return an mpi_errno ... not 'void' */

#undef FUNCNAME
#define FUNCNAME mpig_pm_i_get_topology
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int get_topology(int sj_size, int sj_rank, int * pg_size, int * pg_rank, int * sj_num, int ** sj_addrs)
{
    char topology_buf[GRAM_MYJOB_MAX_BUFFER_LENGTH];
    char * buf;
    int buf_len;
    int i;
    static unsigned int call_idx = 0;
    int sj0_master_idx;    /* used by subjobmasters only */
    int * job_sizes;       /* used by subjobmaster 0 only */
    int * g_ranks;         /* used by subjobmaster 0 only */
#if XXX
    int rc;
#endif    
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_i_get_topology);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_i_get_topology);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    call_idx ++;

    if (sj_rank != 0)
    {
	/* subjob slave */
#       if defined(MPIG_VMPI)
        {
	    int rc;
	    int v[2];

	    rc = mpig_vmpi_bcast((void *) v, 2, MPIG_VMPIT_INT, 0, &mpig_process.vmpi_cw);
	    if (rc != MPI_SUCCESS)
	    {
		globus_libc_fprintf(stderr, "ERROR: get_topology(): erroneous rc = %d from mp_bootstrap_bcast (non-root)\n", rc);
		exit(1);
	    } /* endif */

	    /* 
	     * NOTE: setting pg_rank as i do below works BECAUSE sj_rank == rank in VMPI_COMM_WORLD.  we know this because DUROC
	     * uses VMPI for intra-subjob messaging, and a side-effect of that is it sets sj_rank to the rank in the
	     * VMPI_COMM_WORLD it creates.
	     *
	     * We also know that the rank that DUROC assigns us is the one we will be using because VMPI_init is called only once
	     * ... we test if mp_initialized and don't call vMPI_Init again.
	     * 
	     */
	    *pg_size = v[0];
	    *pg_rank = v[1] + sj_rank; /* v[1] == subjobmstr grank */
        }
#       else
        {
	    char *rbuf;
	    char tag[SUBJOB_MAX_TAG_SIZE];

	    sprintf(tag, "%s:%d", SUBJOB_MASTER_TO_SLAVE_T, call_idx);

	    intra_subjob_receive(tag,     /* tag */
				&buf_len, /* nbytes received? */
				&rbuf);  /* message */

	    sscanf(rbuf, "%d %d", pg_size, pg_rank);
	    MPIU_Free(rbuf);
        }
#       endif
    }
    else
    {
	/* subjob master */
	int duroc_subjobmaster_rank;
	int my_subjob_addr;
	int rsl_subjob_rank;
	char *rsl_subjob_rank_env_var;

	globus_duroc_runtime_inter_subjob_structure(&my_subjob_addr, sj_num, sj_addrs);
	
	/* finding index of master subjob 0, he is the one with the lowest address   */
	for (i = 0, sj0_master_idx = -1, duroc_subjobmaster_rank = 0; i < *sj_num; i ++)
	{
	    if ((sj0_master_idx == -1 && (*sj_addrs)[i] < my_subjob_addr) ||
		(sj0_master_idx != -1 && (*sj_addrs)[i] < (*sj_addrs)[sj0_master_idx]))
	    { 
		sj0_master_idx = i;
	    }
	    if ((*sj_addrs)[i] < my_subjob_addr)
	    { 
		duroc_subjobmaster_rank ++;
	    }
	} /* endfor */
	
	/* globus_duroc_runtime_inter_subjob_structure reports the */
	/* number of REMOTE subjobs (*other* than the subjob i'm   */
	/* master of).  to get the TOTAL number of subjobs in this */
	/* run i must increment the value reported by              */
	/* globus_duroc_runtime_inter_subjob_structure             */
	(*sj_num) ++;

	/* XXX: should not exit here ... should set globus-like rc. */
	if (!(rsl_subjob_rank_env_var=getenv("GLOBUS_DUROC_SUBJOB_INDEX")))
	{
	    globus_libc_fprintf(stderr, "ERROR: required environment variable GLOBUS_DUROC_SUBJOB_INDEX not set.\n");
	    globus_libc_fprintf(stderr, "       Each subjob in envoking RSL must have GLOBUS_DUROC_SUBJOB_INDEX\n");
	    globus_libc_fprintf(stderr, "       set to rank (0, 1, 2, ...) of subjob as it appears in the envoking RSL.\n");
	    exit(1);
	} /* endif */
	rsl_subjob_rank = atoi(rsl_subjob_rank_env_var);
	if (rsl_subjob_rank < 0 || rsl_subjob_rank >= *sj_num)
	{
	    globus_libc_fprintf(stderr, "ERROR: env variable GLOBUS_DUROC_SUBJOB_INDEX %d must be >= 0 and\n", rsl_subjob_rank);
	    globus_libc_fprintf(stderr, 
		"ERROR: less than the number of subjobs %d for this run.\n", 
		*sj_num);
	    exit(1);
	} /* endif */

	if (duroc_subjobmaster_rank)
	{
	    /* NOT master of subjob 0 */

	    sprintf(topology_buf, "%d %d %d", duroc_subjobmaster_rank, rsl_subjob_rank, sj_size);

	    {
		char tag[SUBJOB_MAX_TAG_SIZE];

		sprintf(tag, "%s%d", SUBJOB_MASTER_TO_SUBJOB0_MASTER_T, call_idx);
		globus_duroc_runtime_inter_subjob_send((*sj_addrs)[sj0_master_idx], tag, strlen(topology_buf)+1,
						       (globus_byte_t *) topology_buf);

		sprintf(tag, "%s%d", SUBJOB0_MASTER_TO_SUBJOB_MASTER_T, call_idx);
		globus_duroc_runtime_inter_subjob_receive(tag, &buf_len, (globus_byte_t **) &buf);
	    }

	    sscanf(buf, "%d %d", pg_size, pg_rank);

	    MPIU_Free(buf);
	}
	else
	{
	    /* master of subjob 0 */

	    int j;
	    int temp;
	    /* vectors len sj_num, all indexed by duroc_subjobmaster_rank */
	    int *rsl_ranks; /* received from other subjob masters */

	    /* NICK: exiting on these failed mallocs is not right thing */
	    /*       to do.  should set error rc and return. fix that   */
	    /*       later when i learn more about globus rc stuff.     */
	    if (!(rsl_ranks = 
		(int *) MPIU_Malloc(*sj_num*sizeof(int))))
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: failed malloc of %d bytes\n", 
		    *sj_num*sizeof(int));
		exit(1);
	    } /* endif */
	    if (!(job_sizes = (int *) MPIU_Malloc(*sj_num*sizeof(int))))
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: failed malloc of %d bytes\n", 
		    *sj_num*sizeof(int));
		exit(1);
	    } /* endif */
	    if (!(g_ranks = (int *) MPIU_Malloc(*sj_num*sizeof(int))))
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: failed malloc of %d bytes\n", 
		    *sj_num*sizeof(int));
		exit(1);
	    } /* endif */

	    /* need to sort sj_addrs so that i may associate */
	    /* (using incoming duroc_subjobmaster_rank) incoming     */
	    /* rsl_subjob_rank and sj_size with dest addr     */
	    /* in sj_addrs                                   */
	    for (i = 1; i < *sj_num-1; i ++)
	    {
		for (j = i; j > 0; j --)
		{
		    if ((*sj_addrs)[j] < (*sj_addrs)[j-1])
		    {
			temp = (*sj_addrs)[j];
			(*sj_addrs)[j] = (*sj_addrs)[j-1];
			(*sj_addrs)[j-1] = temp;
		    } /* endif */
		} /* endfor */
	    } /* endfor */

	    /* rsl_ranks[] and job_sizes[] are indexed by  */
	    /* duroc_subjobmaster_rank, and i know that my */
	    /* duroc_subjobmaster_rank==0                  */
	    rsl_ranks[0] = rsl_subjob_rank;
	    job_sizes[0] = sj_size;

	    {
		char tag[SUBJOB_MAX_TAG_SIZE];

		sprintf(tag, "%s%d", 
		    SUBJOB_MASTER_TO_SUBJOB0_MASTER_T, call_idx);

		for (i = 1; i < *sj_num; i ++)
		{
		    int ranks, sizes;
		    
		    /*
		     * receiving 3 longs from other subjob master
		     *    duroc_subjobmaster_rank (used to index job_sizes[] 
		     *                             and rsl_ranks[])
		     *    rsl_subjob_rank 
		     *    sj_size
		     */

		    globus_duroc_runtime_inter_subjob_receive(tag, &buf_len, (globus_byte_t **) &buf);

		    sscanf(buf, "%d %d %d", &j, &ranks, &sizes);
		    rsl_ranks[j] = ranks;
		    job_sizes[j] = sizes;
		    
		    MPIU_Free(buf);
		} /* endfor */
	    }

	    /* calculating pg_size and everyones' g_rank based */
	    /* on rsl_rank and job_sizes ...                  */
	    /* mygrank = sum job_size for all rsl_ranks       */
	    /*           that are less than mine              */
	    for (i = 0, *pg_size = 0; i < *sj_num; i ++)
	    {
		(*pg_size) += job_sizes[i];
		for (g_ranks[i] = 0, j = 0; j < *sj_num; j ++)
		if (rsl_ranks[i] > rsl_ranks[j])
		    g_ranks[i] += job_sizes[j];
	    } /* endfor */
	    *pg_rank = g_ranks[0];

	    {
		char tag[SUBJOB_MAX_TAG_SIZE];

		sprintf(tag, "%s%d", 
		    SUBJOB0_MASTER_TO_SUBJOB_MASTER_T, call_idx);

		/* sending other subjob masters pg_size and their g_rank */
		for (i = 0; i < *sj_num-1; i ++)
		{
		    sprintf(topology_buf, "%d %d", *pg_size, g_ranks[i+1]);
		    globus_duroc_runtime_inter_subjob_send(
			(*sj_addrs)[i],           /* dest */
			tag,                              /* tag */
			strlen(topology_buf)+1,          /* nbytes */
			(globus_byte_t *) topology_buf); /* data */
		} /* endfor */
	    }

	    MPIU_Free(rsl_ranks);
	    MPIU_Free(job_sizes);
	    MPIU_Free(g_ranks);

	} /* endif */

	/* all subjob masters sending pg_size and their g_rank to their slaves */
#       if defined(MPIG_VMPI)
	{
	    int v[2];
	    int rc;

	    v[0] = *pg_size;
	    v[1] = *pg_rank;

	    /* MPI_Bcast(v, 2, MPI_INT, 0, MPI_COMM_WORLD); */
	    rc = vmpi_error_to_mpich_error(
		    mp_bootstrap_bcast((void *) v, /* buf */
					2,          /* count */
					0));        /* type, 0 == vMPI_INT */
	    if (rc != MPI_SUCCESS)
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: get_topology(): erroneous rc = %d from "
		    "mp_bootstrap_bcast (root)\n",
		    rc);
		exit(1);
	    } /* endif */
	}
#       else
        {
	    {
		char tag[SUBJOB_MAX_TAG_SIZE];

		sprintf(tag, "%s%d", SUBJOB_MASTER_TO_SLAVE_T, call_idx);

		for (i = 1; i < sj_size; i ++)
		{
		    sprintf(topology_buf, "%d %d", *pg_size, (*pg_rank) + i);
		    intra_subjob_send(i,                     /* dest */
				    tag,                     /* tag */
				    strlen(topology_buf)+1, /* nbytes */
				    topology_buf);          /* data */
		} /* endfor */
	    }
        }
#       endif
    } /* endif */

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_i_get_topology);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* end get_topology() */


/*
 * distribute_byte_array()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_i_distribute_byte_array
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPIG_STATIC int distribute_byte_array(int pg_size, int pg_rank, int sj_size, int sj_rank, int sj_num, int * sj_addrs,
				      globus_byte_t * inbuf, int inbuf_len, globus_byte_t ** outbufs, int * outbufs_lens)
{

    int i;
    char *buf;
    int buf_len;
    static unsigned int call_idx = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_i_distribute_byte_array);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_i_distribute_byte_array);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
/* globus_libc_fprintf(stderr, "%d: enter distribute_byte_array: sj_rank %d %d bytes\n", MPID_MyWorldRank, sj_rank, inbuf_len); */
    call_idx ++;

    /* initialization */
    for (i = 0; i < pg_size; i ++)
    {
	outbufs[i]    = (globus_byte_t *) NULL;
	outbufs_lens[i] = 0;
    } /* endfor */

    if (sj_rank)
    {
/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: i am subjob slave\n", MPID_MyWorldRank); */
	/* subjob slave */
#       if defined(MPIG_VMPI)
        {
	    int rc;

	    /* MPI_Gather(&inbuf_len,  */
			/* 1, */
			/* MPI_INT, */
			/* (void *) NULL,  */
			/* 1, */
			/* MPI_INT, */
			/* 0, */
			/* MPI_COMM_WORLD); */
	    rc = vmpi_error_to_mpich_error(
		    mp_bootstrap_gather((void *) &inbuf_len,   /* sendbuf */
				1,                   /* sendcount */
				(void *) NULL,       /* rcvbuf, significant 
					                at root only */
				1));                  /* rcvcount, significant
					                at root only */
	    if (rc != MPI_SUCCESS)
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: distribute_byte_array(): erroneous rc = %d from "
		    "mp_bootstrap_gather (non-root)\n",
		    rc);
		exit(1);
	    } /* endif */

	    /* MPI_Gatherv((char *) inbuf,  */
			/* inbuf_len,  */
			/* MPI_CHAR,  */
			/* (void *) NULL,  */
			/* (int *) NULL,  */
			/* (int *) NULL,  */
			/* MPI_CHAR, */
			/* 0, */
			/* MPI_COMM_WORLD); */
	    rc = vmpi_error_to_mpich_error(
		    mp_bootstrap_gatherv((void *) inbuf,  /* sendbuf */
				inbuf_len,           /* sendcount */
				(void *) NULL,       /* rcvbuf, significant 
					                at root only */
				(int *) NULL,        /* rcvcounts, significant
					                at root only */
				(int *) NULL));      /* displs, significant
					                at root only */
	    if (rc != MPI_SUCCESS)
	    {
		globus_libc_fprintf(stderr, "ERROR: distribute_byte_array(): erroneous rc = %d from "
		    "mp_bootstrap_gatherv (non-root)\n", rc);
		exit(1);
	    } /* endif */

        }
#       else
        {
	    char *bigbuf;
	    char *t;
	    char tagged_intrabuf[GRAM_MYJOB_MAX_BUFFER_LENGTH];

	    if (sizeof(tagged_intrabuf) < (2*INT_MAX_STRLEN)+inbuf_len)
	    {
		if (!(bigbuf = (char *) MPIU_Malloc((2*INT_MAX_STRLEN)+inbuf_len)))
		{
		    globus_libc_fprintf(stderr, "ERROR: failed malloc of %d bytes\n", (2*INT_MAX_STRLEN)+inbuf_len);
		    exit(1);
		} /* endif */
		t = bigbuf;
	    }
	    else
	    {
		t = tagged_intrabuf;
		bigbuf = (char *) NULL;
	    } /* endif */

	    /* tagging and copying my byte array for distribution */
	    sprintf(t,              "%d ",  pg_rank);
	    sprintf(t+INT_MAX_STRLEN,    "%d ",  inbuf_len);
	    memcpy(t+(2*INT_MAX_STRLEN), inbuf, inbuf_len);

	    {
		char tag[SUBJOB_MAX_TAG_SIZE];
		sprintf(tag, "%s%d", SUBJOB_SLAVE_TO_MASTER_D, call_idx);
		/* send my byte array to my master */
		intra_subjob_gather(sj_size, sj_rank, t, (2*INT_MAX_STRLEN)+inbuf_len, tag, (int *) NULL, (char **) NULL);
	    }

	    if (bigbuf)
		MPIU_Free(bigbuf);
	}
#       endif
	
	/* receiving all other byte arrays from my master */
	i = 0; 
	while (i < pg_size)
	{
	    char *rbuf;
	    int nbufs;
/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: subjob slave: top of while loop i %d pg_size %d\n", MPID_MyWorldRank,
   i, pg_size); */

#           if defined(MPIG_VMPI)
            {
		int bsize;
		int rc;

		/* MPI_Bcast(&bsize, 1, MPI_INT, 0, MPI_COMM_WORLD); */
		rc = vmpi_error_to_mpich_error(
			/* NICK */
			mp_bootstrap_bcast((void *) &bsize, /* buf */
					    1,   /* count */
					    0)); /* type, 0 == vMPI_INT */
		if (rc != MPI_SUCCESS)
		{
		    globus_libc_fprintf(stderr, 
			"ERROR: distribute_byte_array(): erroneous rc = %d "
			"from mp_bootstrap_bcast (non-root, int)\n",
			rc);
		    exit(1);
		} /* endif */

		if (!(rbuf = (char *) MPIU_Malloc(bsize)))
		{
		    globus_libc_fprintf(stderr, 
			"ERROR: distribute_byte_array(): "
			"failed malloc of %d bytes\n", 
			bsize);
		    exit(1);
		} /* endif */

		/* MPI_Bcast(rbuf, bsize, MPI_CHAR, 0, MPI_COMM_WORLD); */
		rc = vmpi_error_to_mpich_error(
			mp_bootstrap_bcast((void *) rbuf, /* buf */
					    bsize, /* count */
					    1));   /* type, 1 == vMPI_CHAR */
		if (rc != MPI_SUCCESS)
		{
		    globus_libc_fprintf(stderr, 
			"ERROR: distribute_byte_array(): erroneous rc = %d "
			"from mp_bootstrap_bcast (non-root, char)\n",
			rc);
		    exit(1);
		} /* endif */

	    }
#           else
            {
		char tag[SUBJOB_MAX_TAG_SIZE];
		sprintf(tag, "%s%d", SUBJOB_MASTER_TO_SLAVE_D, call_idx);
		/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: subjob slave: before intra_subjob_bcast\n",
		                       MPID_MyWorldRank); */
		intra_subjob_bcast(sj_size, sj_rank, tag, &buf_len, &rbuf);
		/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: subjob slave: after intra_subjob_bcast\n",
		                       MPID_MyWorldRank); */
	    }
#           endif

	    extract_byte_arrays(rbuf, &nbufs, outbufs, outbufs_lens);
	    MPIU_Free(rbuf);
	    i += nbufs;

	} /* endwhile */
    }
    else 
    {
	/* subjob master */
	char *my_subjob_buf;
	int my_subjob_bufsize;
/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: i am subjob master\n", MPID_MyWorldRank); */

#       if defined(MPIG_VMPI)
        {
	    char *temp_buf;
	    char *dest;
	    int *rcounts;
	    int *displs;
	    int rc;

	    if (!(rcounts = 
		    (int *) MPIU_Malloc(sj_size*sizeof(int))))
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: failed malloc of %d bytes\n", 
		    (int) (sj_size*sizeof(int)));
		exit(1);
	    } /* endif */
	    if (!(displs = 
		    (int *) MPIU_Malloc(sj_size*sizeof(int))))
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: failed malloc of %d bytes\n", 
		    (int) (sj_size*sizeof(int)));
		exit(1);
	    } /* endif */

	    /* MPI_Gather(&inbuf_len,  */
			/* 1,  */
			/* MPI_INT,  */
			/* rcounts,  */
			/* 1, */
			/* MPI_INT, */
			/* 0,  */
			/* MPI_COMM_WORLD); */
	    rc = vmpi_error_to_mpich_error(
		    mp_bootstrap_gather((void *) &inbuf_len,   /* sendbuf */
				1,                   /* sendcount */
				(void *) rcounts,    /* rcvbuf  */
				1));                 /* rcvcount */

	    if (rc != MPI_SUCCESS)
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: distribute_byte_array(): erroneous rc = %d from "
		    "mp_bootstrap_gather (root)\n",
		    rc);
		exit(1);
	    } /* endif */

	    my_subjob_bufsize = displs[0] = 0;
	    for (i = 0; i < sj_size; i ++)
	    {
		my_subjob_bufsize += rcounts[i];
		if (i)
		    displs[i] = displs[i-1] + rcounts[i-1];
	    } /* endfor */
	    if (!(temp_buf = (char *) MPIU_Malloc(my_subjob_bufsize)))
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: failed malloc of %d bytes\n", my_subjob_bufsize);
		exit(1);
	    } /* endif */

	    /* MPI_Gatherv((char *) inbuf,  */
			/* inbuf_len,  */
			/* MPI_CHAR,  */
			/* temp_buf,  */
			/* rcounts,  */
			/* displs,  */
			/* MPI_CHAR,  */
			/* 0,  */
			/* MPI_COMM_WORLD); */
	    rc = vmpi_error_to_mpich_error(
		    mp_bootstrap_gatherv((void *) inbuf,   /* sendbuf */
				inbuf_len,            /* sendcount */
				(void *) temp_buf,   /* rcvbuf */
				rcounts,              /* rcvcounts */
				displs));             /* displacements */

	    if (rc != MPI_SUCCESS)
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: distribute_byte_array(): erroneous rc = %d from "
		    "mp_bootstrap_gatherv (root)\n",
		    rc);
		exit(1);
	    } /* endif */

	    my_subjob_bufsize += (INT_MAX_STRLEN+(sj_size*2*INT_MAX_STRLEN));
	    if (!(my_subjob_buf = 
		    (char *) MPIU_Malloc(my_subjob_bufsize)))
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: failed malloc of %d bytes\n", my_subjob_bufsize);
		exit(1);
	    } /* endif */
	    sprintf(my_subjob_buf,               "%d ",  sj_size);
	    sprintf(my_subjob_buf+INT_MAX_STRLEN,     "%d ",  pg_rank);
	    sprintf(my_subjob_buf+(2*INT_MAX_STRLEN), "%d ",  inbuf_len);

	    memcpy(my_subjob_buf+(3*INT_MAX_STRLEN),  inbuf, inbuf_len);

	    dest = my_subjob_buf + (3*INT_MAX_STRLEN) + inbuf_len;

	    /* filling the rest of my_subjob_buf */
	    for (i = 1; i < sj_size; i ++)
	    {
		sprintf(dest, "%d ", pg_rank+i);
		sprintf(dest+INT_MAX_STRLEN, "%d ", rcounts[i]);
		memcpy(dest+(2*INT_MAX_STRLEN), temp_buf+displs[i], rcounts[i]);
		dest += ((2*INT_MAX_STRLEN)+rcounts[i]);
	    } /* endfor */

	    MPIU_Free(temp_buf);
	    MPIU_Free(rcounts);
	    MPIU_Free(displs);
	}
#       else
	{
	    /* 
	     * constructing inter-subjob message MY subjob 
	     * to pass around ring of subjob masters 
	     */
	    
	    char *t;
	    int t_len = (3*INT_MAX_STRLEN)+inbuf_len;

	    if (!(t = (char *) MPIU_Malloc(t_len)))
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: failed malloc of %d bytes\n", t_len);
		exit(1);
	    } /* endif */

	    sprintf(t,               "%d ",  sj_size);
	    sprintf(t+INT_MAX_STRLEN,     "%d ",  pg_rank);
	    sprintf(t+(2*INT_MAX_STRLEN), "%d ",  inbuf_len);
	    memcpy(t+(3*INT_MAX_STRLEN),  inbuf, inbuf_len);

	    {
		char tag[SUBJOB_MAX_TAG_SIZE];
		sprintf(tag, "%s%d", SUBJOB_SLAVE_TO_MASTER_D, call_idx);

		intra_subjob_gather(sj_size, sj_rank, t, t_len, tag, &my_subjob_bufsize, &my_subjob_buf);
	    }
	    MPIU_Free(t);
	}
#       endif

	extract_byte_arrays(my_subjob_buf, (int *) NULL, outbufs,outbufs_lens);

#       if defined(MPIG_VMPI)
        {
	    int rc;

	    /* MPI_Bcast(&my_subjob_bufsize, 1, MPI_INT, 0, MPI_COMM_WORLD); */
	    rc = vmpi_error_to_mpich_error(
		    mp_bootstrap_bcast((void *) &my_subjob_bufsize, /* buf */
					1,   /* count */
					0)); /* type, 0 == vMPI_INT */
	    if (rc != MPI_SUCCESS)
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: distribute_byte_array(): erroneous rc = %d "
		    "from mp_bootstrap_bcast (root, int)\n",
		    rc);
		exit(1);
	    } /* endif */

	    /* MPI_Bcast(my_subjob_buf,  */
			/* my_subjob_bufsize,  */
			/* MPI_CHAR,  */
			/* 0,  */
			/* MPI_COMM_WORLD); */
	    rc = vmpi_error_to_mpich_error(
		    mp_bootstrap_bcast((void *) my_subjob_buf, /* buf */
					my_subjob_bufsize,     /* count */
					1)); /* type, 0 == vMPI_CHAR */
	    if (rc != MPI_SUCCESS)
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: distribute_byte_array(): erroneous rc = %d "
		    "from mp_bootstrap_bcast (root, char)\n",
		    rc);
		exit(1);
	    } /* endif */

	}
#       else
        {
	    char tag[SUBJOB_MAX_TAG_SIZE];

	    /* sending inter-subjob message for MY subjob to all my slaves */
	    sprintf(tag, "%s%d", SUBJOB_MASTER_TO_SLAVE_D, call_idx);
	    /* globus_libc_fprintf(stderr, "%d: distribute_byte_array: subjob master: before intra_subjob_bcast our subjob\n",
	                           MPID_MyWorldRank); */
	    intra_subjob_bcast(sj_size, sj_rank, tag, &my_subjob_bufsize, &my_subjob_buf);
	    /* globus_libc_fprintf(stderr, "%d: distribute_byte_array: subjob master: after intra_subjob_bcast our subjob\n",
	                           MPID_MyWorldRank); */
	}
#       endif

	{
	    char tag[SUBJOB_MAX_TAG_SIZE];
	    sprintf(tag, "%s%d", SUBJOB_MASTER_TO_SUBJOB_MASTER_D, call_idx);

	    /* 
	     * sending inter-subjob message for MY subjob 
	     * to other subjob masters 
	     */
	    for (i = 0; i < sj_num-1; i ++)
	    {
		globus_duroc_runtime_inter_subjob_send( 
		    sj_addrs[i],                 /* dest */ 
		    tag,                                 /* tag */
		    my_subjob_bufsize,                  /* nbytes */
		    (globus_byte_t *) my_subjob_buf);   /* data */
	    } /* endfor */
	}

	MPIU_Free(my_subjob_buf);

	/* receiving subjob byte arrays from other subjob masters */
	for (i = 0; i < sj_num-1; i ++)
	{
/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: subjob master: top of for loop i %d sj_num-1 %d\n",
   MPID_MyWorldRank, i, sj_num-1); */
	    {
		char tag[SUBJOB_MAX_TAG_SIZE];
		sprintf(tag, "%s%d", SUBJOB_MASTER_TO_SUBJOB_MASTER_D, call_idx);

		globus_duroc_runtime_inter_subjob_receive(
		    tag,                        /* tag */
		    &buf_len,                   /* nbytes received? */
		    (globus_byte_t **) &buf);  /* message */
	    }

#           if defined(MPIG_VMPI)
	    {
		int rc;

		rc = mpig_vmpi_bcast(&buf_len, 1, MPIG_VMPI_INT, 0, MPIG_VMPI_COMM_WORLD); */
		rc = vmpi_error_to_mpich_error(
			mp_bootstrap_bcast((void *) &buf_len, /* buf */
					    1,                /* count */
					    0)); /* type, 0 == vMPI_INT */
		if (rc != MPI_SUCCESS)
		{
		    globus_libc_fprintf(stderr, 
			"ERROR: distribute_byte_array(): erroneous rc = %d "
			"from mp_bootstrap_bcast (root, int, 2)\n",
			rc);
		    exit(1);
		} /* endif */

		/* MPI_Bcast(buf, buf_len, MPI_CHAR, 0, MPI_COMM_WORLD); */
		rc = vmpi_error_to_mpich_error(
			mp_bootstrap_bcast((void *) buf, /* buf */
					    buf_len,      /* count */
					    1)); /* type, 1 == vMPI_CHAR */
		if (rc != MPI_SUCCESS)
		{
		    globus_libc_fprintf(stderr, 
			"ERROR: distribute_byte_array(): erroneous rc = %d "
			"from mp_bootstrap_bcast (root, char, 2)\n",
			rc);
		    exit(1);
		} /* endif */

	    }
#           else
	    {
		/* bcasting inter-subjob message to all my slaves */
		char tag[SUBJOB_MAX_TAG_SIZE];
		sprintf(tag, "%s%d", SUBJOB_MASTER_TO_SLAVE_D, call_idx);
		/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: subjob master: before intra_subjob_bcast"
		   " other subjob\n", MPID_MyWorldRank); */
		intra_subjob_bcast(sj_size, sj_rank, tag, &buf_len, &buf);
		/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: subjob master: after intra_subjob_bcast"
		   " other subjob\n", MPID_MyWorldRank); */
	    }
#           endif

	    extract_byte_arrays(buf, (int *) NULL, outbufs, outbufs_lens);
	    MPIU_Free(buf);
	} /* endfor */
    } /* endif */

/* globus_libc_fprintf(stderr, "%d: exit distribute_byte_array: sj_rank %d %d bytes\n", MPID_MyWorldRank, sj_rank, inbuf_len); */

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_i_distribute_byte_array);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* end distribute_byte_array() */


#if !defined(MPIG_VMPI)
MPIG_STATIC void intra_subjob_send(int dest, char *tag_base, int nbytes, char *buf)
{
    char tag[SUBJOB_MAX_TAG_SIZE];
    char *bigtag;
    char *t;
    int i;
    /* NICK: This is a hack because globus_duroc_runtime_intra_subjob_send
     *       dictates that the tag+message must fit into a buffer the
     *       size of GRAM_MYJOB_MAX_BUFFER_LENGTH-10 and they ain't 
     *       likely gonna fix this Globus code ever ... they've moved on 
     *       to Web-services and have abandonded all this DUROC code for good.
     */
    /* char send_buf[GRAM_MYJOB_MAX_BUFFER_LENGTH]; */
    char send_buf[GRAM_MYJOB_MAX_BUFFER_LENGTH-15];
    int max_payload_size = GRAM_MYJOB_MAX_BUFFER_LENGTH - 10 
			    - strlen(tag_base) - 5;
    char *src;
    int bytes_sent;
    int ncpy;


    if (strlen(tag_base)+5 > sizeof(tag))
    {
	if (!(bigtag = (char *) MPIU_Malloc(strlen(tag_base)+5)))
	{
	    globus_libc_fprintf(stderr,
		"ERROR: failed malloc of %d bytes\n", 
		((int) strlen(tag_base))+5);
	    exit(1);
	} /* endif */
	t = bigtag;
    }
    else
    {
	bigtag = (char *) NULL;
	t = tag;
    } /* endif */

    /* sending as much as i can in the first buffer */
    sprintf(send_buf, "%d ", nbytes);
    ncpy = max_payload_size-INT_MAX_STRLEN < nbytes 
	    ? max_payload_size-INT_MAX_STRLEN 
	    : nbytes;

    memcpy(send_buf+INT_MAX_STRLEN, buf, ncpy);

    sprintf(t, "%s0", tag_base);
    globus_duroc_runtime_intra_subjob_send(dest, t, INT_MAX_STRLEN + ncpy, (globus_byte_t *) send_buf);

    /* pushing out remaining data */
    for (i = 1, bytes_sent = ncpy, src = buf+ncpy; bytes_sent < nbytes; i ++)
    {
	ncpy = max_payload_size < nbytes-bytes_sent ? max_payload_size : nbytes-bytes_sent;
	memcpy(send_buf, src, ncpy);
	sprintf(t, "%s%d", tag_base, i);
	globus_duroc_runtime_intra_subjob_send(dest, t, ncpy, (globus_byte_t *) send_buf);

	bytes_sent += ncpy;
	src        += ncpy;
    } /* endfor */

    /* clean-up */
    if (bigtag)
    { 
	MPIU_Free(bigtag);
    }

} /* end intra_subjob_send() */

MPIG_STATIC void intra_subjob_receive(char *tag_base, int *rcvd_nbytes, char **buf)
{
    char tag[SUBJOB_MAX_TAG_SIZE];
    char *bigtag;
    char *t;
    int i;
    char rcv_buf[GRAM_MYJOB_MAX_BUFFER_LENGTH];
    int nr;
    char *dest;
    int bytes_rcvd;

    if (strlen(tag_base) > sizeof(tag)+5)
    {
	if (!(bigtag = (char *) MPIU_Malloc(strlen(tag_base)+5)))
	{
	    globus_libc_fprintf(stderr,
		"ERROR: failed malloc of %d bytes\n", 
		((int) strlen(tag_base))+5);
	    exit(1);
	} /* endif */
	t = bigtag;
    }
    else
    {
	bigtag = (char *) NULL;
	t = tag;
    } /* endif */

    /* rcv as much as i can in the first buffer */
    sprintf(t, "%s0", tag_base);
    globus_duroc_runtime_intra_subjob_receive(
	t,                           /* tag */
	&nr,                         /* nbytes received? */
	(globus_byte_t *) rcv_buf); /* message */
    sscanf(rcv_buf, "%d ", rcvd_nbytes);
    if (!(*buf = (char *) MPIU_Malloc(*rcvd_nbytes)))
    {
	globus_libc_fprintf(stderr,
	    "ERROR: failed malloc of %d bytes\n", *rcvd_nbytes);
	exit(1);
    } /* endif */

    memcpy(*buf, rcv_buf+INT_MAX_STRLEN, nr-INT_MAX_STRLEN);

    /* receiving remaining data */
    for (i = 1, bytes_rcvd = nr-INT_MAX_STRLEN, dest = *buf+(nr-INT_MAX_STRLEN); 
	bytes_rcvd < *rcvd_nbytes; i ++)
    {
	sprintf(t, "%s%d", tag_base, i);
	globus_duroc_runtime_intra_subjob_receive(
	    t,                           /* tag */
	    &nr,                         /* nbytes received? */
	    (globus_byte_t *) rcv_buf); /* message */
	memcpy(dest, rcv_buf, nr);

	bytes_rcvd += nr;
	dest       += nr;

    } /* endfor */

    /* clean-up */
    if (bigtag)
	MPIU_Free(bigtag);

} /* end intra_subjob_receive() */
#endif /* !defined(MPIG_VMPI) */

MPIG_STATIC void extract_byte_arrays(char *rbuf, 
				     int *nbufs_p,  /* optional */
				     globus_byte_t **outbufs, 
				     int *outbuflens)
{
    char *src;
    int nbufs;
    int j;

    src = rbuf + INT_MAX_STRLEN;
    sscanf(rbuf, "%d ", &nbufs);
    if (nbufs_p)
	*nbufs_p = nbufs;

    for (j = 0; j < nbufs; j ++)
    {
	int id;

	sscanf(src, "%d ", &id);
	/* globus_libc_fprintf(stderr, "%d: extract_byte_arrays: FOO extracting rank %d\n", MPID_MyWorldRank, id); */

	if (outbufs[id])
	{
	    globus_libc_fprintf(stderr, "ERROR(%d): just rcvd second byte array from %d\n", mpig_process.my_pg_rank, id);
	    exit(1);
	} /* endif */

	sscanf(src+INT_MAX_STRLEN, "%d ", outbuflens+id);
	if (!(outbufs[id] = 
	    (globus_byte_t *) MPIU_Malloc(outbuflens[id])))
	{
	    globus_libc_fprintf(stderr, 
		"ERROR: failed malloc of %d bytes\n", outbuflens[id]);
	    exit(1);
	} /* endif */

	memcpy(outbufs[id], src+(2*INT_MAX_STRLEN), outbuflens[id]);

	src += ((2*INT_MAX_STRLEN)+outbuflens[id]);
    } /* endfor */

} /* end extract_byte_arrays() */


#if !defined(MPIG_VMPI)
/*
 *
 * NOTE: both bcast/gather assumes root is always rank_in_my_subjob=0
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

MPIG_STATIC void intra_subjob_bcast(int sj_size, int sj_rank, char * tag_base, int * rcvd_nbytes, char ** buf)
{
    int mask;

    /*
     *   Do subdivision.  There are two phases:
     *   1. Wait for arrival of data.  Because of the power of two nature
     *      of the subtree roots, the source of this message is alwyas the
     *      process whose relative rank has the least significant bit CLEARED.
     *      That is, process 4 (100) receives from process 0, process 7 (111) 
     *      from process 6 (110), etc.   
     *   2. Forward to my subtree
     *
     *   Note that the process that is the tree root is handled automatically
     *   by this code, since it has no bits set.
     */

    /* loop to find LSB */
    for (mask = 0x1;
	mask < sj_size && !(sj_rank & mask);
	    mask <<= 1)
    ;

    if (sj_rank & mask) 
    {
	/* i am not root */
	intra_subjob_receive(tag_base,    /* tag */
			    rcvd_nbytes,  /* nbytes rcvd? */
			    buf);        /* message */
    } /* endif */

    /*
     *   This process is responsible for all processes that have bits set from
     *   the LSB upto (but not including) mask.  Because of the "not including",
     *   we start by shifting mask back down one.
     */
    mask >>= 1;
    while (mask > 0) 
    {
	if (sj_rank + mask < sj_size) 
	{
	    intra_subjob_send(sj_rank+mask, /* dest */
			    tag_base,                 /* tag */
			    *rcvd_nbytes,             /* nbytes */
			    *buf);                   /* data */
	} /* endif */
	mask >>= 1;
    } /* endwhile */

} /* end intra_subjob_bcast() */

MPIG_STATIC void intra_subjob_gather(int sj_size, int sj_rank, char *inbuf, int inbuflen, char *tag_base,
				     int *rcvd_nbytes, char **buf)
{
    int mask;
    int my_subjob_bufsize;
    char *my_subjob_buf;
    char *dest;
    char *tag;

/* printf("sj_rank %d: enter intra_subjob_gather\n", sj_rank); fflush(stdout); */
    if (!(tag = (char *) MPIU_Malloc(strlen(tag_base)+10)))
    {
	globus_libc_fprintf(stderr, "ERROR: failed tag malloc of %d bytes\n", strlen(tag_base)+10);
	exit(1);
    } /* endif */

    /* 
     * take a guess of how big my_subjob_buf needs to be 
     * based on my inbuf size and the size of my subjob
     */

    my_subjob_bufsize = sj_size*inbuflen+100;
    if (!(my_subjob_buf = (char *) MPIU_Malloc(my_subjob_bufsize)))
    {
	globus_libc_fprintf(stderr, "ERROR: failed my_subjob_buf malloc of %d bytes\n", my_subjob_bufsize);
	exit(1);
    } /* endif */

    memcpy(my_subjob_buf,  inbuf, inbuflen);
    dest = my_subjob_buf + inbuflen;

    for (mask = 0x1; 
	mask < sj_size && !(sj_rank & mask); 
	    mask <<= 1)
    {
	if (sj_rank+mask < sj_size)
	{
	    int buflen;
	    char *rbuf;

	    sprintf(tag, "%s%d", tag_base, sj_rank+mask);
	    intra_subjob_receive(tag,      /* tag */
				&buflen,  /* nbytes received? */
				&rbuf);   /* message */

	    if (my_subjob_bufsize-(dest-my_subjob_buf) < buflen)
	    {
		/* have to re-alloc */
		int disp = dest-my_subjob_buf;

		my_subjob_bufsize += (sj_size*inbuflen+100);

		if (!(my_subjob_buf = 
			(char *) globus_libc_realloc(my_subjob_buf, 
						    my_subjob_bufsize)))
		{
		    globus_libc_fprintf(stderr, 
			"ERROR: failed realloc of %d bytes\n", 
			my_subjob_bufsize);
		    exit(1);
		} /* endif */

		dest = my_subjob_buf+disp;

	    } /* endif */

	    memcpy(dest, rbuf, buflen);
	    dest += buflen;

	    MPIU_Free(rbuf);
	} /* endif */
    } /* endfor */

    if (sj_rank)
    {
	/* subjob slave */
	sprintf(tag, "%s%d", tag_base, sj_rank);
	intra_subjob_send(sj_rank - mask, /* dest */
			tag,                        /* tag */
			dest-my_subjob_buf,        /* nbytes */
			my_subjob_buf);            /* data */
	MPIU_Free(my_subjob_buf);
    } 
    else
    {
	/* subjob master */
	*rcvd_nbytes = dest-my_subjob_buf;
	*buf        = my_subjob_buf;
    } /* endif */

    MPIU_Free(tag);

} /* end intra_subjob_gather() */

#endif /* !defined(MPIG_VMPI) */

#else /* !FOO */

/*
 * void get_topology(int rank_in_my_subjob, 
 *                   int my_subjob_size, 
 *                   int **subjob_addresses,
 *                   int *nprocs, 
 *                   int *nsubjobs, 
 *                   int *my_grank)
 *
 * MUST be called by EVERY proc, each supplying rank_in_my_subjob
 * rank_in_my_subjob==0 -> subjobmaster, else subjobslave
 * and my_subjob_size.
 *
 * fills the remaining args:
 *     subjob_addresses - malloc'd and filled for OTHER subjobmasters only
 *                        inter_subjob_addr's of other subjobmasters
 *                        my subjob_addr NOT included (so njobs-1)
 *     nsubjobs - populated for subjobmasters only.
 *                total number of procs
 *     nprocs - total number of procs
 *     my_grank - my rank in [0, nprocs-1]
 *     
 */

/* NICK: should return 'globus-like' rc ... not 'void' */

MPIG_STATIC void get_topology(int rank_in_my_subjob, 
			      int my_subjob_size, 
			      int **subjob_addresses,
			      int *nprocs, 
			      int *nsubjobs, 
			      int *my_grank)
{
    char topology_buff[GRAM_MYJOB_MAX_BUFFER_LENGTH];
    char *buff;
    int bufflen;
    int i;
    static unsigned int call_idx = 0;
    int sj0_master_idx;    /* used by subjobmasters only */
    int *job_sizes;        /* used by subjobmaster 0 only */
    int *g_ranks;          /* used by subjobmaster 0 only */

    call_idx ++;

    if (rank_in_my_subjob)
    {
	/* subjob slave */
#       if defined(MPIG_VMPI)
        {
	    int rc;
	    int v[2];

	    /* MPI_Bcast(v, 2, MPI_INT, 0, MPI_COMM_WORLD); */
	    rc = vmpi_error_to_mpich_error(
		    mp_bootstrap_bcast((void *) v, /* buff */
					2,         /* count */
			                0));       /* type, 0 == vMPI_INT */
	    if (rc != MPI_SUCCESS)
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: get_topology(): erroneous rc = %d from "
		    "mp_bootstrap_bcast (non-root)\n",
		    rc);
		exit(1);
	    } /* endif */

	    /* 
	     * note: setting my_grank as i do below works BECAUSE 
	     *       rank_in_my_subjob == rank in vMPI_COMM_WORLD.
	     *       we know this because DUROC uses vMPI for 
	     *       intra-subjob messaging, and a side-effect of
	     *       that is it sets rank_in_my_subjob to the 
	     *       rank in the vMPI_COMM_WORLD it creates.
	     *
	     *       we also know that the rank that DUROC assigns
	     *       us is the one we will be using because vMPI_Init
	     *       is called only once ... we test if mp_initialized
	     *       and don't call vMPI_Init again.
	     * 
	     */
	    
	    *nprocs   = v[0];
	    *my_grank = v[1] + rank_in_my_subjob; /* v[1] == subjobmstr grank */
        }
#       else
        {
	    char *rbuff;
	    char tag[200];

	    sprintf(tag, "%s%d", SUBJOB_MASTER_TO_SLAVE_T, call_idx);

	    intra_subjob_receive(tag,     /* tag */
				&bufflen, /* nbytes received? */
				&rbuff);  /* message */

	    sscanf(rbuff, "%d %d", nprocs, my_grank);
	    globus_libc_free(rbuff);
        }
#       endif
    }
    else
    {
	/* subjob master */
	int duroc_subjobmaster_rank;
	int my_subjob_addr;
	int rsl_subjob_rank;
	char *rsl_subjob_rank_env_var;

	globus_duroc_runtime_inter_subjob_structure(&my_subjob_addr,
						    nsubjobs,
						    subjob_addresses);
	/* finding index of master subjob 0, he */
	/* is the one with the lowest address   */
	for (i = 0, sj0_master_idx = -1, duroc_subjobmaster_rank = 0; 
	    i < *nsubjobs; i ++)
	{
	    if ((sj0_master_idx == -1 
		    && (*subjob_addresses)[i] < my_subjob_addr)
	    || (sj0_master_idx != -1 
		    && (*subjob_addresses)[i] 
				< (*subjob_addresses)[sj0_master_idx]))
		sj0_master_idx = i;
	    if ((*subjob_addresses)[i] < my_subjob_addr)
		duroc_subjobmaster_rank ++;
	} /* endfor */
	/* globus_duroc_runtime_inter_subjob_structure reports the */
	/* number of REMOTE subjobs (*other* than the subjob i'm   */
	/* master of).  to get the TOTAL number of subjobs in this */
	/* run i must increment the value reported by              */
	/* globus_duroc_runtime_inter_subjob_structure             */
	(*nsubjobs) ++;

	/* NICK: should not exit here ... should set globus-like rc. */
	if (!(rsl_subjob_rank_env_var=getenv("GLOBUS_DUROC_SUBJOB_INDEX")))
	{
	    globus_libc_fprintf(stderr, 
		"ERROR: required environment variable "
		"GLOBUS_DUROC_SUBJOB_INDEX not set.\n");
	    globus_libc_fprintf(stderr, 
		"       Each subjob in envoking RSL must have "
		"GLOBUS_DUROC_SUBJOB_INDEX\n");
	    globus_libc_fprintf(stderr, 
		"       set to rank (0, 1, 2, ...) of subjob as it "
		"appears in the envoking RSL.\n");
	    exit(1);
	} /* endif */
	rsl_subjob_rank = atoi(rsl_subjob_rank_env_var);
	if (rsl_subjob_rank < 0 || rsl_subjob_rank >= *nsubjobs)
	{
	    globus_libc_fprintf(stderr, 
		"ERROR: env variable GLOBUS_DUROC_SUBJOB_INDEX "
		"%d must be >= 0 and\n", 
		rsl_subjob_rank);
	    globus_libc_fprintf(stderr, 
		"ERROR: less than the number of subjobs %d for this run.\n", 
		*nsubjobs);
	    exit(1);
	} /* endif */

	if (duroc_subjobmaster_rank)
	{
	    /* NOT master of subjob 0 */

	    sprintf(topology_buff,
		    "%d %d %d",
		    duroc_subjobmaster_rank,
		    rsl_subjob_rank,
		    my_subjob_size);

	    {
		char tag[200];

		sprintf(tag, "%s%d", 
		    SUBJOB_MASTER_TO_SUBJOB0_MASTER_T, call_idx);
		globus_duroc_runtime_inter_subjob_send(
		    (*subjob_addresses)[sj0_master_idx], /* dest */
		    tag,                                 /* tag */
		    strlen(topology_buff)+1,             /* nbytes */
		    (globus_byte_t *) topology_buff);    /* data */

		sprintf(tag, "%s%d", 
		    SUBJOB0_MASTER_TO_SUBJOB_MASTER_T, call_idx);
		globus_duroc_runtime_inter_subjob_receive(
		    tag,                       /* tag */
		    &bufflen,                  /* nbytes received? */
		    (globus_byte_t **) &buff); /* message */
	    }

	    sscanf(buff, "%d %d", nprocs, my_grank);

	    globus_libc_free(buff);
	}
	else
	{
	    /* master of subjob 0 */

	    int j;
	    int temp;
	    /* vectors len nsubjobs, all indexed by duroc_subjobmaster_rank */
	    int *rsl_ranks; /* received from other subjob masters */

	    /* NICK: exiting on these failed mallocs is not right thing */
	    /*       to do.  should set error rc and return. fix that   */
	    /*       later when i learn more about globus rc stuff.     */
	    if (!(rsl_ranks = 
		(int *) globus_libc_malloc(*nsubjobs*sizeof(int))))
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: failed malloc of %d bytes\n", 
		    *nsubjobs*sizeof(int));
		exit(1);
	    } /* endif */
	    if (!(job_sizes = 
		(int *) globus_libc_malloc(*nsubjobs*sizeof(int))))
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: failed malloc of %d bytes\n", 
		    *nsubjobs*sizeof(int));
		exit(1);
	    } /* endif */
	    if (!(g_ranks = (int *) globus_libc_malloc(*nsubjobs*sizeof(int))))
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: failed malloc of %d bytes\n", 
		    *nsubjobs*sizeof(int));
		exit(1);
	    } /* endif */

	    /* need to sort subjob_addresses so that i may associate */
	    /* (using incoming duroc_subjobmaster_rank) incoming     */
	    /* rsl_subjob_rank and my_subjob_size with dest addr     */
	    /* in subjob_addresses                                   */
	    for (i = 1; i < *nsubjobs-1; i ++)
	    {
		for (j = i; j > 0; j --)
		{
		    if ((*subjob_addresses)[j] < (*subjob_addresses)[j-1])
		    {
			temp = (*subjob_addresses)[j];
			(*subjob_addresses)[j] = (*subjob_addresses)[j-1];
			(*subjob_addresses)[j-1] = temp;
		    } /* endif */
		} /* endfor */
	    } /* endfor */

	    /* rsl_ranks[] and job_sizes[] are indexed by  */
	    /* duroc_subjobmaster_rank, and i know that my */
	    /* duroc_subjobmaster_rank==0                  */
	    rsl_ranks[0] = rsl_subjob_rank;
	    job_sizes[0] = my_subjob_size;

	    {
		char tag[200];

		sprintf(tag, "%s%d", 
		    SUBJOB_MASTER_TO_SUBJOB0_MASTER_T, call_idx);

		for (i = 1; i < *nsubjobs; i ++)
		{
		    int ranks, sizes;
		    
		    /*
		     * receiving 3 longs from other subjob master
		     *    duroc_subjobmaster_rank (used to index job_sizes[] 
		     *                             and rsl_ranks[])
		     *    rsl_subjob_rank 
		     *    my_subjob_size
		     */

		    globus_duroc_runtime_inter_subjob_receive(
			tag,                       /* tag */
			&bufflen,                  /* nbytes received? */
			(globus_byte_t **) &buff); /* message */

		    sscanf(buff, "%d %d %d", &j, &ranks, &sizes);
		    rsl_ranks[j] = ranks;
		    job_sizes[j] = sizes;
		    
		    globus_libc_free(buff);
		} /* endfor */
	    }

	    /* calculating nprocs and everyones' g_rank based */
	    /* on rsl_rank and job_sizes ...                  */
	    /* mygrank = sum job_size for all rsl_ranks       */
	    /*           that are less than mine              */
	    for (i = 0, *nprocs = 0; i < *nsubjobs; i ++)
	    {
		(*nprocs) += job_sizes[i];
		for (g_ranks[i] = 0, j = 0; j < *nsubjobs; j ++)
		if (rsl_ranks[i] > rsl_ranks[j])
		    g_ranks[i] += job_sizes[j];
	    } /* endfor */
	    *my_grank = g_ranks[0];

	    {
		char tag[200];

		sprintf(tag, "%s%d", 
		    SUBJOB0_MASTER_TO_SUBJOB_MASTER_T, call_idx);

		/* sending other subjob masters nprocs and their g_rank */
		for (i = 0; i < *nsubjobs-1; i ++)
		{
		    sprintf(topology_buff, "%d %d", *nprocs, g_ranks[i+1]);
		    globus_duroc_runtime_inter_subjob_send(
			(*subjob_addresses)[i],           /* dest */
			tag,                              /* tag */
			strlen(topology_buff)+1,          /* nbytes */
			(globus_byte_t *) topology_buff); /* data */
		} /* endfor */
	    }

	    globus_libc_free(rsl_ranks);
	    globus_libc_free(job_sizes);
	    globus_libc_free(g_ranks);

	} /* endif */

	/* all subjob masters sending nprocs and their g_rank to their slaves */
#       if defined(MPIG_VMPI)
	{
	    int v[2];
	    int rc;

	    v[0] = *nprocs;
	    v[1] = *my_grank;

	    /* MPI_Bcast(v, 2, MPI_INT, 0, MPI_COMM_WORLD); */
	    rc = vmpi_error_to_mpich_error(
		    mp_bootstrap_bcast((void *) v, /* buff */
					2,          /* count */
					0));        /* type, 0 == vMPI_INT */
	    if (rc != MPI_SUCCESS)
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: get_topology(): erroneous rc = %d from "
		    "mp_bootstrap_bcast (root)\n",
		    rc);
		exit(1);
	    } /* endif */
	}
#       else
        {
	    {
		char tag[200];

		sprintf(tag, "%s%d", SUBJOB_MASTER_TO_SLAVE_T, call_idx);

		for (i = 1; i < my_subjob_size; i ++)
		{
		    sprintf(topology_buff, "%d %d", *nprocs, (*my_grank) + i);
		    intra_subjob_send(i,                     /* dest */
				    tag,                     /* tag */
				    strlen(topology_buff)+1, /* nbytes */
				    topology_buff);          /* data */
		} /* endfor */
	    }
        }
#       endif
    } /* endif */

} /* end get_topology() */

MPIG_STATIC void distribute_byte_array(globus_byte_t *inbuff,
				       int inbufflen,
				       int rank_in_my_subjob,
				       int my_subjob_size,
				       int *subjob_addresses,
				       int nprocs,
				       int nsubjobs,
				       int my_grank,
				       globus_byte_t **outbuffs,
				       int *outbufflens)
{

    int i;
    char *buff;
    int bufflen;
    static unsigned int call_idx = 0;

/* globus_libc_fprintf(stderr, "%d: enter distribute_byte_array: rank_in_my_subjob %d %d bytes\n", MPID_MyWorldRank,
   rank_in_my_subjob, inbufflen); */
    call_idx ++;

    /* initialization */
    for (i = 0; i < nprocs; i ++)
    {
	outbuffs[i]    = (globus_byte_t *) NULL;
	outbufflens[i] = 0;
    } /* endfor */

    if (rank_in_my_subjob)
    {
/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: i am subjob slave\n", MPID_MyWorldRank); */
	/* subjob slave */
#       if defined(MPIG_VMPI)
        {
	    int rc;

	    /* MPI_Gather(&inbufflen,  */
			/* 1, */
			/* MPI_INT, */
			/* (void *) NULL,  */
			/* 1, */
			/* MPI_INT, */
			/* 0, */
			/* MPI_COMM_WORLD); */
	    rc = vmpi_error_to_mpich_error(
		    mp_bootstrap_gather((void *) &inbufflen,   /* sendbuff */
				1,                   /* sendcount */
				(void *) NULL,       /* rcvbuff, significant 
					                at root only */
				1));                  /* rcvcount, significant
					                at root only */
	    if (rc != MPI_SUCCESS)
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: distribute_byte_array(): erroneous rc = %d from "
		    "mp_bootstrap_gather (non-root)\n",
		    rc);
		exit(1);
	    } /* endif */

	    /* MPI_Gatherv((char *) inbuff,  */
			/* inbufflen,  */
			/* MPI_CHAR,  */
			/* (void *) NULL,  */
			/* (int *) NULL,  */
			/* (int *) NULL,  */
			/* MPI_CHAR, */
			/* 0, */
			/* MPI_COMM_WORLD); */
	    rc = vmpi_error_to_mpich_error(
		    mp_bootstrap_gatherv((void *) inbuff,  /* sendbuff */
				inbufflen,           /* sendcount */
				(void *) NULL,       /* rcvbuff, significant 
					                at root only */
				(int *) NULL,        /* rcvcounts, significant
					                at root only */
				(int *) NULL));      /* displs, significant
					                at root only */
	    if (rc != MPI_SUCCESS)
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: distribute_byte_array(): erroneous rc = %d from "
		    "mp_bootstrap_gatherv (non-root)\n",
		    rc);
		exit(1);
	    } /* endif */

        }
#       else
        {
	    char *bigbuff;
	    char *t;
	    char tagged_intrabuff[GRAM_MYJOB_MAX_BUFFER_LENGTH];

	    if (sizeof(tagged_intrabuff) < (2*HEADERLEN)+inbufflen)
	    {
		if (!(bigbuff = 
		    (char *) globus_libc_malloc((2*HEADERLEN)+inbufflen)))
		{
		    globus_libc_fprintf(stderr, 
			"ERROR: failed malloc of %d bytes\n", 
			(2*HEADERLEN)+inbufflen);
		    exit(1);
		} /* endif */
		t = bigbuff;
	    }
	    else
	    {
		t = tagged_intrabuff;
		bigbuff = (char *) NULL;
	    } /* endif */

	    /* tagging and copying my byte array for distribution */
	    sprintf(t,              "%d ",  my_grank);
	    sprintf(t+HEADERLEN,    "%d ",  inbufflen);
	    memcpy(t+(2*HEADERLEN), inbuff, inbufflen);

	    {
		char tag[200];
		sprintf(tag, "%s%d", SUBJOB_SLAVE_TO_MASTER_D, call_idx);
		/* send my byte array to my master */
		intra_subjob_gather(rank_in_my_subjob,
				    my_subjob_size,
				    t,                        /* data */
				    (2*HEADERLEN)+inbufflen,  /* nbytes */
				    tag,                      /* tag */
				    (int *) NULL,         /* subjob mstr only */
				    (char **) NULL);      /* subjob mstr only */
	    }

	    if (bigbuff)
		globus_libc_free(bigbuff);
	}
#       endif
	
	/* receiving all other byte arrays from my master */
	i = 0; 
	while (i < nprocs)
	{
	    char *rbuff;
	    int nbuffs;
/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: subjob slave: top of while loop i %d nprocs %d\n",
   MPID_MyWorldRank, i, nprocs); */

#           if defined(MPIG_VMPI)
            {
		int bsize;
		int rc;

		/* MPI_Bcast(&bsize, 1, MPI_INT, 0, MPI_COMM_WORLD); */
		rc = vmpi_error_to_mpich_error(
			/* NICK */
			mp_bootstrap_bcast((void *) &bsize, /* buff */
					    1,   /* count */
					    0)); /* type, 0 == vMPI_INT */
		if (rc != MPI_SUCCESS)
		{
		    globus_libc_fprintf(stderr, 
			"ERROR: distribute_byte_array(): erroneous rc = %d "
			"from mp_bootstrap_bcast (non-root, int)\n",
			rc);
		    exit(1);
		} /* endif */

		if (!(rbuff = (char *) globus_libc_malloc(bsize)))
		{
		    globus_libc_fprintf(stderr, 
			"ERROR: distribute_byte_array(): "
			"failed malloc of %d bytes\n", 
			bsize);
		    exit(1);
		} /* endif */

		/* MPI_Bcast(rbuff, bsize, MPI_CHAR, 0, MPI_COMM_WORLD); */
		rc = vmpi_error_to_mpich_error(
			mp_bootstrap_bcast((void *) rbuff, /* buff */
					    bsize, /* count */
					    1));   /* type, 1 == vMPI_CHAR */
		if (rc != MPI_SUCCESS)
		{
		    globus_libc_fprintf(stderr, 
			"ERROR: distribute_byte_array(): erroneous rc = %d "
			"from mp_bootstrap_bcast (non-root, char)\n",
			rc);
		    exit(1);
		} /* endif */

	    }
#           else
            {
		char tag[200];
		sprintf(tag, "%s%d", SUBJOB_MASTER_TO_SLAVE_D, call_idx);
/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: subjob slave: before intra_subjob_bcast\n", MPID_MyWorldRank); */
		intra_subjob_bcast(rank_in_my_subjob,
				    my_subjob_size,
				    tag,              /* tag */
				    &bufflen,         /* nbytes rcvd? */
				    &rbuff);          /* message */
/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: subjob slave: after intra_subjob_bcast\n", MPID_MyWorldRank); */
	    }
#           endif

	    extract_byte_arrays(rbuff, &nbuffs, outbuffs, outbufflens);
	    globus_libc_free(rbuff);
	    i += nbuffs;

	} /* endwhile */
    }
    else 
    {
	/* subjob master */
	char *my_subjob_buff;
	int my_subjob_buffsize;
/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: i am subjob master\n", MPID_MyWorldRank); */

#       if defined(MPIG_VMPI)
        {
	    char *temp_buff;
	    char *dest;
	    int *rcounts;
	    int *displs;
	    int rc;

	    if (!(rcounts = 
		    (int *) globus_libc_malloc(my_subjob_size*sizeof(int))))
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: failed malloc of %d bytes\n", 
		    (int) (my_subjob_size*sizeof(int)));
		exit(1);
	    } /* endif */
	    if (!(displs = 
		    (int *) globus_libc_malloc(my_subjob_size*sizeof(int))))
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: failed malloc of %d bytes\n", 
		    (int) (my_subjob_size*sizeof(int)));
		exit(1);
	    } /* endif */

	    /* MPI_Gather(&inbufflen,  */
			/* 1,  */
			/* MPI_INT,  */
			/* rcounts,  */
			/* 1, */
			/* MPI_INT, */
			/* 0,  */
			/* MPI_COMM_WORLD); */
	    rc = vmpi_error_to_mpich_error(
		    mp_bootstrap_gather((void *) &inbufflen,   /* sendbuff */
				1,                   /* sendcount */
				(void *) rcounts,    /* rcvbuff  */
				1));                 /* rcvcount */

	    if (rc != MPI_SUCCESS)
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: distribute_byte_array(): erroneous rc = %d from "
		    "mp_bootstrap_gather (root)\n",
		    rc);
		exit(1);
	    } /* endif */

	    my_subjob_buffsize = displs[0] = 0;
	    for (i = 0; i < my_subjob_size; i ++)
	    {
		my_subjob_buffsize += rcounts[i];
		if (i)
		    displs[i] = displs[i-1] + rcounts[i-1];
	    } /* endfor */
	    if (!(temp_buff = (char *) globus_libc_malloc(my_subjob_buffsize)))
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: failed malloc of %d bytes\n", my_subjob_buffsize);
		exit(1);
	    } /* endif */

	    /* MPI_Gatherv((char *) inbuff,  */
			/* inbufflen,  */
			/* MPI_CHAR,  */
			/* temp_buff,  */
			/* rcounts,  */
			/* displs,  */
			/* MPI_CHAR,  */
			/* 0,  */
			/* MPI_COMM_WORLD); */
	    rc = vmpi_error_to_mpich_error(
		    mp_bootstrap_gatherv((void *) inbuff,   /* sendbuff */
				inbufflen,            /* sendcount */
				(void *) temp_buff,   /* rcvbuff */
				rcounts,              /* rcvcounts */
				displs));             /* displacements */

	    if (rc != MPI_SUCCESS)
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: distribute_byte_array(): erroneous rc = %d from "
		    "mp_bootstrap_gatherv (root)\n",
		    rc);
		exit(1);
	    } /* endif */

	    my_subjob_buffsize += (HEADERLEN+(my_subjob_size*2*HEADERLEN));
	    if (!(my_subjob_buff = 
		    (char *) globus_libc_malloc(my_subjob_buffsize)))
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: failed malloc of %d bytes\n", my_subjob_buffsize);
		exit(1);
	    } /* endif */
	    sprintf(my_subjob_buff,               "%d ",  my_subjob_size);
	    sprintf(my_subjob_buff+HEADERLEN,     "%d ",  my_grank);
	    sprintf(my_subjob_buff+(2*HEADERLEN), "%d ",  inbufflen);

	    memcpy(my_subjob_buff+(3*HEADERLEN),  inbuff, inbufflen);

	    dest = my_subjob_buff + (3*HEADERLEN) + inbufflen;

	    /* filling the rest of my_subjob_buff */
	    for (i = 1; i < my_subjob_size; i ++)
	    {
		sprintf(dest, "%d ", my_grank+i);
		sprintf(dest+HEADERLEN, "%d ", rcounts[i]);
		memcpy(dest+(2*HEADERLEN), temp_buff+displs[i], rcounts[i]);
		dest += ((2*HEADERLEN)+rcounts[i]);
	    } /* endfor */

	    globus_libc_free(temp_buff);
	    globus_libc_free(rcounts);
	    globus_libc_free(displs);
	}
#       else
	{
	    /* 
	     * constructing inter-subjob message MY subjob 
	     * to pass around ring of subjob masters 
	     */
	    
	    char *t;
	    int t_len = (3*HEADERLEN)+inbufflen;

	    if (!(t = (char *) globus_libc_malloc(t_len)))
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: failed malloc of %d bytes\n", t_len);
		exit(1);
	    } /* endif */

	    sprintf(t,               "%d ",  my_subjob_size);
	    sprintf(t+HEADERLEN,     "%d ",  my_grank);
	    sprintf(t+(2*HEADERLEN), "%d ",  inbufflen);
	    memcpy(t+(3*HEADERLEN),  inbuff, inbufflen);

	    {
		char tag[200];
		sprintf(tag, "%s%d", SUBJOB_SLAVE_TO_MASTER_D, call_idx);

		intra_subjob_gather(rank_in_my_subjob,
				    my_subjob_size,
				    t,
				    t_len,
				    tag,                 /* tag */
				    &my_subjob_buffsize, /* nbytes rcvd? */
				    &my_subjob_buff);    /* message */
	    }
	    globus_libc_free(t);
	}
#       endif

	extract_byte_arrays(my_subjob_buff, (int *) NULL, outbuffs,outbufflens);

#       if defined(MPIG_VMPI)
        {
	    int rc;

	    /* MPI_Bcast(&my_subjob_buffsize, 1, MPI_INT, 0, MPI_COMM_WORLD); */
	    rc = vmpi_error_to_mpich_error(
		    mp_bootstrap_bcast((void *) &my_subjob_buffsize, /* buff */
					1,   /* count */
					0)); /* type, 0 == vMPI_INT */
	    if (rc != MPI_SUCCESS)
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: distribute_byte_array(): erroneous rc = %d "
		    "from mp_bootstrap_bcast (root, int)\n",
		    rc);
		exit(1);
	    } /* endif */

	    /* MPI_Bcast(my_subjob_buff,  */
			/* my_subjob_buffsize,  */
			/* MPI_CHAR,  */
			/* 0,  */
			/* MPI_COMM_WORLD); */
	    rc = vmpi_error_to_mpich_error(
		    mp_bootstrap_bcast((void *) my_subjob_buff, /* buff */
					my_subjob_buffsize,     /* count */
					1)); /* type, 0 == vMPI_CHAR */
	    if (rc != MPI_SUCCESS)
	    {
		globus_libc_fprintf(stderr, 
		    "ERROR: distribute_byte_array(): erroneous rc = %d "
		    "from mp_bootstrap_bcast (root, char)\n",
		    rc);
		exit(1);
	    } /* endif */

	}
#       else
        {
	    char tag[200];

	    /* sending inter-subjob message for MY subjob to all my slaves */
	    sprintf(tag, "%s%d", SUBJOB_MASTER_TO_SLAVE_D, call_idx);
/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: subjob master: before intra_subjob_bcast our subjob\n",
   MPID_MyWorldRank); */
	    intra_subjob_bcast(rank_in_my_subjob,
				my_subjob_size,
				tag,                 /* tag */
				&my_subjob_buffsize, /* nbytes bcast */
				&my_subjob_buff);    /* message */
/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: subjob master: after intra_subjob_bcast our subjob\n",
   MPID_MyWorldRank); */
	}
#       endif

	{
	    char tag[200];
	    sprintf(tag, "%s%d", SUBJOB_MASTER_TO_SUBJOB_MASTER_D, call_idx);

	    /* 
	     * sending inter-subjob message for MY subjob 
	     * to other subjob masters 
	     */
	    for (i = 0; i < nsubjobs-1; i ++)
	    {
		globus_duroc_runtime_inter_subjob_send( 
		    subjob_addresses[i],                 /* dest */ 
		    tag,                                 /* tag */
		    my_subjob_buffsize,                  /* nbytes */
		    (globus_byte_t *) my_subjob_buff);   /* data */
	    } /* endfor */
	}

	globus_libc_free(my_subjob_buff);

	/* receiving subjob byte arrays from other subjob masters */
	for (i = 0; i < nsubjobs-1; i ++)
	{
/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: subjob master: top of for loop i %d nsubjobs-1 %d\n",
   MPID_MyWorldRank, i, nsubjobs-1); */
	    {
		char tag[200];
		sprintf(tag, "%s%d", 
		    SUBJOB_MASTER_TO_SUBJOB_MASTER_D, call_idx);

		globus_duroc_runtime_inter_subjob_receive(
		    tag,                        /* tag */
		    &bufflen,                   /* nbytes received? */
		    (globus_byte_t **) &buff);  /* message */
	    }

#           if defined(MPIG_VMPI)
	    {
		int rc;

		/* MPI_Bcast(&bufflen, 1, MPI_INT, 0, MPI_COMM_WORLD); */
		rc = vmpi_error_to_mpich_error(
			mp_bootstrap_bcast((void *) &bufflen, /* buff */
					    1,                /* count */
					    0)); /* type, 0 == vMPI_INT */
		if (rc != MPI_SUCCESS)
		{
		    globus_libc_fprintf(stderr, 
			"ERROR: distribute_byte_array(): erroneous rc = %d "
			"from mp_bootstrap_bcast (root, int, 2)\n",
			rc);
		    exit(1);
		} /* endif */

		/* MPI_Bcast(buff, bufflen, MPI_CHAR, 0, MPI_COMM_WORLD); */
		rc = vmpi_error_to_mpich_error(
			mp_bootstrap_bcast((void *) buff, /* buff */
					    bufflen,      /* count */
					    1)); /* type, 1 == vMPI_CHAR */
		if (rc != MPI_SUCCESS)
		{
		    globus_libc_fprintf(stderr, 
			"ERROR: distribute_byte_array(): erroneous rc = %d "
			"from mp_bootstrap_bcast (root, char, 2)\n",
			rc);
		    exit(1);
		} /* endif */

	    }
#           else
	    {
		/* bcasting inter-subjob message to all my slaves */
		char tag[200];
		sprintf(tag, "%s%d", SUBJOB_MASTER_TO_SLAVE_D, call_idx);
/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: subjob master: before intra_subjob_bcast other subjob\n",
   MPID_MyWorldRank); */
		intra_subjob_bcast(rank_in_my_subjob,
				    my_subjob_size,
				    tag,              /* tag */
				    &bufflen,         /* nbytes bcast */
				    &buff);           /* message */
/* globus_libc_fprintf(stderr, "%d: distribute_byte_array: subjob master: after intra_subjob_bcast other subjob\n",
   MPID_MyWorldRank); */
	    }
#           endif

	    extract_byte_arrays(buff, (int *) NULL, outbuffs, outbufflens);
	    globus_libc_free(buff);
	} /* endfor */
    } /* endif */

/* globus_libc_fprintf(stderr, "%d: exit distribute_byte_array: rank_in_my_subjob %d %d bytes\n",
   MPID_MyWorldRank, rank_in_my_subjob, inbufflen); */

} /* end distribute_byte_array() */

#if !defined(MPIG_VMPI)
MPIG_STATIC void intra_subjob_send(int dest, char *tag_base, int nbytes, char *buff)
{
    char tag[100];
    char *bigtag;
    char *t;
    int i;
    /* NICK: This is a hack because globus_duroc_runtime_intra_subjob_send
     *       dictates that the tag+message must fit into a buffer the
     *       size of GRAM_MYJOB_MAX_BUFFER_LENGTH-10 and they ain't 
     *       likely gonna fix this Globus code ever ... they've moved on 
     *       to Web-services and have abandonded all this DUROC code for good.
     */
    /* char send_buff[GRAM_MYJOB_MAX_BUFFER_LENGTH]; */
    char send_buff[GRAM_MYJOB_MAX_BUFFER_LENGTH-15];
    int max_payload_size = GRAM_MYJOB_MAX_BUFFER_LENGTH - 10 
			    - strlen(tag_base) - 5;
    char *src;
    int bytes_sent;
    int ncpy;


    if (strlen(tag_base)+5 > sizeof(tag))
    {
	if (!(bigtag = (char *) globus_libc_malloc(strlen(tag_base)+5)))
	{
	    globus_libc_fprintf(stderr,
		"ERROR: failed malloc of %d bytes\n", 
		((int) strlen(tag_base))+5);
	    exit(1);
	} /* endif */
	t = bigtag;
    }
    else
    {
	bigtag = (char *) NULL;
	t = tag;
    } /* endif */

    /* sending as much as i can in the first buffer */
    sprintf(send_buff, "%d ", nbytes);
    ncpy = max_payload_size-HEADERLEN < nbytes 
	    ? max_payload_size-HEADERLEN 
	    : nbytes;

    memcpy(send_buff+HEADERLEN, buff, ncpy);

    sprintf(t, "%s0", tag_base);
    globus_duroc_runtime_intra_subjob_send(
	dest,                         /* dest */
	t,                            /* tag */
	HEADERLEN+ncpy,               /* nbytes */
	(globus_byte_t *) send_buff); /* data */

    /* pushing out remaining data */
    for (i = 1, bytes_sent = ncpy, src = buff+ncpy; bytes_sent < nbytes; i ++)
    {
	ncpy = max_payload_size < nbytes-bytes_sent
		? max_payload_size
		: nbytes-bytes_sent;
	memcpy(send_buff, src, ncpy);
	sprintf(t, "%s%d", tag_base, i);
	globus_duroc_runtime_intra_subjob_send(
	    dest,                         /* dest */
	    t,                            /* tag */
	    ncpy,                         /* nbytes */
	    (globus_byte_t *) send_buff); /* data */

	bytes_sent += ncpy;
	src        += ncpy;
    } /* endfor */

    /* clean-up */
    if (bigtag)
	globus_libc_free(bigtag);

} /* end intra_subjob_send() */

MPIG_STATIC void intra_subjob_receive(char *tag_base, int *rcvd_nbytes, char **buff)
{
    char tag[100];
    char *bigtag;
    char *t;
    int i;
    char rcv_buff[GRAM_MYJOB_MAX_BUFFER_LENGTH];
    int nr;
    char *dest;
    int bytes_rcvd;

    if (strlen(tag_base) > sizeof(tag)+5)
    {
	if (!(bigtag = (char *) globus_libc_malloc(strlen(tag_base)+5)))
	{
	    globus_libc_fprintf(stderr,
		"ERROR: failed malloc of %d bytes\n", 
		((int) strlen(tag_base))+5);
	    exit(1);
	} /* endif */
	t = bigtag;
    }
    else
    {
	bigtag = (char *) NULL;
	t = tag;
    } /* endif */

    /* rcv as much as i can in the first buffer */
    sprintf(t, "%s0", tag_base);
    globus_duroc_runtime_intra_subjob_receive(
	t,                           /* tag */
	&nr,                         /* nbytes received? */
	(globus_byte_t *) rcv_buff); /* message */
    sscanf(rcv_buff, "%d ", rcvd_nbytes);
    if (!(*buff = (char *) globus_libc_malloc(*rcvd_nbytes)))
    {
	globus_libc_fprintf(stderr,
	    "ERROR: failed malloc of %d bytes\n", *rcvd_nbytes);
	exit(1);
    } /* endif */

    memcpy(*buff, rcv_buff+HEADERLEN, nr-HEADERLEN);

    /* receiving remaining data */
    for (i = 1, bytes_rcvd = nr-HEADERLEN, dest = *buff+(nr-HEADERLEN); 
	bytes_rcvd < *rcvd_nbytes; i ++)
    {
	sprintf(t, "%s%d", tag_base, i);
	globus_duroc_runtime_intra_subjob_receive(
	    t,                           /* tag */
	    &nr,                         /* nbytes received? */
	    (globus_byte_t *) rcv_buff); /* message */
	memcpy(dest, rcv_buff, nr);

	bytes_rcvd += nr;
	dest       += nr;

    } /* endfor */

    /* clean-up */
    if (bigtag)
	globus_libc_free(bigtag);

} /* end intra_subjob_receive() */
#endif /* !defined(MPIG_VMPI) */

MPIG_STATIC void extract_byte_arrays(char *rbuff, 
				     int *nbuffs_p,  /* optional */
				     globus_byte_t **outbuffs, 
				     int *outbufflens)
{
    char *src;
    int nbuffs;
    int j;

    src = rbuff + HEADERLEN;
    sscanf(rbuff, "%d ", &nbuffs);
    if (nbuffs_p)
	*nbuffs_p = nbuffs;

    for (j = 0; j < nbuffs; j ++)
    {
	int id;

	sscanf(src, "%d ", &id);
/* globus_libc_fprintf(stderr, "%d: extract_byte_arrays: FOO extracting rank %d\n", MPID_MyWorldRank, id); */

	if (outbuffs[id])
	{
	    globus_libc_fprintf(stderr, "ERROR(%d): just rcvd second byte array from %d\n", mpig_process.my_pg_rank, id);
	    exit(1);
	} /* endif */

	sscanf(src+HEADERLEN, "%d ", outbufflens+id);
	if (!(outbuffs[id] = 
	    (globus_byte_t *) globus_libc_malloc(outbufflens[id])))
	{
	    globus_libc_fprintf(stderr, 
		"ERROR: failed malloc of %d bytes\n", outbufflens[id]);
	    exit(1);
	} /* endif */

	memcpy(outbuffs[id], src+(2*HEADERLEN), outbufflens[id]);

	src += ((2*HEADERLEN)+outbufflens[id]);
    } /* endfor */

} /* end extract_byte_arrays() */

#if !defined(MPIG_VMPI)
/*
 *
 * NOTE: both bcast/gather assumes root is always rank_in_my_subjob=0
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

MPIG_STATIC void intra_subjob_bcast(int rank_in_my_subjob, 
				    int my_subjob_size, 
				    char *tag_base, 
				    int *rcvd_nbytes, 
				    char **buff)
{
    int mask;

    /*
     *   Do subdivision.  There are two phases:
     *   1. Wait for arrival of data.  Because of the power of two nature
     *      of the subtree roots, the source of this message is alwyas the
     *      process whose relative rank has the least significant bit CLEARED.
     *      That is, process 4 (100) receives from process 0, process 7 (111) 
     *      from process 6 (110), etc.   
     *   2. Forward to my subtree
     *
     *   Note that the process that is the tree root is handled automatically
     *   by this code, since it has no bits set.
     */

    /* loop to find LSB */
    for (mask = 0x1;
	mask < my_subjob_size && !(rank_in_my_subjob & mask);
	    mask <<= 1)
    ;

    if (rank_in_my_subjob & mask) 
    {
	/* i am not root */
	intra_subjob_receive(tag_base,    /* tag */
			    rcvd_nbytes,  /* nbytes rcvd? */
			    buff);        /* message */
    } /* endif */

    /*
     *   This process is responsible for all processes that have bits set from
     *   the LSB upto (but not including) mask.  Because of the "not including",
     *   we start by shifting mask back down one.
     */
    mask >>= 1;
    while (mask > 0) 
    {
	if (rank_in_my_subjob + mask < my_subjob_size) 
	{
	    intra_subjob_send(rank_in_my_subjob+mask, /* dest */
			    tag_base,                 /* tag */
			    *rcvd_nbytes,             /* nbytes */
			    *buff);                   /* data */
	} /* endif */
	mask >>= 1;
    } /* endwhile */

} /* end intra_subjob_bcast() */

MPIG_STATIC void intra_subjob_gather(int rank_in_my_subjob,
				     int my_subjob_size,
				     char *inbuff,
				     int inbufflen,
				     char *tag_base, 
				     int *rcvd_nbytes, /* subjob master only */
				     char **buff)      /* subjob master only */
{

    int mask;
    int my_subjob_buffsize;
    char *my_subjob_buff;
    char *dest;
    char *tag;

/* printf("rank_in_my_subjob %d: enter intra_subjob_gather\n", rank_in_my_subjob); fflush(stdout); */
    if (!(tag = (char *) globus_libc_malloc(strlen(tag_base)+10)))
    {
	globus_libc_fprintf(stderr, 
	    "ERROR: failed tag malloc of %d bytes\n", strlen(tag_base)+10);
	exit(1);
    } /* endif */

    /* 
     * take a guess of how big my_subjob_buff needs to be 
     * based on my inbuff size and the size of my subjob
     */

    my_subjob_buffsize = my_subjob_size*inbufflen+100;
    if (!(my_subjob_buff = (char *) globus_libc_malloc(my_subjob_buffsize)))
    {
	globus_libc_fprintf(stderr, 
	    "ERROR: failed my_subjob_buff malloc of %d bytes\n", 
	    my_subjob_buffsize);
	exit(1);
    } /* endif */

    memcpy(my_subjob_buff,  inbuff, inbufflen);
    dest = my_subjob_buff + inbufflen;

    for (mask = 0x1; 
	mask < my_subjob_size && !(rank_in_my_subjob & mask); 
	    mask <<= 1)
    {
	if (rank_in_my_subjob+mask < my_subjob_size)
	{
	    int bufflen;
	    char *rbuff;

	    sprintf(tag, "%s%d", tag_base, rank_in_my_subjob+mask);
	    intra_subjob_receive(tag,      /* tag */
				&bufflen,  /* nbytes received? */
				&rbuff);   /* message */

	    if (my_subjob_buffsize-(dest-my_subjob_buff) < bufflen)
	    {
		/* have to re-alloc */
		int disp = dest-my_subjob_buff;

		my_subjob_buffsize += (my_subjob_size*inbufflen+100);

		if (!(my_subjob_buff = 
			(char *) globus_libc_realloc(my_subjob_buff, 
						    my_subjob_buffsize)))
		{
		    globus_libc_fprintf(stderr, 
			"ERROR: failed realloc of %d bytes\n", 
			my_subjob_buffsize);
		    exit(1);
		} /* endif */

		dest = my_subjob_buff+disp;

	    } /* endif */

	    memcpy(dest, rbuff, bufflen);
	    dest += bufflen;

	    globus_libc_free(rbuff);
	} /* endif */
    } /* endfor */

    if (rank_in_my_subjob)
    {
	/* subjob slave */
	sprintf(tag, "%s%d", tag_base, rank_in_my_subjob);
	intra_subjob_send(rank_in_my_subjob - mask, /* dest */
			tag,                        /* tag */
			dest-my_subjob_buff,        /* nbytes */
			my_subjob_buff);            /* data */
	globus_libc_free(my_subjob_buff);
    } 
    else
    {
	/* subjob master */
	*rcvd_nbytes = dest-my_subjob_buff;
	*buff        = my_subjob_buff;
    } /* endif */

    globus_libc_free(tag);

} /* end intra_subjob_gather() */

#endif /* !defined(MPIG_VMPI) */

#endif /*FOO */

