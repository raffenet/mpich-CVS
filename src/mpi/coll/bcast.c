/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Bcast */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Bcast = PMPI_Bcast
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Bcast  MPI_Bcast
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Bcast as PMPI_Bcast
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Bcast PMPI_Bcast

/* This is the default implementation of broadcast. The algorithm is:
   
   Algorithm: MPI_Bcast

   For short messages, we use a minimum spanning tree (MST) algorithm. 
   Cost = lgp.alpha + n.lgp.beta

   For long messages, we do a scatter followed by an allgather. 
   We first scatter the buffer using an MST algorithm. This costs
   lgp.alpha + n.((p-1)/p).beta
   If the datatype is contiguous and the communicator is homogeneous,
   we treat the data as bytes and divide (scatter) it among processes
   by using ceiling division. For the noncontiguous or heterogeneous
   cases, we first pack the data into a temporary buffer by using
   MPI_Pack, scatter it as bytes, and unpack it after the allgather.

   For the allgather, we use a recursive doubling algorithm. This
   takes lgp steps. In each step pairs of processes exchange all the
   data they have (we take care of non-power-of-two situations). This
   costs approximately lgp.alpha + n.((p-1)/p).beta. (Approximately
   because it may be slightly more in the non-power-of-two case, but
   it's still a logarithmic algorithm.) Therefore, for long messages
   Total Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta

   Note that this algorithm has twice the latency as the MST algorithm
   we use for short messages, but requires lower bandwidth: 2.n.beta
   versus n.lgp.beta. Therefore, for long messages and when lgp > 2,
   this algorithm will perform better.

   Possible improvements: 
   For clusters of SMPs, we may want to do something differently to
   take advantage of shared memory on each node.

   End Algorithm: MPI_Bcast
*/

