/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPICH_NMPI_H_INCLUDED
#define MPICH_NMPI_H_INCLUDED

/* 
 * This file provides a flexible way to map MPI routines that are used
 * within the MPICH2 implementation to either the MPI or PMPI versions.
 * In normal use, it is appropriate to use PMPI, but in some cases, 
 * using the MPI routines instead is desired.
 */

/*
 * When adding names, make sure that you add them to both branches.
 * This allows the --enable-nmpi-as-mpi switch in configure to 
 * cause the internal routines to use the MPI versions; this is good for 
 * using tools that use the MPI profiling interface to collect data (e.g.,
 * for people who what to see what happens when the collective operations 
 * are implemented over MPI point-to-point.
 */

#ifdef USE_MPI_FOR_NMPI
#define NMPI_Abort MPI_Abort
#define NMPI_Bcast MPI_Bcast
#define NMPI_Get_count MPI_Get_count
#define NMPI_Pack MPI_Pack
#define NMPI_Pack_size MPI_Pack_size
#define NMPI_Reduce MPI_Reduce
#define NMPI_Reduce_scatter MPI_Reduce_scatter
#define NMPI_Unpack MPI_Unpack
#define NMPI_Wait MPI_Wait
#define NMPI_Test MPI_Test
#define NMPI_Allreduce MPI_Allreduce
#define NMPI_Allgather MPI_Allgather
#define NMPI_Comm_get_attr MPI_Comm_get_attr
#define NMPI_Comm_set_attr MPI_Comm_set_attr
#define NMPI_Comm_create_keyval MPI_Comm_create_keyval 
#define NMPI_Comm_free_keyval MPI_Comm_free_keyval 
#define NMPI_Comm_group MPI_Comm_group
#define NMPI_Comm_remote_group MPI_Comm_remote_group
#define NMPI_Group_compare MPI_Group_compare
#define NMPI_Group_free MPI_Group_free
#define NMPI_Comm_split MPI_Comm_split
#define NMPI_Comm_size MPI_Comm_size 
#define NMPI_Comm_rank MPI_Comm_rank
#define NMPI_Alltoall MPI_Alltoall
#define NMPI_Isend MPI_Isend
#define NMPI_Irecv MPI_Irecv
#define NMPI_Recv MPI_Recv 
#define NMPI_Send MPI_Send
#define NMPI_Waitall MPI_Waitall
#define NMPI_Sendrecv MPI_Sendrecv
#define NMPI_Type_extent MPI_Type_extent
#define NMPI_Comm_free MPI_Comm_free
#define NMPI_Comm_dup MPI_Comm_dup
#define NMPI_Type_lb MPI_Type_lb
#define NMPI_Type_indexed MPI_Type_indexed 
#define NMPI_Type_commit MPI_Type_commit
#define NMPI_Type_free MPI_Type_free
#define NMPI_Cart_rank MPI_Cart_rank
#define NMPI_Iprobe MPI_Iprobe
#define NMPI_Barrier MPI_Barrier
#define NMPI_Type_get_true_extent MPI_Type_get_true_extent
#define NMPI_Group_translate_ranks MPI_Group_translate_ranks
#define NMPI_Type_create_indexed_block MPI_Type_create_indexed_block
#define NMPI_Wtime MPI_Wtime
#else
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
#define NMPI_Wtime PMPI_Wtime
#endif
#endif /* MPICH_NMPI_H_INCLUDED */


