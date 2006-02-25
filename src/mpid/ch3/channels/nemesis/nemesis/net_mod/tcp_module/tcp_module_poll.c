#include "tcp_module_impl.h"

//#define TRACE 

void
MPID_nem_tcp_module_poll_send( void )
{
  MPID_nem_cell_ptr_t   cell;
  MPID_nem_pkt_t         *pkt;
  int            offset;
  int            dest;
  int            len;
  int            index,grank;

  /* first, handle pending sends */
  if  (MPID_nem_tcp_n_pending_send > 0)
    {
      for(index = 0 ; index < MPID_nem_mem_region.ext_procs ;index++)
	{
	  grank = MPID_nem_mem_region.ext_ranks[index];
	  if((grank != MPID_nem_mem_region.rank ) && (!MPID_nem_tcp_internal_queue_empty (MPID_nem_tcp_nodes[grank].internal_recv_queue)))
	    {	     
#ifdef TRACE 
	      fprintf(stderr,"[%i] -- TCP RETRY SEND for %i ... \n",MPID_nem_mem_region.rank,grank);
	      /*MPID_nem_dump_queue( nodes[grank].internal_recv_queue ); */
#endif		  	  
	      pkt    = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET ( MPID_nem_tcp_nodes[grank].internal_recv_queue->head ); /* cast away volatile */
	      dest   = pkt->mpich2.dest;
	      len    = (MPID_NEM_PACKET_OPT_LEN(pkt)) - MPID_nem_tcp_nodes[dest].left2write;
	      
	      offset = write(MPID_nem_tcp_nodes[dest].desc,(char *)pkt + MPID_nem_tcp_nodes[dest].left2write, len);
#ifdef TRACE 
	      fprintf(stderr,"[%i] -- TCP RETRY SEND for %i/offset %i/remaining %i \n/pkt len : %i/curr offset : %i \n",
		      MPID_nem_mem_region.rank,
		      grank,
		      offset,
		      len,
		      MPID_NEM_PACKET_OPT_LEN(pkt),
		      MPID_nem_tcp_nodes[dest].left2write);
#endif		  	  
	      if(offset != -1)
		{
		  MPID_nem_tcp_nodes[dest].left2write += offset;
		  
		  if(MPID_nem_tcp_nodes[dest].left2write == (MPID_NEM_PACKET_OPT_LEN(pkt)))
		    {		 
#ifdef TRACE 
		      fprintf(stderr,"[%i] -- TCP SEND : sent PARTIAL MSG 2 %i len, [%i total/%i payload]\n",
			      MPID_nem_mem_region.rank,
			      offset,
			      MPID_nem_tcp_nodes[dest].left2write,
			      pkt->mpich2.datalen);
#endif		  
		      
		      MPID_nem_tcp_nodes[dest].left2write = 0;
		      MPID_nem_tcp_internal_queue_dequeue (MPID_nem_tcp_nodes[dest].internal_recv_queue, &cell);
		      MPID_nem_queue_enqueue (process_free_queue, cell); 
		      MPID_nem_tcp_n_pending_send--;
		      MPID_nem_tcp_n_pending_sends[dest]--;
		    }
		}
	      else 
		{
		  if(errno == EFAULT){
		    perror("Send : invalid pointer ");
		  }
		  else if (errno == ENOTSOCK){
		    perror("Send : invalid socket");
		    return;
		  }
		}
	    }
	}
    }

  /* handle main Q */      
  while (!MPID_nem_queue_empty (module_tcp_recv_queue))
    {      
      MPID_nem_queue_dequeue (module_tcp_recv_queue, &cell);
      pkt  = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET (cell); /* cast away volatile */
      len  = MPID_NEM_PACKET_OPT_LEN(pkt);
      dest = pkt->mpich2.dest;

      if(MPID_nem_tcp_internal_queue_empty (MPID_nem_tcp_nodes[dest].internal_recv_queue))
	{
	  offset = write(MPID_nem_tcp_nodes[dest].desc, pkt,len);
	  	  
	  if( offset == len )	
	    {
	      MPID_nem_tcp_nodes[dest].left2write = 0;
	      MPID_nem_queue_enqueue (process_free_queue, cell);	      
#ifdef TRACE
	      fprintf(stderr,"[%i] -- TCP SEND : sent ALL MSG (%i len, payload is %i)\n",
		      MPID_nem_mem_region.rank,
		      offset, 
		      pkt->mpich2.datalen);
#endif
	    }
	  else if(offset != -1)
	    {
	      MPID_nem_tcp_nodes[dest].left2write = offset;
	      MPID_nem_tcp_internal_queue_enqueue (MPID_nem_tcp_nodes[dest].internal_recv_queue, cell);
	      MPID_nem_tcp_n_pending_send++;
	      MPID_nem_tcp_n_pending_sends[dest]++;
#ifdef TRACE
	      fprintf(stderr,"[%i] -- TCP SEND : sent PARTIAL MSG 1 (%i len)\n",
		      MPID_nem_mem_region.rank,
		      offset);
#endif
	    }
	  else
	    {	      
	      if(errno == EAGAIN)
		{
		  MPID_nem_tcp_nodes[dest].left2write = 0;
		  MPID_nem_tcp_internal_queue_enqueue (MPID_nem_tcp_nodes[dest].internal_recv_queue, cell);
		  MPID_nem_tcp_n_pending_send++;   
		  MPID_nem_tcp_n_pending_sends[dest]++;   
#ifdef TRACE
		  fprintf(stderr,"[%i] -- TCP SEND : sent NO bytes MSG \n",MPID_nem_mem_region.rank);
#endif      
		}
	      else
		{
		  return;
		}
	    }
	}
      else
	{
	  MPID_nem_tcp_internal_queue_enqueue (MPID_nem_tcp_nodes[dest].internal_recv_queue, cell);
	  MPID_nem_tcp_n_pending_send++;
	  MPID_nem_tcp_n_pending_sends[dest]++;
#ifdef TRACE
	  fprintf(stderr,"[%i] -- TCP SEND : sent NO MSG : direct EnQ \n",MPID_nem_mem_region.rank);
#endif
	}
    }
}


