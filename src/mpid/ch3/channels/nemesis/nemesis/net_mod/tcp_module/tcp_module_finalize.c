#include "tcp_module.h"
#include "tcp_module_impl.h"

//#define TRACE 
#define NEM_TCP_BUF_SIZE    MPID_NEM_OPT_HEAD_LEN
#define NEM_TCP_MASTER_RANK 0

int
MPID_nem_tcp_module_finalize ()
{
  if (MPID_nem_mem_region.ext_procs > 0)
    {
      node_t *MPID_nem_tcp_nodes = MPID_nem_tcp_internal_vars.nodes ;
      char buff[NEM_TCP_BUF_SIZE] = TCP_END_STRING;
      int index;
      int grank;
      int is_master = 0;

#ifdef TRACE
      fprintf(stderr,"[%i] --- TCP END PENDING SEND \n",MPID_nem_mem_region.rank);
#endif
      while( MPID_nem_tcp_internal_vars.n_pending_send > 0 )
	{
	  MPID_nem_tcp_module_poll( MPID_NEM_POLL_OUT );
	}
      
#ifdef TRACE
      fprintf(stderr,"[%i] --- TCP END PENDING DONE  1\n",MPID_nem_mem_region.rank);
#endif
      
      
      for(index = 0 ; index < MPID_nem_mem_region.ext_procs ; index++)
	{
	  grank = MPID_nem_mem_region.ext_ranks[index];
	  if(grank <= MPID_nem_mem_region.rank) 		
	    {
	      if ((grank != MPID_nem_mem_region.rank) && (!MPID_NEM_IS_LOCAL (grank)))
	      	{
		  write(MPID_nem_tcp_nodes[grank].desc, buff,NEM_TCP_BUF_SIZE);
#ifdef TRACE
		  fprintf(stderr,"[%i] --- SLAVE WROTE TO MASTER %i on desc %i: %s, size %i\n",MPID_nem_mem_region.rank, grank, MPID_nem_tcp_nodes[grank].desc, buff,NEM_TCP_BUF_SIZE);
#endif
		}
	    }
	}
#ifdef TRACE 
      fprintf(stderr,"[%i] --- TCP END PENDING  3 : waiting for %i slaves\n",MPID_nem_mem_region.rank, MPID_nem_tcp_internal_vars.nb_slaves);      
#endif 
      while (MPID_nem_tcp_internal_vars.nb_slaves > 0)
	{
	  MPID_nem_tcp_module_poll_recv();
	}

#ifdef TRACE 
      fprintf(stderr,"[%i] --- TCP END PENDING  4 : %i slaves left \n",MPID_nem_mem_region.rank, MPID_nem_tcp_internal_vars.nb_slaves);            
#endif //TRACE 
    } 
  return MPID_nem_tcp_module_ckpt_shutdown ();    
}

int
MPID_nem_tcp_module_ckpt_shutdown ()
{
  if (MPID_nem_mem_region.ext_procs > 0)
    {  
      int index;
      int grank;
      
      /* close the sockets */
      for (index = 0 ; index < MPID_nem_mem_region.ext_procs ; index++)
	{
	  grank = MPID_nem_mem_region.ext_ranks[index];
	  if ((grank != MPID_nem_mem_region.rank) && (!MPID_NEM_IS_LOCAL (grank)))
	    {
	      shutdown ((MPID_nem_tcp_internal_vars.nodes)[grank].desc, SHUT_RDWR);
	      close ((MPID_nem_tcp_internal_vars.nodes)[grank].desc);
	    }
	}
      
#ifdef TRACE
      fprintf(stderr,"[%i] --- sockets closed .... \n",MPID_nem_mem_region.rank);
#endif
    }
  return 0 ;
}

