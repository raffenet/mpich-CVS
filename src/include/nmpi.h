/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPICH_NMPI_H_INCLUDED
#define MPICH_NMPI_H_INCLUDED

#define NMPI_Abort PMPI_Abort
#define NMPI_Bcast PMPI_Bcast
#define NMPI_Get_count PMPI_Get_count
#define NMPI_Pack PMPI_Pack
#define NMPI_Pack_size PMPI_Pack_size
#define NMPI_Reduce PMPI_Reduce
#define NMPI_Reduce_scatter PMPI_Reduce_scatter
#define NMPI_Unpack PMPI_Unpack
#define NMPI_Wait PMPI_Wait
#define NMPI_Test PMPI_Test
#define NMPI_Allreduce PMPI_Allreduce
#define NMPI_Allgather PMPI_Allgather
#define NMPI_Comm_get_attr PMPI_Comm_get_attr
#define NMPI_Comm_set_attr PMPI_Comm_set_attr
#define NMPI_Comm_create_keyval PMPI_Comm_create_keyval 
#define NMPI_Comm_free_keyval PMPI_Comm_free_keyval 
#define NMPI_Comm_group PMPI_Comm_group
#define NMPI_Comm_remote_group PMPI_Comm_remote_group
#define NMPI_Group_compare PMPI_Group_compare
#define NMPI_Group_free PMPI_Group_free
#define NMPI_Comm_split PMPI_Comm_split
#define NMPI_Comm_size PMPI_Comm_size 
#define NMPI_Comm_rank PMPI_Comm_rank
#define NMPI_Alltoall PMPI_Alltoall
#define NMPI_Isend PMPI_Isend
#define NMPI_Irecv PMPI_Irecv
#define NMPI_Recv PMPI_Recv 
#define NMPI_Send PMPI_Send
#define NMPI_Waitall PMPI_Waitall
#define NMPI_Sendrecv PMPI_Sendrecv
#define NMPI_Type_extent PMPI_Type_extent
#define NMPI_Comm_free PMPI_Comm_free
#define NMPI_Comm_dup PMPI_Comm_dup
#define NMPI_Type_lb PMPI_Type_lb
#define NMPI_Type_indexed PMPI_Type_indexed 
#define NMPI_Type_commit PMPI_Type_commit
#define NMPI_Type_free PMPI_Type_free
#define NMPI_Cart_rank PMPI_Cart_rank
#define NMPI_Iprobe PMPI_Iprobe
#define NMPI_Barrier PMPI_Barrier
#define NMPI_Type_get_true_extent PMPI_Type_get_true_extent
#define NMPI_Group_translate_ranks PMPI_Group_translate_ranks
#define NMPI_Type_create_indexed_block PMPI_Type_create_indexed_block
#endif /* MPICH_NMPI_H_INCLUDED */