/*#define TRACE  */

void
MPID_nem_tcp_module_poll_recv( void  )
{
    MPID_nem_cell_ptr_t   cell     = NULL;
    fd_set         read_set = MPID_nem_tcp_set;
    int            ret      = 0;
    int            index,grank,outstanding2 = 0 ;   
    int            offset;
    MPID_nem_pkt_t *pkt      = NULL;
    struct timeval time;

    time.tv_sec  = 0;
    time.tv_usec = 0;
    ret  = select( MPID_nem_tcp_max_fd, &read_set ,NULL,NULL,&time);

#ifdef TRACE    
    if(ret)
	fprintf(stderr,
		"[%i] -- RECV TCP select with ret : %i \n",
		MPID_nem_mem_region.rank,
		ret);
#endif
 
    while( (ret > 0) || (MPID_nem_tcp_outstanding > 0))
    {
	for(index = 0 ; index < MPID_nem_mem_region.ext_procs ; index++)
	{
	    grank = MPID_nem_mem_region.ext_ranks[index];
	    if(FD_ISSET(MPID_nem_tcp_nodes[grank].desc,&read_set))
	    {
		FD_CLR(MPID_nem_tcp_nodes[grank].desc,&read_set);
		ret--;
		MPID_nem_tcp_nodes[grank].toread = 0;
		MPID_nem_tcp_outstanding         = 0;

#ifdef TRACE
		fprintf(stderr,"[%i] -- RECV TCP READ : desc is %i (index %i)\n",
			MPID_nem_mem_region.rank, 
			MPID_nem_tcp_nodes[grank].desc,
			index);
		//MPID_nem_dump_queue( nodes[grank].internal_free_queue );
#endif	    

		main_routine :
		    /* handle pending recvs */
		    if(!MPID_nem_tcp_internal_queue_empty (MPID_nem_tcp_nodes[grank].internal_free_queue))
		    {
			pkt = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET (MPID_nem_tcp_nodes[grank].internal_free_queue->head); /* cast away volatile */
			if ((MPID_nem_tcp_nodes[grank].left2read_head > 0) && (MPID_nem_tcp_nodes[grank].left2read == 0))
			{
			    offset = read(MPID_nem_tcp_nodes[grank].desc,
					  (char *)pkt + MPID_nem_tcp_nodes[grank].left2read_head,
					  MPID_NEM_OPT_HEAD_LEN - MPID_nem_tcp_nodes[grank].left2read_head);  
			    if(offset != -1)
			    {
				MPID_nem_tcp_nodes[grank].left2read_head += offset;
				if(MPID_nem_tcp_nodes[grank].left2read_head == MPID_NEM_OPT_HEAD_LEN)
				{	      
				    MPID_nem_tcp_nodes[grank].left2read_head = 0;
				    if (pkt->mpich2.datalen > 0)
				    {
					MPID_nem_tcp_nodes[grank].left2read = pkt->mpich2.datalen - MPID_NEM_OPT_SIZE; 
					offset                 =  read(MPID_nem_tcp_nodes[grank].desc,
								       (pkt->mpich2.payload + MPID_NEM_OPT_SIZE),
								       MPID_nem_tcp_nodes[grank].left2read);
				  
				  
					if(offset != -1)
					{
					    MPID_nem_tcp_nodes[grank].left2read -= offset;
					    if (MPID_nem_tcp_nodes[grank].left2read == 0)
					    {
						MPID_nem_tcp_internal_queue_dequeue (MPID_nem_tcp_nodes[grank].internal_free_queue, &cell);
						MPID_nem_queue_enqueue (process_recv_queue, cell);	      
						MPID_nem_tcp_n_pending_recv--;
					    }
					}
					continue ;
				    }
				}
#ifdef TRACE
				else
				{
			            fprintf(stderr,"[%i] -- RECV TCP READ : NOT FULL HEAD YET !!!\n",MPID_nem_mem_region.rank);
				}
#endif
			    }
			    else
			    { 
				if(errno == ENOTSOCK)
				{
				    fprintf(stderr,"[%i] -- RECV TCP READ : SOCK 2 NOT CONNECTED\n",MPID_nem_mem_region.rank);
				    perror("socket 2");
				}
				else if (errno == EFAULT)
				{
				    perror("ptr error");
				}
			    }		      
			}
			else if (MPID_nem_tcp_nodes[grank].left2read > 0)
			{
			    offset = read(MPID_nem_tcp_nodes[grank].desc,
					  (char *)&(pkt->mpich2.payload) + (pkt->mpich2.datalen - MPID_nem_tcp_nodes[grank].left2read),    
					  MPID_nem_tcp_nodes[grank].left2read);
		      
			    if(offset != -1)
			    {
				MPID_nem_tcp_nodes[grank].left2read -= offset;			  
#ifdef TRACE 
				{
				    int index;
				    fprintf(stderr,
					    "[%i] -- RECV TCP READ : RETRY 3 for %i, got %i bytes [%i current/ %i total] \n",
					    MPID_nem_mem_region.rank,
					    grank,
					    offset,
					    (pkt->mpich2.datalen - MPID_nem_tcp_nodes[grank].left2read),
					    pkt->mpich2.datalen);

				}
#endif	

				if (MPID_nem_tcp_nodes[grank].left2read == 0)
				{
				    MPID_nem_tcp_nodes[grank].left2read_head = 0;
				    MPID_nem_tcp_internal_queue_dequeue (MPID_nem_tcp_nodes[grank].internal_free_queue, &cell);
				    MPID_nem_queue_enqueue (process_recv_queue, cell);	      
				    MPID_nem_tcp_n_pending_recv--;					  
				}
			    }
			    else{ 
				if(errno == ENOTSOCK)
				{
				    fprintf(stderr,"[%i] -- RECV TCP READ : SOCK 3 NOT CONNECTED\n",MPID_nem_mem_region.rank);
				    perror("socket 3");
				}
			
			    }
			}
		    }
		    else {
			/* handle regular Net Q */
			if (!MPID_nem_queue_empty(module_tcp_free_queue))
			{
			    MPID_nem_queue_dequeue (module_tcp_free_queue, &cell);
			    offset = read (MPID_nem_tcp_nodes[grank].desc,
					   (MPID_nem_pkt_mpich2_t *)&(cell->pkt.mpich2), /* cast away volatile */
					   MPID_NEM_OPT_HEAD_LEN);
#ifdef TRACE 			     
			     {				
				int index ;
				for(index = 0 ; index < ((cell->pkt.mpich2.datalen)/sizeof(int)); index ++)
				  {
				     fprintf(stderr,"[%i] --- Got cell[%i] : %i\n",MPID_nem_mem_region.rank,index,((int *)&(cell->pkt.mpich2))[index] );
				  }
			     }
#endif 
			   
			    if(offset != -1)
			    {
				MPID_nem_tcp_nodes[grank].left2read_head += offset;		    
				if( MPID_nem_tcp_nodes[grank].left2read_head != 0)
				{
				    if( MPID_nem_tcp_nodes[grank].left2read_head < MPID_NEM_OPT_HEAD_LEN)
				    {
#ifdef TRACE 
					fprintf(stderr,"[%i] -- RECV TCP READ : got PARTIAL header [%i bytes/ %i total] \n",
						MPID_nem_mem_region.rank,
						offset, 
						MPID_NEM_OPT_HEAD_LEN);
#endif
					MPID_nem_tcp_internal_queue_enqueue (MPID_nem_tcp_nodes[grank].internal_free_queue, cell);
					MPID_nem_tcp_n_pending_recv++;
				    }
				    else
				    {		    
				
#ifdef TRACE	
					{
					    int index;
					    fprintf(stderr,"[%i] -- RECV TCP READ : got FULL header [%i bytes/ %i total] [src %i -- dest %i -- dlen %i -- seqno %i]\n",
						    MPID_nem_mem_region.rank,
						    offset, 
						    MPID_NEM_OPT_HEAD_LEN,
						    cell->pkt.mpich2.source,
						    cell->pkt.mpich2.dest,
						    cell->pkt.mpich2.datalen,
						    cell->pkt.mpich2.seqno);
					}
#endif
					if ( (cell->pkt.mpich2.datalen) > (MPID_NEM_OPT_SIZE) )
					 {
					    MPID_nem_tcp_nodes[grank].left2read = ((cell->pkt.mpich2.datalen) - (MPID_NEM_OPT_SIZE));
					 }				       
					else
					 {					    
					    MPID_nem_tcp_nodes[grank].left2read = 0;
					 }
				       
					if(MPID_nem_tcp_nodes[grank].left2read != 0)
					{
					    offset = read(MPID_nem_tcp_nodes[grank].desc,
							  ((char *)&(cell->pkt.mpich2) + (MPID_NEM_OPT_HEAD_LEN)),
							  MPID_nem_tcp_nodes[grank].left2read );
				    
					    if(offset != -1)
					    {
#ifdef TRACE 
						{
						    int index;
						    fprintf(stderr,"[%i] -- RECV TCP READ : got  [%i bytes/ %i total] \n",
							    MPID_nem_mem_region.rank,
							    offset,
							    MPID_nem_tcp_nodes[grank].left2read);
						}
#endif				    			    
						MPID_nem_tcp_nodes[grank].left2read_head = 0;
						MPID_nem_tcp_nodes[grank].left2read     -= offset;
						if (MPID_nem_tcp_nodes[grank].left2read == 0)
						{
						    MPID_nem_queue_enqueue (process_recv_queue, cell);	      
						}
						else				     
						{
						    MPID_nem_tcp_internal_queue_enqueue (MPID_nem_tcp_nodes[grank].internal_free_queue, cell);
						    MPID_nem_tcp_n_pending_recv++;
						}
					    }
					    else 
					    {
						if(errno == ENOTSOCK)
						{
						    fprintf(stderr,"[%i] -- RECV TCP READ : SOCK 4 NOT CONNECTED\n",MPID_nem_mem_region.rank);
						    perror("socket 4");
						}
						else if (errno == EAGAIN)
						{
						    MPID_nem_tcp_internal_queue_enqueue (MPID_nem_tcp_nodes[grank].internal_free_queue, cell);
						    MPID_nem_tcp_n_pending_recv++;
						}
					    }
					}
					else{
					    MPID_nem_tcp_nodes[grank].left2read      = 0;
					    MPID_nem_tcp_nodes[grank].left2read_head = 0;
					    MPID_nem_queue_enqueue (process_recv_queue, cell);
					}
				    }
				}
				else
				{
				    /* eof i guess */
				    MPID_nem_queue_enqueue (module_tcp_free_queue, cell);
#ifdef TRACE			
				    perror("EOF SOCK");
#endif		
				}
			    }
			    else 
			    {
				if(errno == ENOTSOCK)
				{			
				    fprintf(stderr,"[%i] -- RECV TCP READ : SOCK 5 NOT CONNECTED\n",MPID_nem_mem_region.rank);
				    perror("socket 5");
				}
				if(errno == EAGAIN)
				{
				    perror("Nothing to read !!");
				}
			    }
			}
			else {
			    /* Q is empty !!! */
			    MPID_nem_tcp_nodes[grank].toread++;
			    outstanding2++;
			}
		    }
	    }
	    else
	    {
#ifdef TRACE       
		fprintf(stderr,"[%i] -- RECV TCP READ NO desc (%i) (index %i) \n",
			MPID_nem_mem_region.rank, MPID_nem_tcp_nodes[index].desc,grank);
#endif	   
		if (MPID_nem_tcp_nodes[grank].toread > 0 )
		{
		    MPID_nem_tcp_nodes[grank].toread--;
		    MPID_nem_tcp_outstanding--;
		    goto  main_routine;
		}
	    }
	}
    }
    MPID_nem_tcp_outstanding  = outstanding2;
    outstanding2 = 0;
}

