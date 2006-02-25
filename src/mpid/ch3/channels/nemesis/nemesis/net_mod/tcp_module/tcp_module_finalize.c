#include "tcp_module.h"
#include "tcp_module_impl.h"

int
MPID_nem_tcp_module_finalize ()
{
    long int toto = 1000;
    
#ifdef TRACE
    fprintf(stderr,"[%i] --- TCP END PENDING SEND & RECV \n",MPID_nem_mem_region.rank);
#endif
    while( MPID_nem_tcp_n_pending_send > 0 )
    {
	MPID_nem_tcp_module_poll( 1 );
    }

#ifdef TRACE
    fprintf(stderr,"[%i] --- TCP END PENDING DONE  1\n",MPID_nem_mem_region.rank);
#endif
    while(--toto)
	MPID_nem_tcp_module_poll( 1 );
#ifdef TRACE
    fprintf(stderr,"[%i] --- TCP END PENDING DONE  2\n",MPID_nem_mem_region.rank);
#endif

    return MPID_nem_tcp_module_ckpt_shutdown ();    
}

int
MPID_nem_tcp_module_ckpt_shutdown ()
{
    int index;
    int grank;
    
    /* close the sockets */
    for (index = 0 ; index < MPID_nem_mem_region.ext_procs ; index++)
    {
	grank = MPID_nem_mem_region.ext_ranks[index];
	if ((grank != MPID_nem_mem_region.rank) && (!MPID_NEM_IS_LOCAL (grank)))
	{
	    shutdown (MPID_nem_tcp_nodes[grank].desc, SHUT_RDWR);
	    close (MPID_nem_tcp_nodes[grank].desc);
	}
    }

#ifdef TRACE
    fprintf(stderr,"[%i] --- sockets closed .... \n",MPID_nem_mem_region.rank);
#endif

    return 0 ;
}

