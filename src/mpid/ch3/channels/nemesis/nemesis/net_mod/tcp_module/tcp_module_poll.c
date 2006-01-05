#include "tcp_module_impl.h"
/********************* MPICH2 FLAVOR ***************************/

/*#define TRACE  */

void
tcp_module_poll_send( void )
{
  MPID_nem_cell_ptr_t   cell;
  MPID_nem_pkt_t         *pkt;
  int            offset;
  int            dest;
  int            len;
  int            index,grank;

  /* first, handle pending sends */
  if  (n_pending_send > 0)
    {
      for(index = 0 ; index < ext_numnodes ;index++)
	{
	  grank = ext_ranks[index];
	  if((grank != rank ) && (!internal_queue_empty (nodes[grank].internal_recv_queue)))
	    {	     
#ifdef TRACE 
	      fprintf(stderr,"[%i] -- TCP RETRY SEND for %i ... \n",rank,grank);
	      /*MPID_nem_dump_queue( nodes[grank].internal_recv_queue ); */
#endif		  	  
#ifndef MPID_NEM_USE_SHADOW_HEAD
	      pkt    = MPID_NEM_CELL_TO_PACKET ( nodes[grank].internal_recv_queue->head );
#else
	      pkt    = MPID_NEM_CELL_TO_PACKET ( nodes[grank].internal_recv_queue->my_head );
#endif
	      dest   = pkt->mpich2.dest;
	      len    = (MPID_NEM_PACKET_OPT_LEN(pkt)) - nodes[dest].left2write;
	      
	      offset = write(nodes[dest].desc,(char *)pkt + nodes[dest].left2write, len);
#ifdef TRACE 
	      fprintf(stderr,"[%i] -- TCP RETRY SEND for %i/offset %i/remaining %i \n/pkt len : %i/curr offset : %i \n",rank,grank,offset,len,MPID_NEM_PACKET_OPT_LEN(pkt),nodes[dest].left2write);
#endif		  	  
	      if(offset != -1)
		{
		  nodes[dest].left2write += offset;
		  
		  if(nodes[dest].left2write == (MPID_NEM_PACKET_OPT_LEN(pkt)))
		    {		 
#ifdef TRACE 
		      fprintf(stderr,"[%i] -- TCP SEND : sent PARTIAL MSG 2 %i len, [%i total/%i payload]\n",rank,offset,nodes[dest].left2write,pkt->mpich2.datalen);
#endif		  
		      
		      nodes[dest].left2write = 0;
		      internal_queue_dequeue (nodes[dest].internal_recv_queue, &cell);
		      MPID_nem_queue_enqueue ( process_free_queue, MPID_NEM_ABS_TO_REL( cell )); 
		      n_pending_send--;
		      n_pending_sends[dest]--;
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
      pkt  = MPID_NEM_CELL_TO_PACKET (cell);      
      len  = MPID_NEM_PACKET_OPT_LEN(pkt);
      dest = pkt->mpich2.dest;

      if(internal_queue_empty (nodes[dest].internal_recv_queue))
	{
	  offset = write(nodes[dest].desc, pkt,len);
	  	  
	  if( offset == len )	
	    {
	      nodes[dest].left2write = 0;
	      MPID_nem_queue_enqueue ( process_free_queue, MPID_NEM_ABS_TO_REL( cell ));	      
#ifdef TRACE
	      fprintf(stderr,"[%i] -- TCP SEND : sent ALL MSG (%i len, payload is %i)\n",rank,offset, pkt->mpich2.datalen);
#endif
	    }
	  else if(offset != -1)
	    {
	      nodes[dest].left2write = offset;
	      internal_queue_enqueue (nodes[dest].internal_recv_queue, cell);
	      n_pending_send++;
	      n_pending_sends[dest]++;
#ifdef TRACE
	      fprintf(stderr,"[%i] -- TCP SEND : sent PARTIAL MSG 1 (%i len)\n",rank,offset);
#endif
	    }
	  else
	    {	      
	      if(errno == EAGAIN)
		{
		  nodes[dest].left2write = 0;
		  internal_queue_enqueue (nodes[dest].internal_recv_queue, cell);
		  n_pending_send++;   
		  n_pending_sends[dest]++;   
#ifdef TRACE
		  fprintf(stderr,"[%i] -- TCP SEND : sent NO bytes MSG \n",rank);
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
	  internal_queue_enqueue (nodes[dest].internal_recv_queue, cell);
	  n_pending_send++;
	  n_pending_sends[dest]++;
#ifdef TRACE
	  fprintf(stderr,"[%i] -- TCP SEND : sent NO MSG : direct EnQ \n",rank);
#endif
	}
    }
}


/*#define TRACE  */

void
tcp_module_poll_recv( void  )
{
  MPID_nem_cell_ptr_t   cell     = NULL;
  fd_set         read_set = set;
  int            ret      = 0;
  int            index,grank,outstanding2 = 0 ;   
  int            offset;
  MPID_nem_pkt_t         *pkt      = NULL;
  struct timeval time;

  time.tv_sec  = 0;
  time.tv_usec = 0;
  ret  = select( max_fd, &read_set ,NULL,NULL,&time);

#ifdef TRACE    
  if(ret)
    fprintf(stderr,
	    "[%i] -- RECV TCP select with ret : %i \n",
	    rank,
	    ret);
#endif
 
  while( (ret > 0) || (outstanding > 0))
    {
      for(index = 0 ; index < ext_numnodes ; index++)
	{
	  grank = ext_ranks[index];
	  if(FD_ISSET(nodes[grank].desc,&read_set))
	    {
	      FD_CLR(nodes[grank].desc,&read_set);
	      ret--;
	      nodes[grank].toread = 0;
	      outstanding         = 0;

#ifdef TRACE
	      fprintf(stderr,"[%i] -- RECV TCP READ : desc is %i (index %i)\n",
		      rank, nodes[grank].desc,index);
	      MPID_nem_dump_queue( nodes[grank].internal_free_queue );
#endif	    

main_routine :
	      /* handle pending recvs */
	      if(!internal_queue_empty (nodes[grank].internal_free_queue))
		{
#ifndef MPID_NEM_USE_SHADOW_HEAD
		  pkt = MPID_NEM_CELL_TO_PACKET (nodes[grank].internal_free_queue->head);
#else
		  pkt = MPID_NEM_CELL_TO_PACKET (nodes[grank].internal_free_queue->my_head);
#endif
		  if ((nodes[grank].left2read_head > 0) && (nodes[grank].left2read == 0))
		    {
		      offset = read(nodes[grank].desc,
				    (char *)pkt + nodes[grank].left2read_head,
				    MPID_NEM_OPT_HEAD_LEN - nodes[grank].left2read_head);  
		      if(offset != -1)
			{
			  nodes[grank].left2read_head += offset;
			  if(nodes[grank].left2read_head == MPID_NEM_OPT_HEAD_LEN)
			    {	      
			      nodes[grank].left2read_head = 0;
			      if (pkt->mpich2.datalen > 0)
				{
				  nodes[grank].left2read = pkt->mpich2.datalen - MPID_NEM_OPT_SIZE; 
				  offset                 =  read(nodes[grank].desc,
								 (pkt->mpich2.payload + MPID_NEM_OPT_SIZE),
								 nodes[grank].left2read);
				  
				  
				  if(offset != -1)
				    {
				      nodes[grank].left2read -= offset;
				      if (nodes[grank].left2read == 0)
					{
					  internal_queue_dequeue (nodes[grank].internal_free_queue, &cell);
					  MPID_nem_queue_enqueue ( process_recv_queue, MPID_NEM_ABS_TO_REL( cell ));	      
					  n_pending_recv--;
					}
				    }
				  continue ;
				}
			    }
#ifdef TRACE
			  else
			    {
			            fprintf(stderr,"[%i] -- RECV TCP READ : NOT FULL HEAD YET !!!\n",rank);
			    }
#endif
			}
		      else
			{ 
			  if(errno == ENOTSOCK)
			    {
			      fprintf(stderr,"[%i] -- RECV TCP READ : SOCK 2 NOT CONNECTED\n",rank);
			      perror("socket 2");
			    }
			  else if (errno == EFAULT)
			    {
			      perror("ptr error");
			    }
			}		      
		    }
		  else if (nodes[grank].left2read > 0)
		    {
		      offset = read(nodes[grank].desc,
				    (char *)&(pkt->mpich2.payload) + (pkt->mpich2.datalen - nodes[grank].left2read),    
				    nodes[grank].left2read);
		      
		      if(offset != -1)
			{
			  nodes[grank].left2read -= offset;			  
#ifdef TRACE 
			  {
			    int index;
			    fprintf(stderr,
				    "[%i] -- RECV TCP READ : RETRY 3 for %i, got %i bytes [%i current/ %i total] \n",
				    rank,
				    grank,
				    offset,
				    (pkt->mpich2.datalen - nodes[grank].left2read),
				    pkt->mpich2.datalen);

			  }
#endif	

			  if (nodes[grank].left2read == 0)
			    {
			      nodes[grank].left2read_head = 0;
			      internal_queue_dequeue (nodes[grank].internal_free_queue, &cell);
			      MPID_nem_queue_enqueue ( process_recv_queue, MPID_NEM_ABS_TO_REL( cell ));	      
			      n_pending_recv--;					  
			    }
			}
		      else{ 
			if(errno == ENOTSOCK)
			  {
			    fprintf(stderr,"[%i] -- RECV TCP READ : SOCK 3 NOT CONNECTED\n",rank);
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
		    offset = read(nodes[grank].desc,
				  &(cell->pkt.mpich2),
				  MPID_NEM_OPT_HEAD_LEN);
		    
		    if(offset != -1)
		      {
			nodes[grank].left2read_head += offset;		    
			if( nodes[grank].left2read_head != 0)
			  {
			    if( nodes[grank].left2read_head < MPID_NEM_OPT_HEAD_LEN)
			      {
#ifdef TRACE 
				fprintf(stderr,"[%i] -- RECV TCP READ : got PARTIAL header [%i bytes/ %i total] \n",rank,offset, MPID_NEM_OPT_HEAD_LEN);
#endif
				internal_queue_enqueue (nodes[grank].internal_free_queue, cell);
				n_pending_recv++;
			      }
			    else
			      {		    
				
#ifdef TRACE	
				{
				  int index;
				  fprintf(stderr,"[%i] -- RECV TCP READ : got FULL header [%i bytes/ %i total] [src %i -- dest %i -- dlen %i -- seqno %i]\n",
					  rank,offset, MPID_NEM_OPT_HEAD_LEN,
					  cell->pkt.mpich2.source,
					  cell->pkt.mpich2.dest,
					  cell->pkt.mpich2.datalen,
					  cell->pkt.mpich2.seqno);
				}
#endif
				if (( cell->pkt.mpich2.datalen - MPID_NEM_OPT_SIZE) >= 0)
				  nodes[grank].left2read = cell->pkt.mpich2.datalen - MPID_NEM_OPT_SIZE;
				else
				  nodes[grank].left2read = 0;
				
				if(nodes[grank].left2read != 0)
				  {
				    offset = read(nodes[grank].desc,
						  (char *)&(cell->pkt.mpich2) + MPID_NEM_OPT_HEAD_LEN,
						  nodes[grank].left2read );
				    
				    if(offset != -1)
				      {
#ifdef TRACE 
					{
					  int index;
					  fprintf(stderr,"[%i] -- RECV TCP READ : got  [%i bytes/ %i total] \n",rank,offset,cell->pkt.mpich2.datalen - MPID_NEM_OPT_SIZE);
					}
#endif				    			    
					nodes[grank].left2read_head = 0;
					nodes[grank].left2read     -= offset;
					if (nodes[grank].left2read == 0)
					  {
					    MPID_nem_queue_enqueue ( process_recv_queue, MPID_NEM_ABS_TO_REL( cell ));	      
					  }
					else				     
					  {
					    internal_queue_enqueue (nodes[grank].internal_free_queue, cell);
					    n_pending_recv++;
					  }
				      }
				    else 
				      {
					if(errno == ENOTSOCK)
					  {
					    fprintf(stderr,"[%i] -- RECV TCP READ : SOCK 4 NOT CONNECTED\n",rank);
					    perror("socket 4");
					  }
					else if (errno == EAGAIN)
					  {
					    internal_queue_enqueue (nodes[grank].internal_free_queue, cell);
					    n_pending_recv++;
					  }
				      }
				  }
				else{
				  nodes[grank].left2read      = 0;
				  nodes[grank].left2read_head = 0;
				  MPID_nem_queue_enqueue ( process_recv_queue, MPID_NEM_ABS_TO_REL( cell ));
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
			    fprintf(stderr,"[%i] -- RECV TCP READ : SOCK 5 NOT CONNECTED\n",rank);
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
		  nodes[grank].toread++;
		  outstanding2++;
		}
	      }
	    }
	  else
	    {
#ifdef TRACE       
	      fprintf(stderr,"[%i] -- RECV TCP READ NO desc (%i) (index %i) \n",
		      rank, nodes[index].desc,grank);
#endif	   
	      if (nodes[grank].toread > 0 )
		{
		  nodes[grank].toread--;
                  outstanding--;
		  goto  main_routine;
		}
	    }
	}
    }
  outstanding  = outstanding2;
  outstanding2 = 0;
}

void 
tcp_module_poll( int in_or_out )
{  
    if(poll_freq >= 0)
    {
        if (in_or_out == MPID_NEM_POLL_OUT)
	{
	    if( n_pending_send > 0 )
	    {
	        tcp_module_poll_send();
		if (n_pending_recv > 0)
	        {
		    tcp_module_poll_recv();
		} 
		else if (--poll_freq == 0) 
		{
		    tcp_module_poll_recv();
		    poll_freq = old_poll_freq;
		}
	    }
	    else if (--poll_freq == 0)
	    {
	        tcp_module_poll_send();
	        tcp_module_poll_recv();
	        poll_freq = old_poll_freq;
	    }
	}
	else 
	{
	    if( n_pending_recv > 0 )
	    {
	      tcp_module_poll_recv();
	      if (n_pending_send > 0)
	      {
		  tcp_module_poll_send();
	      }
	      else if (--poll_freq == 0) 
	      {
		  tcp_module_poll_send();
		  poll_freq = old_poll_freq;
	      }
	    }
	    else if (--poll_freq == 0)
	    {
	        tcp_module_poll_recv();
		tcp_module_poll_send();
		poll_freq = old_poll_freq;
	    }
	}
    }
}

void 
__tcp_module_poll( int in_or_out )
{  
    if(poll_freq >= 0)
    {
        if (in_or_out == MPID_NEM_POLL_OUT)
	  {
	    tcp_module_poll_send();
	    tcp_module_poll_recv();
	  }
	else 
	{
	  tcp_module_poll_recv();
	  tcp_module_poll_send();
	}
    }
}