PMPI_LOCAL int MPIR_Bcast ( 
	void *buffer, 
	int count, 
	MPI_Datatype datatype, 
	int root, 
	MPID_Comm *comm_ptr )
{
  MPI_Status status;
  int        rank, comm_size, src, dst;
  int        relative_rank, mask;
  int        mpi_errno = MPI_SUCCESS;
  int scatter_size, nbytes, curr_size, recv_size, send_size;
  int type_size, j, k, i, tmp_mask, is_contig, is_homogeneous;
  int relative_dst, dst_tree_root, my_tree_root, send_offset;
  int recv_offset, tree_root, nprocs_completed, offset, position;
  void *tmp_buf;
  MPI_Comm comm;

  if (comm_ptr->comm_kind == MPID_INTERCOMM) {
      printf("ERROR: MPI_Bcast for intercommunicators not yet implemented.\n");
      PMPI_Abort(MPI_COMM_WORLD, 1);
  }

  comm = comm_ptr->handle;

  if (count == 0) return MPI_SUCCESS;

  comm_size = comm_ptr->local_size;
  rank = comm_ptr->rank;
  
  /* If there is only one process, return */
  if (comm_size == 1) return mpi_errno;

  if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
      is_contig = 1;
  else {
      is_contig = 0;
      /* CHANGE THIS TO CHECK THE is_contig FIELD OF THE DATATYPE */
  }

  is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
  if (comm_ptr->is_hetero)
      is_homogeneous = 0;
#endif

  if (is_contig && is_homogeneous) {
      /* contiguous and homogeneous */
      MPID_Datatype_get_size_macro(datatype, type_size);
      nbytes = type_size * count;
  }
  else {
#ifdef UNIMPLEMENTED
      NMPI_Pack_size(count, datatype, comm, &nbytes);
#endif
  }

  relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

  /* Lock for collective operation */
  MPID_Comm_thread_lock( comm_ptr );

  if ((nbytes < MPIR_BCAST_SHORT_MSG) && (comm_size <= MPIR_BCAST_MIN_PROCS)) {

      /* Use short message algorithm, namely, minimum spanning tree */

      /* Algorithm:
         This uses a fairly basic recursive subdivision algorithm.
         The root sends to the process comm_size/2 away; the receiver becomes
         a root for a subtree and applies the same process. 

         So that the new root can easily identify the size of its
         subtree, the (subtree) roots are all powers of two (relative
         to the root) If m = the first power of 2 such that 2^m >= the
         size of the communicator, then the subtree at root at 2^(m-k)
         has size 2^k (with special handling for subtrees that aren't
         a power of two in size).
     
         Do subdivision.  There are two phases:
         1. Wait for arrival of data.  Because of the power of two nature
         of the subtree roots, the source of this message is alwyas the
         process whose relative rank has the least significant 1 bit CLEARED.
         That is, process 4 (100) receives from process 0, process 7 (111) 
         from process 6 (110), etc.   
         2. Forward to my subtree
         
         Note that the process that is the tree root is handled automatically
         by this code, since it has no bits set.  */

      mask = 0x1;
      while (mask < comm_size) {
          if (relative_rank & mask) {
              src = rank - mask; 
              if (src < 0) src += comm_size;
              mpi_errno = MPIC_Recv(buffer,count,datatype,src,
                                   MPIR_BCAST_TAG,comm,&status);
              if (mpi_errno) return mpi_errno;
              break;
          }
          mask <<= 1;
      }

      /* This process is responsible for all processes that have bits
         set from the LSB upto (but not including) mask.  Because of
         the "not including", we start by shifting mask back down one.
         
         We can easily change to a different algorithm at any power of two
         by changing the test (mask > 1) to (mask > block_size) 

         One such version would use non-blocking operations for the last 2-4
         steps (this also bounds the number of MPI_Requests that would
         be needed).  */

      mask >>= 1;
      while (mask > 0) {
          if (relative_rank + mask < comm_size) {
              dst = rank + mask;
              if (dst >= comm_size) dst -= comm_size;
              mpi_errno = MPIC_Send (buffer,count,datatype,dst,
                                     MPIR_BCAST_TAG,comm); 
              if (mpi_errno) return mpi_errno;
          }
          mask >>= 1;
      }
  }

  else { 
      /* use long message algorithm: MST scatter followed by an allgather */

      /* The scatter algorithm divides the buffer into nprocs pieces and
         scatters them among the processes. Root gets the first piece,
         root+1 gets the second piece, and so forth. Uses the same minimum
         spanning tree (MST) algorithm as above. Ceiling division
         is used to compute the size of each piece. This means some
         processes may not get any data. For example if bufsize = 97 and
         nprocs = 16, ranks 15 and 16 will get 0 data. On each process, the
         scattered data is stored at the same offset in the buffer as it is
         on the root process. */ 

      if (is_contig && is_homogeneous) {
          /* contiguous and homogeneous. no need to pack. */
          tmp_buf = buffer;
      }
      else {
          /* noncontiguous or heterogeneous. pack into temporary buffer. */
          tmp_buf = MPIU_Malloc(nbytes);
          if (!tmp_buf) {
              mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
              return mpi_errno;
          }

          if (rank == root) {
              position = 0;
#ifdef UNIMPLEMENTED
              NMPI_Pack(buffer, count, datatype, tmp_buf, nbytes,
                        &position, comm);
#endif
          }
      }

      scatter_size = (nbytes + comm_size - 1)/comm_size; /* ceiling division */
      curr_size = (rank == root) ? nbytes : 0; /* root starts with all the
                                                  data */

      mask = 0x1;
      while (mask < comm_size) {
          if (relative_rank & mask) {
              src = rank - mask; 
              if (src < 0) src += comm_size;
              recv_size = nbytes - relative_rank*scatter_size;
              /* recv_size is larger than what might actually be sent by the
                 sender. We don't need compute the exact value because MPI
                 allows you to post a larger recv.*/ 
              if (recv_size <= 0) 
                  curr_size = 0; /* this process doesn't receive any data
                                    because of uneven division */
              else {
                  mpi_errno = MPIC_Recv(((char *)tmp_buf +
                                         relative_rank*scatter_size),
                                        recv_size, MPI_BYTE, src,
                                        MPIR_BCAST_TAG, comm, &status);
                  if (mpi_errno) return mpi_errno;

                  /* query actual size of data received */
                  NMPI_Get_count(&status, MPI_BYTE, &curr_size);
              }
              break;
          }
          mask <<= 1;
      }

      /* This process is responsible for all processes that have bits
         set from the LSB upto (but not including) mask.  Because of
         the "not including", we start by shifting mask back down
         one. */

      mask >>= 1;
      while (mask > 0) {
          if (relative_rank + mask < comm_size) {
              
              send_size = curr_size - scatter_size * mask; 
              /* mask is also the size of this process's subtree */

              if (send_size > 0) {
                  dst = rank + mask;
                  if (dst >= comm_size) dst -= comm_size;
                  mpi_errno = MPIC_Send (((char *)tmp_buf +
                                         scatter_size*(relative_rank+mask)),
                                        send_size, MPI_BYTE, dst,
                                        MPIR_BCAST_TAG, comm);
                  if (mpi_errno) return mpi_errno;
                  curr_size -= send_size;
              }
          }
          mask >>= 1;
      }

      /* Scatter complete. Now do an allgather using recursive
         doubling. The basic recursive doubling algorithm works for
         power-of-two number of processes. We modify it for
         non-powers-of-two.  */ 

      mask = 0x1;
      i = 0;
      while (mask < comm_size) {
          relative_dst = relative_rank ^ mask;

          dst = (relative_dst + root) % comm_size; 

          /* find offset into send and recv buffers.
             zero out the least significant "i" bits of relative_rank and
             relative_dst to find root of src and dst
             subtrees. Use ranks of roots as index to send from
             and recv into  buffer */ 

          dst_tree_root = relative_dst >> i;
          dst_tree_root <<= i;
          
          my_tree_root = relative_rank >> i;
          my_tree_root <<= i;

          send_offset = my_tree_root * scatter_size;
          recv_offset = dst_tree_root * scatter_size;

          if (relative_dst < comm_size) {
              mpi_errno = MPIC_Sendrecv(((char *)tmp_buf + send_offset),
                            curr_size, MPI_BYTE, dst, MPIR_BCAST_TAG, 
                            ((char *)tmp_buf + recv_offset),
                            scatter_size*mask, MPI_BYTE, dst,
                            MPIR_BCAST_TAG, comm, &status);
              if (mpi_errno != MPI_SUCCESS) return mpi_errno;
              NMPI_Get_count(&status, MPI_BYTE, &recv_size);
              curr_size += recv_size;
          }

          /* if some processes in this process's subtree in this step
             did not have any destination process to communicate with
             because of non-power-of-two, we need to send them the
             data that they would normally have received from those
             processes. That is, the haves in this subtree must send to
             the havenots. We use a logarithmic recursive-halfing algorithm
             for this. */

          if (dst_tree_root + mask > comm_size) {
              nprocs_completed = comm_size - my_tree_root - mask;
              /* nprocs_completed is the number of processes in this
                 subtree that have all the data. Send data to others
                 in a tree fashion. First find root of current tree
                 that is being divided into two. k is the number of
                 least-significant bits in this process's rank that
                 must be zeroed out to find the rank of the root */ 
              j = mask;
              k = 0;
              while (j) {
                  j >>= 1;
                  k++;
              }
              k--;

              offset = scatter_size * (my_tree_root + mask);
              tmp_mask = mask >> 1;

              while (tmp_mask) {
                  relative_dst = relative_rank ^ tmp_mask;
                  dst = (relative_dst + root) % comm_size; 
                  
                  tree_root = relative_rank >> k;
                  tree_root <<= k;

                  /* send only if this proc has data and destination
                     doesn't have data. */

                  /* if (rank == 3) { 
                      printf("rank %d, dst %d, root %d, nprocs_completed %d\n", relative_rank, relative_dst, tree_root, nprocs_completed);
                      fflush(stdout);
                      }*/

                  if ((relative_dst > relative_rank) && 
                      (relative_rank < tree_root + nprocs_completed)
                      && (relative_dst >= tree_root + nprocs_completed)) {

                      /* printf("Rank %d, send to %d, offset %d, size %d\n", rank, dst, offset, recv_size);
                         fflush(stdout); */
                      mpi_errno = MPIC_Send(((char *)tmp_buf + offset),
                                            recv_size, MPI_BYTE, dst,
                                            MPIR_BCAST_TAG, comm); 
                      /* recv_size was set in the previous
                         receive. that's the amount of data to be
                         sent now. */
                      if (mpi_errno != MPI_SUCCESS) return mpi_errno;
                  }
                  /* recv only if this proc. doesn't have data and sender
                     has data */
                  else if ((relative_dst < relative_rank) && 
                           (relative_dst < tree_root + nprocs_completed) &&
                           (relative_rank >= tree_root + nprocs_completed)) {
                      /* printf("Rank %d waiting to recv from rank %d\n",
                         relative_rank, dst); */
                      mpi_errno = MPIC_Recv(((char *)tmp_buf + offset),
                                            scatter_size*nprocs_completed, 
                                            MPI_BYTE, dst, MPIR_BCAST_TAG,
                                            comm, &status); 
                      /* nprocs_completed is also equal to the no. of processes
                         whose data we don't have */
                      if (mpi_errno != MPI_SUCCESS) return mpi_errno;
                      NMPI_Get_count(&status, MPI_BYTE, &recv_size);
                      curr_size += recv_size;
                      /* printf("Rank %d, recv from %d, offset %d, size %d\n", rank, dst, offset, recv_size);
                         fflush(stdout);*/
                  }
                  tmp_mask >>= 1;
                  k--;
              }
          }

          mask <<= 1;
          i++;
      }

      if (!is_contig || !is_homogeneous) {
          if (rank != root) {
              position = 0;
#ifdef UNIMPLEMENTED
              NMPI_Unpack(tmp_buf, nbytes, &position, buffer, count,
                          datatype, comm);
#endif
          }
          MPIU_Free(tmp_buf);
      }
  }

  /* Unlock for collective operation */
  MPID_Comm_thread_unlock( comm_ptr );

  return mpi_errno;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Bcast

/*@
   MPI_Bcast - broadcast

   Input Arguments:
+  void *buffer
.  int count
.  MPI_Datatype datatype
.  int root
-  MPI_Comm comm

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Bcast( void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm )
{
    static const char FCNAME[] = "MPI_Bcast";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_BCAST);

    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_BCAST);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype *datatype_ptr = NULL;
	    
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_BCAST);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(count, datatype, mpi_errno);
	    MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);
	    
	    MPID_Datatype_get_ptr(datatype, datatype_ptr);
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_BCAST);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Bcast != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Bcast(buffer, count,
                                              datatype, root, comm_ptr);
    }
    else
    {
	mpi_errno = MPIR_Bcast( buffer, count, datatype, root, comm_ptr );
    }
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_BCAST);
	return MPI_SUCCESS;
    }
    else
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_BCAST);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }
    /* ... end of body of routine ... */
}