void 
MPID_nem_tcp_module_poll (MPID_nem_poll_dir_t in_or_out)
{  
    if(MPID_nem_tcp_poll_freq >= 0)
    {
        if (in_or_out == MPID_NEM_POLL_OUT)
	{
	    if( MPID_nem_tcp_n_pending_send > 0 )
	    {
	        MPID_nem_tcp_module_poll_send();
		if (MPID_nem_tcp_n_pending_recv > 0)
	        {
		    MPID_nem_tcp_module_poll_recv();
		} 
		else if (--MPID_nem_tcp_poll_freq == 0) 
		{
		    MPID_nem_tcp_module_poll_recv();
		    MPID_nem_tcp_poll_freq = MPID_nem_tcp_old_poll_freq;
		}
	    }
	    else if (--MPID_nem_tcp_poll_freq == 0)
	    {
	        MPID_nem_tcp_module_poll_send();
	        MPID_nem_tcp_module_poll_recv();
	        MPID_nem_tcp_poll_freq = MPID_nem_tcp_old_poll_freq;
	    }
	}
	else 
	{
	    if( MPID_nem_tcp_n_pending_recv > 0 )
	    {
	      MPID_nem_tcp_module_poll_recv();
	      if (MPID_nem_tcp_n_pending_send > 0)
	      {
		  MPID_nem_tcp_module_poll_send();
	      }
	      else if (--MPID_nem_tcp_poll_freq == 0) 
	      {
		  MPID_nem_tcp_module_poll_send();
		  MPID_nem_tcp_poll_freq = MPID_nem_tcp_old_poll_freq;
	      }
	    }
	    else if (--MPID_nem_tcp_poll_freq == 0)
	    {
	        MPID_nem_tcp_module_poll_recv();
		MPID_nem_tcp_module_poll_send();
		MPID_nem_tcp_poll_freq = MPID_nem_tcp_old_poll_freq;
	    }
	}
    }
}

void 
MPID_nem_alt_tcp_module_poll (MPID_nem_poll_dir_t in_or_out)
{  
    if(MPID_nem_tcp_poll_freq >= 0)
    {
        if (in_or_out == MPID_NEM_POLL_OUT)
	  {
	    MPID_nem_tcp_module_poll_send();
	    MPID_nem_tcp_module_poll_recv();
	  }
	else 
	{
	  MPID_nem_tcp_module_poll_recv();
	  MPID_nem_tcp_module_poll_send();
	}
    }
}


