/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <winsock2.h>
#include <windows.h>
#include "mpichconf.h"
#include "mpichtimer.h"
#include <stdio.h>
#include <stdarg.h>
#define MPIU_UNREFERENCED_ARG(a) a

/*
 * Windows only mpi binding
 *
 * This file implements an mpi binding that calls another dynamically loaded mpi binding.
 * The environment variables MPI_DLL_NAME and MPICH2_CHANNEL control which library should be loaded.
 * The default library is mpich2.dll or mpich2d.dll.
 * A wrapper dll can also be named to replace only the MPI functions using the MPI_WRAP_DLL_NAME
 * environment variable.
 *
 * The motivation for this binding is to allow compiled mpi applications to be able
 * to use different implementations of mpich2 at run-time without re-linking the application.
 * This way mpiexec or the user can choose the best channel to use at run-time.
 *
 * For example, mpiexec may choose the shm channel for up to 8 processes on a single node
 * and sshm for more than 8 processes on a single node and the sock channel for multi-node
 * jobs.
 * Example 2: A user has an infiniband cluster and wants the ib channel to be the default.
 * So the user sets the mpiexec option to use the ib channel as the default and then all
 * jobs run on the cluster use the ib channel without modification or re-linking.
 *
 * A profiled or logged mpi implementation may be selected at run-time.  If the mpi dll is itself
 * profiled then the dll can be specified just as any other dll would be named.  Or a wrapper
 * dll can be named to work in conjunction with the mpi dll using the MPI_WRAP_DLL_NAME environment
 * variable.  The wrapper dll will implement the MPI interface and the mpi dll will implement the 
 * PMPI interface.
 *
 * The user can run a job and then decide to run the job again and produce a log file.  All that
 * needs to be done is specify the logged version of the mpich2 channel or a wrapper dll like mpe
 * and a log file will be produced.
 * Examples:
 * mpiexec -n 4 cpi
 * mpiexec -env MPICH2_CHANNEL ib -n 4 cpi
 * mpiexec -env MPI_DLL_NAME mpich2p.dll -n 4 cpi
 * mpiexec -env MPI_WRAP_DLL_NAME mpich2mped.dll -n 4 cpi
 * mpiexec -env MPICH2_CHANNEL ib -env MPI_WRAP_DLL_NAME mpich2mped.dll -n 4 cpi
 *
 */

#define MPI_ENV_DLL_NAME          "MPI_DLL_NAME"
#define MPI_ENV_CHANNEL_NAME      "MPICH2_CHANNEL"
#define MPI_ENV_MPIWRAP_DLL_NAME  "MPI_WRAP_DLL_NAME"
#ifdef _DEBUG
#define MPI_DEFAULT_DLL_NAME      "mpich2d.dll"
#define MPI_DEFAULT_WRAP_DLL_NAME "mpich2mped.dll"
#define DLL_FORMAT_STRING         "mpich2%sd.dll"
#else
#define MPI_DEFAULT_DLL_NAME      "mpich2.dll"
#define MPI_DEFAULT_WRAP_DLL_NAME "mpich2mpe.dll"
#define DLL_FORMAT_STRING         "mpich2%s.dll"
#endif
#define MAX_DLL_NAME              100

MPI_Fint *MPI_F_STATUS_IGNORE;
MPI_Fint *MPI_F_STATUSES_IGNORE;

static struct fn_table
{
    /* MPI */
    MPI_Comm (*MPI_Comm_f2c)(MPI_Fint);
    MPI_Datatype (*MPI_Type_f2c)(MPI_Fint);
    MPI_File (*MPI_File_f2c)(MPI_Fint);
    MPI_Fint (*MPI_Comm_c2f)(MPI_Comm);
    MPI_Fint (*MPI_File_c2f)(MPI_File);
    MPI_Fint (*MPI_Group_c2f)(MPI_Group);
    MPI_Fint (*MPI_Info_c2f)(MPI_Info);
    MPI_Fint (*MPI_Op_c2f)(MPI_Op);
    MPI_Fint (*MPI_Request_c2f)(MPI_Request);
    MPI_Fint (*MPI_Type_c2f)(MPI_Datatype);
    MPI_Fint (*MPI_Win_c2f)(MPI_Win);
    MPI_Group (*MPI_Group_f2c)(MPI_Fint);
    MPI_Info (*MPI_Info_f2c)(MPI_Fint);
    MPI_Op (*MPI_Op_f2c)(MPI_Fint);
    MPI_Request (*MPI_Request_f2c)(MPI_Fint);
    MPI_Win (*MPI_Win_f2c)(MPI_Fint);
    int (*MPI_File_open)(MPI_Comm, char *, int, MPI_Info, MPI_File *);
    int (*MPI_File_close)(MPI_File *);
    int (*MPI_File_delete)(char *, MPI_Info);
    int (*MPI_File_set_size)(MPI_File, MPI_Offset);
    int (*MPI_File_preallocate)(MPI_File, MPI_Offset);
    int (*MPI_File_get_size)(MPI_File, MPI_Offset *);
    int (*MPI_File_get_group)(MPI_File, MPI_Group *);
    int (*MPI_File_get_amode)(MPI_File, int *);
    int (*MPI_File_set_info)(MPI_File, MPI_Info);
    int (*MPI_File_get_info)(MPI_File, MPI_Info *);
    int (*MPI_File_set_view)(MPI_File, MPI_Offset, MPI_Datatype, MPI_Datatype, char *, MPI_Info);
    int (*MPI_File_get_view)(MPI_File, MPI_Offset *, MPI_Datatype *, MPI_Datatype *, char *);
    int (*MPI_File_read_at)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *);
    int (*MPI_File_read_at_all)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *);
    int (*MPI_File_write_at)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *);
    int (*MPI_File_write_at_all)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *);
    int (*MPI_File_iread_at)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPIO_Request *);
    int (*MPI_File_iwrite_at)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPIO_Request *);
    int (*MPI_File_read)(MPI_File, void *, int, MPI_Datatype, MPI_Status *); 
    int (*MPI_File_read_all)(MPI_File, void *, int, MPI_Datatype, MPI_Status *); 
    int (*MPI_File_write)(MPI_File, void *, int, MPI_Datatype, MPI_Status *);
    int (*MPI_File_write_all)(MPI_File, void *, int, MPI_Datatype, MPI_Status *);
    int (*MPI_File_iread)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *); 
    int (*MPI_File_iwrite)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *);
    int (*MPI_File_seek)(MPI_File, MPI_Offset, int);
    int (*MPI_File_get_position)(MPI_File, MPI_Offset *);
    int (*MPI_File_get_byte_offset)(MPI_File, MPI_Offset, MPI_Offset *);
    int (*MPI_File_read_shared)(MPI_File, void *, int, MPI_Datatype, MPI_Status *);
    int (*MPI_File_write_shared)(MPI_File, void *, int, MPI_Datatype, MPI_Status *);
    int (*MPI_File_iread_shared)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *);
    int (*MPI_File_iwrite_shared)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *);
    int (*MPI_File_read_ordered)(MPI_File, void *, int, MPI_Datatype, MPI_Status *);
    int (*MPI_File_write_ordered)(MPI_File, void *, int, MPI_Datatype, MPI_Status *);
    int (*MPI_File_seek_shared)(MPI_File, MPI_Offset, int);
    int (*MPI_File_get_position_shared)(MPI_File, MPI_Offset *);
    int (*MPI_File_read_at_all_begin)(MPI_File, MPI_Offset, void *, int, MPI_Datatype);
    int (*MPI_File_read_at_all_end)(MPI_File, void *, MPI_Status *);
    int (*MPI_File_write_at_all_begin)(MPI_File, MPI_Offset, void *, int, MPI_Datatype);
    int (*MPI_File_write_at_all_end)(MPI_File, void *, MPI_Status *);
    int (*MPI_File_read_all_begin)(MPI_File, void *, int, MPI_Datatype);
    int (*MPI_File_read_all_end)(MPI_File, void *, MPI_Status *);
    int (*MPI_File_write_all_begin)(MPI_File, void *, int, MPI_Datatype);
    int (*MPI_File_write_all_end)(MPI_File, void *, MPI_Status *);
    int (*MPI_File_read_ordered_begin)(MPI_File, void *, int, MPI_Datatype);
    int (*MPI_File_read_ordered_end)(MPI_File, void *, MPI_Status *);
    int (*MPI_File_write_ordered_begin)(MPI_File, void *, int, MPI_Datatype);
    int (*MPI_File_write_ordered_end)(MPI_File, void *, MPI_Status *);
    int (*MPI_File_get_type_extent)(MPI_File, MPI_Datatype, MPI_Aint *);
    int (*MPI_Register_datarep)(char *, MPI_Datarep_conversion_function *, MPI_Datarep_conversion_function *, MPI_Datarep_extent_function *, void *);
    int (*MPI_File_set_atomicity)(MPI_File, int);
    int (*MPI_File_get_atomicity)(MPI_File, int *);
    int (*MPI_File_sync)(MPI_File);
    int (*MPI_Send)(void*, int, MPI_Datatype, int, int, MPI_Comm);
    int (*MPI_Recv)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
    int (*MPI_Get_count)(MPI_Status *, MPI_Datatype, int *);
    int (*MPI_Bsend)(void*, int, MPI_Datatype, int, int, MPI_Comm);
    int (*MPI_Ssend)(void*, int, MPI_Datatype, int, int, MPI_Comm);
    int (*MPI_Rsend)(void*, int, MPI_Datatype, int, int, MPI_Comm);
    int (*MPI_Buffer_attach)( void*, int);
    int (*MPI_Buffer_detach)( void*, int *);
    int (*MPI_Isend)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
    int (*MPI_Ibsend)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
    int (*MPI_Issend)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
    int (*MPI_Irsend)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
    int (*MPI_Irecv)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
    int (*MPI_Wait)(MPI_Request *, MPI_Status *);
    int (*MPI_Test)(MPI_Request *, int *, MPI_Status *);
    int (*MPI_Request_free)(MPI_Request *);
    int (*MPI_Waitany)(int, MPI_Request *, int *, MPI_Status *);
    int (*MPI_Testany)(int, MPI_Request *, int *, int *, MPI_Status *);
    int (*MPI_Waitall)(int, MPI_Request *, MPI_Status *);
    int (*MPI_Testall)(int, MPI_Request *, int *, MPI_Status *);
    int (*MPI_Waitsome)(int, MPI_Request *, int *, int *, MPI_Status *);
    int (*MPI_Testsome)(int, MPI_Request *, int *, int *, MPI_Status *);
    int (*MPI_Iprobe)(int, int, MPI_Comm, int *, MPI_Status *);
    int (*MPI_Probe)(int, int, MPI_Comm, MPI_Status *);
    int (*MPI_Cancel)(MPI_Request *);
    int (*MPI_Test_cancelled)(MPI_Status *, int *);
    int (*MPI_Send_init)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
    int (*MPI_Bsend_init)(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *);
    int (*MPI_Ssend_init)(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *);
    int (*MPI_Rsend_init)(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *);
    int (*MPI_Recv_init)(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *);
    int (*MPI_Start)(MPI_Request *);
    int (*MPI_Startall)(int, MPI_Request *);
    int (*MPI_Sendrecv)(void *, int, MPI_Datatype,int, int, void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
    int (*MPI_Sendrecv_replace)(void*, int, MPI_Datatype, int, int, int, int, MPI_Comm, MPI_Status *);
    int (*MPI_Type_contiguous)(int, MPI_Datatype, MPI_Datatype *);
    int (*MPI_Type_vector)(int, int, int, MPI_Datatype, MPI_Datatype *);
    int (*MPI_Type_hvector)(int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *);
    int (*MPI_Type_indexed)(int, int *, int *, MPI_Datatype, MPI_Datatype *);
    int (*MPI_Type_hindexed)(int, int *, MPI_Aint *, MPI_Datatype, MPI_Datatype *);
    int (*MPI_Type_struct)(int, int *, MPI_Aint *, MPI_Datatype *, MPI_Datatype *);
    int (*MPI_Address)(void*, MPI_Aint *);
    int (*MPI_Type_extent)(MPI_Datatype, MPI_Aint *);
    int (*MPI_Type_size)(MPI_Datatype, int *);
    int (*MPI_Type_lb)(MPI_Datatype, MPI_Aint *);
    int (*MPI_Type_ub)(MPI_Datatype, MPI_Aint *);
    int (*MPI_Type_commit)(MPI_Datatype *);
    int (*MPI_Type_free)(MPI_Datatype *);
    int (*MPI_Get_elements)(MPI_Status *, MPI_Datatype, int *);
    int (*MPI_Pack)(void*, int, MPI_Datatype, void *, int, int *,  MPI_Comm);
    int (*MPI_Unpack)(void*, int, int *, void *, int, MPI_Datatype, MPI_Comm);
    int (*MPI_Pack_size)(int, MPI_Datatype, MPI_Comm, int *);
    int (*MPI_Barrier)(MPI_Comm );
    int (*MPI_Bcast)(void*, int, MPI_Datatype, int, MPI_Comm );
    int (*MPI_Gather)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm); 
    int (*MPI_Gatherv)(void* , int, MPI_Datatype, void*, int *, int *, MPI_Datatype, int, MPI_Comm); 
    int (*MPI_Scatter)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm);
    int (*MPI_Scatterv)(void* , int *, int *,  MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm);
    int (*MPI_Allgather)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
    int (*MPI_Allgatherv)(void* , int, MPI_Datatype, void*, int *, int *, MPI_Datatype, MPI_Comm);
    int (*MPI_Alltoall)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
    int (*MPI_Alltoallv)(void* , int *, int *, MPI_Datatype, void*, int *, int *, MPI_Datatype, MPI_Comm);
    int (*MPI_Reduce)(void* , void*, int, MPI_Datatype, MPI_Op, int, MPI_Comm);
    int (*MPI_Op_create)(MPI_User_function *, int, MPI_Op *);
    int (*MPI_Op_free)( MPI_Op *);
    int (*MPI_Allreduce)(void* , void*, int, MPI_Datatype, MPI_Op, MPI_Comm);
    int (*MPI_Reduce_scatter)(void* , void*, int *, MPI_Datatype, MPI_Op, MPI_Comm);
    int (*MPI_Scan)(void* , void*, int, MPI_Datatype, MPI_Op, MPI_Comm );
    int (*MPI_Group_size)(MPI_Group, int *);
    int (*MPI_Group_rank)(MPI_Group, int *);
    int (*MPI_Group_translate_ranks )(MPI_Group, int, int *, MPI_Group, int *);
    int (*MPI_Group_compare)(MPI_Group, MPI_Group, int *);
    int (*MPI_Comm_group)(MPI_Comm, MPI_Group *);
    int (*MPI_Group_union)(MPI_Group, MPI_Group, MPI_Group *);
    int (*MPI_Group_intersection)(MPI_Group, MPI_Group, MPI_Group *);
    int (*MPI_Group_difference)(MPI_Group, MPI_Group, MPI_Group *);
    int (*MPI_Group_incl)(MPI_Group, int, int *, MPI_Group *);
    int (*MPI_Group_excl)(MPI_Group, int, int *, MPI_Group *);
    int (*MPI_Group_range_incl)(MPI_Group, int, int [][3], MPI_Group *);
    int (*MPI_Group_range_excl)(MPI_Group, int, int [][3], MPI_Group *);
    int (*MPI_Group_free)(MPI_Group *);
    int (*MPI_Comm_size)(MPI_Comm, int *);
    int (*MPI_Comm_rank)(MPI_Comm, int *);
    int (*MPI_Comm_compare)(MPI_Comm, MPI_Comm, int *);
    int (*MPI_Comm_dup)(MPI_Comm, MPI_Comm *);
    int (*MPI_Comm_create)(MPI_Comm, MPI_Group, MPI_Comm *);
    int (*MPI_Comm_split)(MPI_Comm, int, int, MPI_Comm *);
    int (*MPI_Comm_free)(MPI_Comm *);
    int (*MPI_Comm_test_inter)(MPI_Comm, int *);
    int (*MPI_Comm_remote_size)(MPI_Comm, int *);
    int (*MPI_Comm_remote_group)(MPI_Comm, MPI_Group *);
    int (*MPI_Intercomm_create)(MPI_Comm, int, MPI_Comm, int, int, MPI_Comm * );
    int (*MPI_Intercomm_merge)(MPI_Comm, int, MPI_Comm *);
    int (*MPI_Keyval_create)(MPI_Copy_function *, MPI_Delete_function *, int *, void*);
    int (*MPI_Keyval_free)(int *);
    int (*MPI_Attr_put)(MPI_Comm, int, void*);
    int (*MPI_Attr_get)(MPI_Comm, int, void *, int *);
    int (*MPI_Attr_delete)(MPI_Comm, int);
    int (*MPI_Topo_test)(MPI_Comm, int *);
    int (*MPI_Cart_create)(MPI_Comm, int, int *, int *, int, MPI_Comm *);
    int (*MPI_Dims_create)(int, int, int *);
    int (*MPI_Graph_create)(MPI_Comm, int, int *, int *, int, MPI_Comm *);
    int (*MPI_Graphdims_get)(MPI_Comm, int *, int *);
    int (*MPI_Graph_get)(MPI_Comm, int, int, int *, int *);
    int (*MPI_Cartdim_get)(MPI_Comm, int *);
    int (*MPI_Cart_get)(MPI_Comm, int, int *, int *, int *);
    int (*MPI_Cart_rank)(MPI_Comm, int *, int *);
    int (*MPI_Cart_coords)(MPI_Comm, int, int, int *);
    int (*MPI_Graph_neighbors_count)(MPI_Comm, int, int *);
    int (*MPI_Graph_neighbors)(MPI_Comm, int, int, int *);
    int (*MPI_Cart_shift)(MPI_Comm, int, int, int *, int *);
    int (*MPI_Cart_sub)(MPI_Comm, int *, MPI_Comm *);
    int (*MPI_Cart_map)(MPI_Comm, int, int *, int *, int *);
    int (*MPI_Graph_map)(MPI_Comm, int, int *, int *, int *);
    int (*MPI_Get_processor_name)(char *, int *);
    int (*MPI_Get_version)(int *, int *);
    int (*MPI_Errhandler_create)(MPI_Handler_function *, MPI_Errhandler *);
    int (*MPI_Errhandler_set)(MPI_Comm, MPI_Errhandler);
    int (*MPI_Errhandler_get)(MPI_Comm, MPI_Errhandler *);
    int (*MPI_Errhandler_free)(MPI_Errhandler *);
    int (*MPI_Error_string)(int, char *, int *);
    int (*MPI_Error_class)(int, int *);
    double (*MPI_Wtime)(void);
    double (*MPI_Wtick)(void);
    int (*MPI_Init)(int *, char ***);
    int (*MPI_Finalize)(void);
    int (*MPI_Initialized)(int *);
    int (*MPI_Abort)(MPI_Comm, int);
    int (*MPI_Pcontrol)(const int, ...);
    int (*MPI_DUP_FN )( MPI_Comm, int, void *, void *, void *, int * );
    int (*MPI_Close_port)(char *);
    int (*MPI_Comm_accept)(char *, MPI_Info, int, MPI_Comm, MPI_Comm *);
    int (*MPI_Comm_connect)(char *, MPI_Info, int, MPI_Comm, MPI_Comm *);
    int (*MPI_Comm_disconnect)(MPI_Comm *);
    int (*MPI_Comm_get_parent)(MPI_Comm *);
    int (*MPI_Comm_join)(int, MPI_Comm *);
    int (*MPI_Comm_spawn)(char *, char *[], int, MPI_Info, int, MPI_Comm, MPI_Comm *, int []);
    int (*MPI_Comm_spawn_multiple)(int, char *[], char **[], int [], MPI_Info [], int, MPI_Comm, MPI_Comm *, int []); 
    int (*MPI_Lookup_name)(char *, MPI_Info, char *);
    int (*MPI_Open_port)(MPI_Info, char *);
    int (*MPI_Publish_name)(char *, MPI_Info, char *);
    int (*MPI_Unpublish_name)(char *, MPI_Info, char *);
    int (*MPI_Accumulate)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype,  MPI_Op, MPI_Win);
    int (*MPI_Get)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win);
    int (*MPI_Put)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win);
    int (*MPI_Win_complete)(MPI_Win);
    int (*MPI_Win_create)(void *, MPI_Aint, int, MPI_Info, MPI_Comm, MPI_Win *);
    int (*MPI_Win_fence)(int, MPI_Win);
    int (*MPI_Win_free)(MPI_Win *);
    int (*MPI_Win_get_group)(MPI_Win, MPI_Group *);
    int (*MPI_Win_lock)(int, int, int, MPI_Win);
    int (*MPI_Win_post)(MPI_Group, int, MPI_Win);
    int (*MPI_Win_start)(MPI_Group, int, MPI_Win);
    int (*MPI_Win_test)(MPI_Win, int *);
    int (*MPI_Win_unlock)(int, MPI_Win);
    int (*MPI_Win_wait)(MPI_Win);
    int (*MPI_Alltoallw)(void *, int [], int [], MPI_Datatype [], void *, int [], int [], MPI_Datatype [], MPI_Comm);
    int (*MPI_Exscan)(void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);
    int (*MPI_Add_error_class)(int *);
    int (*MPI_Add_error_code)(int, int *);
    int (*MPI_Add_error_string)(int, char *);
    int (*MPI_Comm_call_errhandler)(MPI_Comm, int);
    int (*MPI_Comm_create_keyval)(MPI_Comm_copy_attr_function *, MPI_Comm_delete_attr_function *, int *, void *);
    int (*MPI_Comm_delete_attr)(MPI_Comm, int);
    int (*MPI_Comm_free_keyval)(int *);
    int (*MPI_Comm_get_attr)(MPI_Comm, int, void *, int *);
    int (*MPI_Comm_get_name)(MPI_Comm, char *, int *);
    int (*MPI_Comm_set_attr)(MPI_Comm, int, void *);
    int (*MPI_Comm_set_name)(MPI_Comm, char *);
    int (*MPI_File_call_errhandler)(MPI_File, int);
    int (*MPI_Grequest_complete)(MPI_Request);
    int (*MPI_Grequest_start)(MPI_Grequest_query_function *, MPI_Grequest_free_function *, MPI_Grequest_cancel_function *, void *, MPI_Request *);
    int (*MPI_Init_thread)(int *, char ***, int, int *);
    int (*MPI_Is_thread_main)(int *);
    int (*MPI_Query_thread)(int *);
    int (*MPI_Status_set_cancelled)(MPI_Status *, int);
    int (*MPI_Status_set_elements)(MPI_Status *, MPI_Datatype, int);
    int (*MPI_Type_create_keyval)(MPI_Type_copy_attr_function *, MPI_Type_delete_attr_function *, int *, void *);
    int (*MPI_Type_delete_attr)(MPI_Datatype, int);
    int (*MPI_Type_dup)(MPI_Datatype, MPI_Datatype *);
    int (*MPI_Type_free_keyval)(int *);
    int (*MPI_Type_get_attr)(MPI_Datatype, int, void *, int *);
    int (*MPI_Type_get_contents)(MPI_Datatype, int, int, int, int [], MPI_Aint [], MPI_Datatype []);
    int (*MPI_Type_get_envelope)(MPI_Datatype, int *, int *, int *, int *);
    int (*MPI_Type_get_name)(MPI_Datatype, char *, int *);
    int (*MPI_Type_set_attr)(MPI_Datatype, int, void *);
    int (*MPI_Type_set_name)(MPI_Datatype, char *);
    int (*MPI_Type_match_size)( int, int, MPI_Datatype *);
    int (*MPI_Win_call_errhandler)(MPI_Win, int);
    int (*MPI_Win_create_keyval)(MPI_Win_copy_attr_function *, MPI_Win_delete_attr_function *, int *, void *);
    int (*MPI_Win_delete_attr)(MPI_Win, int);
    int (*MPI_Win_free_keyval)(int *);
    int (*MPI_Win_get_attr)(MPI_Win, int, void *, int *);
    int (*MPI_Win_get_name)(MPI_Win, char *, int *);
    int (*MPI_Win_set_attr)(MPI_Win, int, void *);
    int (*MPI_Win_set_name)(MPI_Win, char *);
    int (*MPI_Alloc_mem)(MPI_Aint, MPI_Info info, void *baseptr);
    int (*MPI_Comm_create_errhandler)(MPI_Comm_errhandler_fn *, MPI_Errhandler *);
    int (*MPI_Comm_get_errhandler)(MPI_Comm, MPI_Errhandler *);
    int (*MPI_Comm_set_errhandler)(MPI_Comm, MPI_Errhandler);
    int (*MPI_File_create_errhandler)(MPI_File_errhandler_fn *, MPI_Errhandler *);
    int (*MPI_File_get_errhandler)(MPI_File, MPI_Errhandler *);
    int (*MPI_File_set_errhandler)(MPI_File, MPI_Errhandler);
    int (*MPI_Finalized)(int *);
    int (*MPI_Free_mem)(void *);
    int (*MPI_Get_address)(void *, MPI_Aint *);
    int (*MPI_Info_create)(MPI_Info *);
    int (*MPI_Info_delete)(MPI_Info, char *);
    int (*MPI_Info_dup)(MPI_Info, MPI_Info *);
    int (*MPI_Info_free)(MPI_Info *info);
    int (*MPI_Info_get)(MPI_Info, char *, int, char *, int *);
    int (*MPI_Info_get_nkeys)(MPI_Info, int *);
    int (*MPI_Info_get_nthkey)(MPI_Info, int, char *);
    int (*MPI_Info_get_valuelen)(MPI_Info, char *, int *, int *);
    int (*MPI_Info_set)(MPI_Info, char *, char *);
    int (*MPI_Pack_external)(char *, void *, int, MPI_Datatype, void *, MPI_Aint, MPI_Aint *); 
    int (*MPI_Pack_external_size)(char *, int, MPI_Datatype, MPI_Aint *); 
    int (*MPI_Request_get_status)(MPI_Request, int *, MPI_Status *);
    int (*MPI_Status_c2f)(MPI_Status *, MPI_Fint *);
    int (*MPI_Status_f2c)(MPI_Fint *, MPI_Status *);
    int (*MPI_Type_create_darray)(int, int, int, int [], int [], int [], int [], int, MPI_Datatype, MPI_Datatype *);
    int (*MPI_Type_create_hindexed)(int, int [], MPI_Aint [], MPI_Datatype, MPI_Datatype *);
    int (*MPI_Type_create_hvector)(int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *);
    int (*MPI_Type_create_indexed_block)(int, int, int [], MPI_Datatype, MPI_Datatype *);
    int (*MPI_Type_create_resized)(MPI_Datatype, MPI_Aint, MPI_Aint, MPI_Datatype *);
    int (*MPI_Type_create_struct)(int, int [], MPI_Aint [], MPI_Datatype [], MPI_Datatype *);
    int (*MPI_Type_create_subarray)(int, int [], int [], int [], int, MPI_Datatype, MPI_Datatype *);
    int (*MPI_Type_get_extent)(MPI_Datatype, MPI_Aint *, MPI_Aint *);
    int (*MPI_Type_get_true_extent)(MPI_Datatype, MPI_Aint *, MPI_Aint *);
    int (*MPI_Unpack_external)(char *, void *, MPI_Aint, MPI_Aint *, void *, int, MPI_Datatype); 
    int (*MPI_Win_create_errhandler)(MPI_Win_errhandler_fn *, MPI_Errhandler *);
    int (*MPI_Win_get_errhandler)(MPI_Win, MPI_Errhandler *);
    int (*MPI_Win_set_errhandler)(MPI_Win, MPI_Errhandler);
    int (*MPI_Type_create_f90_integer)( int, MPI_Datatype * );
    int (*MPI_Type_create_f90_real)( int, int, MPI_Datatype * );
    int (*MPI_Type_create_f90_complex)( int, int, MPI_Datatype * );
    /* PMPI */
    MPI_Comm (*PMPI_Comm_f2c)(MPI_Fint);
    MPI_Datatype (*PMPI_Type_f2c)(MPI_Fint);
    MPI_File (*PMPI_File_f2c)(MPI_Fint);
    MPI_Fint (*PMPI_Comm_c2f)(MPI_Comm);
    MPI_Fint (*PMPI_File_c2f)(MPI_File);
    MPI_Fint (*PMPI_Group_c2f)(MPI_Group);
    MPI_Fint (*PMPI_Info_c2f)(MPI_Info);
    MPI_Fint (*PMPI_Op_c2f)(MPI_Op);
    MPI_Fint (*PMPI_Request_c2f)(MPI_Request);
    MPI_Fint (*PMPI_Type_c2f)(MPI_Datatype);
    MPI_Fint (*PMPI_Win_c2f)(MPI_Win);
    MPI_Group (*PMPI_Group_f2c)(MPI_Fint);
    MPI_Info (*PMPI_Info_f2c)(MPI_Fint);
    MPI_Op (*PMPI_Op_f2c)(MPI_Fint);
    MPI_Request (*PMPI_Request_f2c)(MPI_Fint);
    MPI_Win (*PMPI_Win_f2c)(MPI_Fint);
    int (*PMPI_File_open)(MPI_Comm, char *, int, MPI_Info, MPI_File *);
    int (*PMPI_File_close)(MPI_File *);
    int (*PMPI_File_delete)(char *, MPI_Info);
    int (*PMPI_File_set_size)(MPI_File, MPI_Offset);
    int (*PMPI_File_preallocate)(MPI_File, MPI_Offset);
    int (*PMPI_File_get_size)(MPI_File, MPI_Offset *);
    int (*PMPI_File_get_group)(MPI_File, MPI_Group *);
    int (*PMPI_File_get_amode)(MPI_File, int *);
    int (*PMPI_File_set_info)(MPI_File, MPI_Info);
    int (*PMPI_File_get_info)(MPI_File, MPI_Info *);
    int (*PMPI_File_set_view)(MPI_File, MPI_Offset, MPI_Datatype, MPI_Datatype, char *, MPI_Info);
    int (*PMPI_File_get_view)(MPI_File, MPI_Offset *, MPI_Datatype *, MPI_Datatype *, char *);
    int (*PMPI_File_read_at)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *);
    int (*PMPI_File_read_at_all)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *);
    int (*PMPI_File_write_at)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *);
    int (*PMPI_File_write_at_all)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *);
    int (*PMPI_File_iread_at)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPIO_Request *);
    int (*PMPI_File_iwrite_at)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPIO_Request *);
    int (*PMPI_File_read)(MPI_File, void *, int, MPI_Datatype, MPI_Status *); 
    int (*PMPI_File_read_all)(MPI_File, void *, int, MPI_Datatype, MPI_Status *); 
    int (*PMPI_File_write)(MPI_File, void *, int, MPI_Datatype, MPI_Status *);
    int (*PMPI_File_write_all)(MPI_File, void *, int, MPI_Datatype, MPI_Status *);
    int (*PMPI_File_iread)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *); 
    int (*PMPI_File_iwrite)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *);
    int (*PMPI_File_seek)(MPI_File, MPI_Offset, int);
    int (*PMPI_File_get_position)(MPI_File, MPI_Offset *);
    int (*PMPI_File_get_byte_offset)(MPI_File, MPI_Offset, MPI_Offset *);
    int (*PMPI_File_read_shared)(MPI_File, void *, int, MPI_Datatype, MPI_Status *);
    int (*PMPI_File_write_shared)(MPI_File, void *, int, MPI_Datatype, MPI_Status *);
    int (*PMPI_File_iread_shared)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *);
    int (*PMPI_File_iwrite_shared)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *);
    int (*PMPI_File_read_ordered)(MPI_File, void *, int, MPI_Datatype, MPI_Status *);
    int (*PMPI_File_write_ordered)(MPI_File, void *, int, MPI_Datatype, MPI_Status *);
    int (*PMPI_File_seek_shared)(MPI_File, MPI_Offset, int);
    int (*PMPI_File_get_position_shared)(MPI_File, MPI_Offset *);
    int (*PMPI_File_read_at_all_begin)(MPI_File, MPI_Offset, void *, int, MPI_Datatype);
    int (*PMPI_File_read_at_all_end)(MPI_File, void *, MPI_Status *);
    int (*PMPI_File_write_at_all_begin)(MPI_File, MPI_Offset, void *, int, MPI_Datatype);
    int (*PMPI_File_write_at_all_end)(MPI_File, void *, MPI_Status *);
    int (*PMPI_File_read_all_begin)(MPI_File, void *, int, MPI_Datatype);
    int (*PMPI_File_read_all_end)(MPI_File, void *, MPI_Status *);
    int (*PMPI_File_write_all_begin)(MPI_File, void *, int, MPI_Datatype);
    int (*PMPI_File_write_all_end)(MPI_File, void *, MPI_Status *);
    int (*PMPI_File_read_ordered_begin)(MPI_File, void *, int, MPI_Datatype);
    int (*PMPI_File_read_ordered_end)(MPI_File, void *, MPI_Status *);
    int (*PMPI_File_write_ordered_begin)(MPI_File, void *, int, MPI_Datatype);
    int (*PMPI_File_write_ordered_end)(MPI_File, void *, MPI_Status *);
    int (*PMPI_File_get_type_extent)(MPI_File, MPI_Datatype, MPI_Aint *);
    int (*PMPI_Register_datarep)(char *, MPI_Datarep_conversion_function *, MPI_Datarep_conversion_function *, MPI_Datarep_extent_function *, void *);
    int (*PMPI_File_set_atomicity)(MPI_File, int);
    int (*PMPI_File_get_atomicity)(MPI_File, int *);
    int (*PMPI_File_sync)(MPI_File);
    int (*PMPI_Send)(void*, int, MPI_Datatype, int, int, MPI_Comm);
    int (*PMPI_Recv)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
    int (*PMPI_Get_count)(MPI_Status *, MPI_Datatype, int *);
    int (*PMPI_Bsend)(void*, int, MPI_Datatype, int, int, MPI_Comm);
    int (*PMPI_Ssend)(void*, int, MPI_Datatype, int, int, MPI_Comm);
    int (*PMPI_Rsend)(void*, int, MPI_Datatype, int, int, MPI_Comm);
    int (*PMPI_Buffer_attach)( void* buffer, int);
    int (*PMPI_Buffer_detach)( void* buffer, int *);
    int (*PMPI_Isend)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
    int (*PMPI_Ibsend)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
    int (*PMPI_Issend)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
    int (*PMPI_Irsend)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
    int (*PMPI_Irecv)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
    int (*PMPI_Wait)(MPI_Request *, MPI_Status *);
    int (*PMPI_Test)(MPI_Request *, int *, MPI_Status *);
    int (*PMPI_Request_free)(MPI_Request *);
    int (*PMPI_Waitany)(int, MPI_Request *, int *, MPI_Status *);
    int (*PMPI_Testany)(int, MPI_Request *, int *, int *, MPI_Status *);
    int (*PMPI_Waitall)(int, MPI_Request *, MPI_Status *);
    int (*PMPI_Testall)(int, MPI_Request *, int *, MPI_Status *);
    int (*PMPI_Waitsome)(int, MPI_Request *, int *, int *, MPI_Status *);
    int (*PMPI_Testsome)(int, MPI_Request *, int *, int *, MPI_Status *);
    int (*PMPI_Iprobe)(int, int, MPI_Comm, int *, MPI_Status *);
    int (*PMPI_Probe)(int, int, MPI_Comm, MPI_Status *);
    int (*PMPI_Cancel)(MPI_Request *);
    int (*PMPI_Test_cancelled)(MPI_Status *, int *);
    int (*PMPI_Send_init)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
    int (*PMPI_Bsend_init)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
    int (*PMPI_Ssend_init)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
    int (*PMPI_Rsend_init)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
    int (*PMPI_Recv_init)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *);
    int (*PMPI_Start)(MPI_Request *);
    int (*PMPI_Startall)(int, MPI_Request *);
    int (*PMPI_Sendrecv)(void *, int, MPI_Datatype, int, int, void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *);
    int (*PMPI_Sendrecv_replace)(void*, int, MPI_Datatype, int, int, int, int, MPI_Comm, MPI_Status *);
    int (*PMPI_Type_contiguous)(int, MPI_Datatype, MPI_Datatype *);
    int (*PMPI_Type_vector)(int, int, int, MPI_Datatype, MPI_Datatype *);
    int (*PMPI_Type_hvector)(int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *);
    int (*PMPI_Type_indexed)(int, int *, int *, MPI_Datatype, MPI_Datatype *);
    int (*PMPI_Type_hindexed)(int, int *, MPI_Aint *, MPI_Datatype, MPI_Datatype *);
    int (*PMPI_Type_struct)(int, int *, MPI_Aint *, MPI_Datatype *, MPI_Datatype *);
    int (*PMPI_Address)(void*, MPI_Aint *);
    int (*PMPI_Type_extent)(MPI_Datatype, MPI_Aint *);
    int (*PMPI_Type_size)(MPI_Datatype, int *);
    int (*PMPI_Type_lb)(MPI_Datatype, MPI_Aint *);
    int (*PMPI_Type_ub)(MPI_Datatype, MPI_Aint *);
    int (*PMPI_Type_commit)(MPI_Datatype *);
    int (*PMPI_Type_free)(MPI_Datatype *);
    int (*PMPI_Get_elements)(MPI_Status *, MPI_Datatype, int *);
    int (*PMPI_Pack)(void*, int, MPI_Datatype, void *, int, int *,  MPI_Comm);
    int (*PMPI_Unpack)(void*, int, int *, void *, int, MPI_Datatype, MPI_Comm);
    int (*PMPI_Pack_size)(int, MPI_Datatype, MPI_Comm, int *);
    int (*PMPI_Barrier)(MPI_Comm );
    int (*PMPI_Bcast)(void* buffer, int, MPI_Datatype, int, MPI_Comm );
    int (*PMPI_Gather)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm); 
    int (*PMPI_Gatherv)(void* , int, MPI_Datatype, void*, int *, int *, MPI_Datatype, int, MPI_Comm); 
    int (*PMPI_Scatter)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm);
    int (*PMPI_Scatterv)(void* , int *, int *displs, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm);
    int (*PMPI_Allgather)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
    int (*PMPI_Allgatherv)(void* , int, MPI_Datatype, void*, int *, int *, MPI_Datatype, MPI_Comm);
    int (*PMPI_Alltoall)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm);
    int (*PMPI_Alltoallv)(void* , int *, int *, MPI_Datatype, void*, int *, int *, MPI_Datatype, MPI_Comm);
    int (*PMPI_Reduce)(void* , void*, int, MPI_Datatype, MPI_Op, int, MPI_Comm);
    int (*PMPI_Op_create)(MPI_User_function *, int, MPI_Op *);
    int (*PMPI_Op_free)( MPI_Op *);
    int (*PMPI_Allreduce)(void* , void*, int, MPI_Datatype, MPI_Op, MPI_Comm);
    int (*PMPI_Reduce_scatter)(void* , void*, int *, MPI_Datatype, MPI_Op, MPI_Comm);
    int (*PMPI_Scan)(void* , void*, int, MPI_Datatype, MPI_Op, MPI_Comm );
    int (*PMPI_Group_size)(MPI_Group, int *);
    int (*PMPI_Group_rank)(MPI_Group, int *);
    int (*PMPI_Group_translate_ranks )(MPI_Group, int, int *, MPI_Group, int *);
    int (*PMPI_Group_compare)(MPI_Group, MPI_Group, int *);
    int (*PMPI_Comm_group)(MPI_Comm, MPI_Group *);
    int (*PMPI_Group_union)(MPI_Group, MPI_Group, MPI_Group *);
    int (*PMPI_Group_intersection)(MPI_Group, MPI_Group, MPI_Group *);
    int (*PMPI_Group_difference)(MPI_Group, MPI_Group, MPI_Group *);
    int (*PMPI_Group_incl)(MPI_Group, int, int *, MPI_Group *);
    int (*PMPI_Group_excl)(MPI_Group, int, int *, MPI_Group *);
    int (*PMPI_Group_range_incl)(MPI_Group, int, int [][3], MPI_Group *);
    int (*PMPI_Group_range_excl)(MPI_Group, int, int [][3], MPI_Group *);
    int (*PMPI_Group_free)(MPI_Group *);
    int (*PMPI_Comm_size)(MPI_Comm, int *);
    int (*PMPI_Comm_rank)(MPI_Comm, int *);
    int (*PMPI_Comm_compare)(MPI_Comm, MPI_Comm, int *);
    int (*PMPI_Comm_dup)(MPI_Comm, MPI_Comm *);
    int (*PMPI_Comm_create)(MPI_Comm, MPI_Group, MPI_Comm *);
    int (*PMPI_Comm_split)(MPI_Comm, int, int, MPI_Comm *);
    int (*PMPI_Comm_free)(MPI_Comm *);
    int (*PMPI_Comm_test_inter)(MPI_Comm, int *);
    int (*PMPI_Comm_remote_size)(MPI_Comm, int *);
    int (*PMPI_Comm_remote_group)(MPI_Comm, MPI_Group *);
    int (*PMPI_Intercomm_create)(MPI_Comm, int, MPI_Comm, int, int, MPI_Comm *);
    int (*PMPI_Intercomm_merge)(MPI_Comm, int, MPI_Comm *);
    int (*PMPI_Keyval_create)(MPI_Copy_function *, MPI_Delete_function *, int *, void*);
    int (*PMPI_Keyval_free)(int *);
    int (*PMPI_Attr_put)(MPI_Comm, int, void*);
    int (*PMPI_Attr_get)(MPI_Comm, int, void *, int *);
    int (*PMPI_Attr_delete)(MPI_Comm, int);
    int (*PMPI_Topo_test)(MPI_Comm, int *);
    int (*PMPI_Cart_create)(MPI_Comm, int, int *, int *, int, MPI_Comm *);
    int (*PMPI_Dims_create)(int, int, int *);
    int (*PMPI_Graph_create)(MPI_Comm, int, int *, int *, int, MPI_Comm *);
    int (*PMPI_Graphdims_get)(MPI_Comm, int *, int *);
    int (*PMPI_Graph_get)(MPI_Comm, int, int, int *, int *);
    int (*PMPI_Cartdim_get)(MPI_Comm, int *);
    int (*PMPI_Cart_get)(MPI_Comm, int, int *, int *, int *);
    int (*PMPI_Cart_rank)(MPI_Comm, int *, int *);
    int (*PMPI_Cart_coords)(MPI_Comm, int, int, int *);
    int (*PMPI_Graph_neighbors_count)(MPI_Comm, int, int *);
    int (*PMPI_Graph_neighbors)(MPI_Comm, int, int, int *);
    int (*PMPI_Cart_shift)(MPI_Comm, int, int, int *, int *);
    int (*PMPI_Cart_sub)(MPI_Comm, int *, MPI_Comm *);
    int (*PMPI_Cart_map)(MPI_Comm, int, int *, int *, int *);
    int (*PMPI_Graph_map)(MPI_Comm, int, int *, int *, int *);
    int (*PMPI_Get_processor_name)(char *, int *);
    int (*PMPI_Get_version)(int *, int *);
    int (*PMPI_Errhandler_create)(MPI_Handler_function *, MPI_Errhandler *);
    int (*PMPI_Errhandler_set)(MPI_Comm, MPI_Errhandler);
    int (*PMPI_Errhandler_get)(MPI_Comm, MPI_Errhandler *);
    int (*PMPI_Errhandler_free)(MPI_Errhandler *);
    int (*PMPI_Error_string)(int, char *, int *);
    int (*PMPI_Error_class)(int, int *);
    int (*PMPI_Init)(int *, char ***);
    int (*PMPI_Finalize)(void);
    int (*PMPI_Initialized)(int *);
    int (*PMPI_Abort)(MPI_Comm, int);
    int (*PMPI_Pcontrol)(const int, ...);
    int (*PMPI_Close_port)(char *);
    int (*PMPI_Comm_accept)(char *, MPI_Info, int, MPI_Comm, MPI_Comm *);
    int (*PMPI_Comm_connect)(char *, MPI_Info, int, MPI_Comm, MPI_Comm *);
    int (*PMPI_Comm_disconnect)(MPI_Comm *);
    int (*PMPI_Comm_get_parent)(MPI_Comm *);
    int (*PMPI_Comm_join)(int, MPI_Comm *);
    int (*PMPI_Comm_spawn)(char *, char *[], int, MPI_Info, int, MPI_Comm, MPI_Comm *, int []);
    int (*PMPI_Comm_spawn_multiple)(int, char *[], char **[], int [], MPI_Info [], int, MPI_Comm, MPI_Comm *, int []); 
    int (*PMPI_Lookup_name)(char *, MPI_Info, char *);
    int (*PMPI_Open_port)(MPI_Info, char *);
    int (*PMPI_Publish_name)(char *, MPI_Info, char *);
    int (*PMPI_Unpublish_name)(char *, MPI_Info, char *);
    int (*PMPI_Accumulate)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype,  MPI_Op, MPI_Win);
    int (*PMPI_Get)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win);
    int (*PMPI_Put)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win);
    int (*PMPI_Win_complete)(MPI_Win);
    int (*PMPI_Win_create)(void *, MPI_Aint, int, MPI_Info, MPI_Comm, MPI_Win *);
    int (*PMPI_Win_fence)(int, MPI_Win);
    int (*PMPI_Win_free)(MPI_Win *);
    int (*PMPI_Win_get_group)(MPI_Win, MPI_Group *);
    int (*PMPI_Win_lock)(int, int, int, MPI_Win);
    int (*PMPI_Win_post)(MPI_Group, int, MPI_Win);
    int (*PMPI_Win_start)(MPI_Group, int, MPI_Win);
    int (*PMPI_Win_test)(MPI_Win, int *);
    int (*PMPI_Win_unlock)(int, MPI_Win);
    int (*PMPI_Win_wait)(MPI_Win);
    int (*PMPI_Alltoallw)(void *, int [], int [], MPI_Datatype [], void *, int [], int [], MPI_Datatype [], MPI_Comm);
    int (*PMPI_Exscan)(void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm) ;
    int (*PMPI_Add_error_class)(int *);
    int (*PMPI_Add_error_code)(int, int *);
    int (*PMPI_Add_error_string)(int, char *);
    int (*PMPI_Comm_call_errhandler)(MPI_Comm, int);
    int (*PMPI_Comm_create_keyval)(MPI_Comm_copy_attr_function *, MPI_Comm_delete_attr_function *, int *, void *);
    int (*PMPI_Comm_delete_attr)(MPI_Comm, int);
    int (*PMPI_Comm_free_keyval)(int *);
    int (*PMPI_Comm_get_attr)(MPI_Comm, int, void *, int *);
    int (*PMPI_Comm_get_name)(MPI_Comm, char *, int *);
    int (*PMPI_Comm_set_attr)(MPI_Comm, int, void *);
    int (*PMPI_Comm_set_name)(MPI_Comm, char *);
    int (*PMPI_File_call_errhandler)(MPI_File, int);
    int (*PMPI_Grequest_complete)(MPI_Request);
    int (*PMPI_Grequest_start)(MPI_Grequest_query_function *, MPI_Grequest_free_function *, MPI_Grequest_cancel_function *, void *, MPI_Request *);
    int (*PMPI_Init_thread)(int *, char ***, int, int *);
    int (*PMPI_Is_thread_main)(int *);
    int (*PMPI_Query_thread)(int *);
    int (*PMPI_Status_set_cancelled)(MPI_Status *, int);
    int (*PMPI_Status_set_elements)(MPI_Status *, MPI_Datatype, int);
    int (*PMPI_Type_create_keyval)(MPI_Type_copy_attr_function *, MPI_Type_delete_attr_function *, int *, void *);
    int (*PMPI_Type_delete_attr)(MPI_Datatype, int);
    int (*PMPI_Type_dup)(MPI_Datatype, MPI_Datatype *);
    int (*PMPI_Type_free_keyval)(int *);
    int (*PMPI_Type_get_attr)(MPI_Datatype, int, void *, int *);
    int (*PMPI_Type_get_contents)(MPI_Datatype, int, int, int, int [], MPI_Aint [], MPI_Datatype []);
    int (*PMPI_Type_get_envelope)(MPI_Datatype, int *, int *, int *, int *);
    int (*PMPI_Type_get_name)(MPI_Datatype, char *, int *);
    int (*PMPI_Type_set_attr)(MPI_Datatype, int, void *);
    int (*PMPI_Type_set_name)(MPI_Datatype, char *);
    int (*PMPI_Type_match_size)( int, int, MPI_Datatype *);
    int (*PMPI_Win_call_errhandler)(MPI_Win, int);
    int (*PMPI_Win_create_keyval)(MPI_Win_copy_attr_function *, MPI_Win_delete_attr_function *, int *, void *);
    int (*PMPI_Win_delete_attr)(MPI_Win, int);
    int (*PMPI_Win_free_keyval)(int *);
    int (*PMPI_Win_get_attr)(MPI_Win, int, void *, int *);
    int (*PMPI_Win_get_name)(MPI_Win, char *, int *);
    int (*PMPI_Win_set_attr)(MPI_Win, int, void *);
    int (*PMPI_Win_set_name)(MPI_Win, char *);
    int (*PMPI_Type_create_f90_integer)( int, MPI_Datatype * );
    int (*PMPI_Type_create_f90_real)( int, int, MPI_Datatype * );
    int (*PMPI_Type_create_f90_complex)( int, int, MPI_Datatype * );
    int (*PMPI_Alloc_mem)(MPI_Aint, MPI_Info info, void *baseptr);
    int (*PMPI_Comm_create_errhandler)(MPI_Comm_errhandler_fn *, MPI_Errhandler *);
    int (*PMPI_Comm_get_errhandler)(MPI_Comm, MPI_Errhandler *);
    int (*PMPI_Comm_set_errhandler)(MPI_Comm, MPI_Errhandler);
    int (*PMPI_File_create_errhandler)(MPI_File_errhandler_fn *, MPI_Errhandler *);
    int (*PMPI_File_get_errhandler)(MPI_File, MPI_Errhandler *);
    int (*PMPI_File_set_errhandler)(MPI_File, MPI_Errhandler);
    int (*PMPI_Finalized)(int *);
    int (*PMPI_Free_mem)(void *);
    int (*PMPI_Get_address)(void *, MPI_Aint *);
    int (*PMPI_Info_create)(MPI_Info *);
    int (*PMPI_Info_delete)(MPI_Info, char *);
    int (*PMPI_Info_dup)(MPI_Info, MPI_Info *);
    int (*PMPI_Info_free)(MPI_Info *info);
    int (*PMPI_Info_get)(MPI_Info, char *, int, char *, int *);
    int (*PMPI_Info_get_nkeys)(MPI_Info, int *);
    int (*PMPI_Info_get_nthkey)(MPI_Info, int, char *);
    int (*PMPI_Info_get_valuelen)(MPI_Info, char *, int *, int *);
    int (*PMPI_Info_set)(MPI_Info, char *, char *);
    int (*PMPI_Pack_external)(char *, void *, int, MPI_Datatype, void *, MPI_Aint, MPI_Aint *); 
    int (*PMPI_Pack_external_size)(char *, int, MPI_Datatype, MPI_Aint *); 
    int (*PMPI_Request_get_status)(MPI_Request, int *, MPI_Status *);
    int (*PMPI_Status_c2f)(MPI_Status *, MPI_Fint *);
    int (*PMPI_Status_f2c)(MPI_Fint *, MPI_Status *);
    int (*PMPI_Type_create_darray)(int, int, int, int [], int [], int [], int [], int, MPI_Datatype, MPI_Datatype *);
    int (*PMPI_Type_create_hindexed)(int, int [], MPI_Aint [], MPI_Datatype, MPI_Datatype *);
    int (*PMPI_Type_create_hvector)(int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *);
    int (*PMPI_Type_create_indexed_block)(int, int, int [], MPI_Datatype, MPI_Datatype *);
    int (*PMPI_Type_create_resized)(MPI_Datatype, MPI_Aint, MPI_Aint, MPI_Datatype *);
    int (*PMPI_Type_create_struct)(int, int [], MPI_Aint [], MPI_Datatype [], MPI_Datatype *);
    int (*PMPI_Type_create_subarray)(int, int [], int [], int [], int, MPI_Datatype, MPI_Datatype *);
    int (*PMPI_Type_get_extent)(MPI_Datatype, MPI_Aint *, MPI_Aint *);
    int (*PMPI_Type_get_true_extent)(MPI_Datatype, MPI_Aint *, MPI_Aint *);
    int (*PMPI_Unpack_external)(char *, void *, MPI_Aint, MPI_Aint *, void *, int, MPI_Datatype); 
    int (*PMPI_Win_create_errhandler)(MPI_Win_errhandler_fn *, MPI_Errhandler *);
    int (*PMPI_Win_get_errhandler)(MPI_Win, MPI_Errhandler *);
    int (*PMPI_Win_set_errhandler)(MPI_Win, MPI_Errhandler);
    double (*PMPI_Wtime)(void);
    double (*PMPI_Wtick)(void);

    /* Extra exported internal symbols */
    void (*MPIR_Keyval_set_fortran)(int);
    void (*MPIR_Keyval_set_fortran90)(int);
    void (*MPIR_Grequest_set_lang_f77)(MPI_Request);
    void (*MPIR_Keyval_set_cxx)(int, void (*)(void), void (*)(void));
    void (*MPIR_Errhandler_set_cxx)(MPI_Errhandler, void (*)(void));
    void (*MPIR_Op_set_cxx)(MPI_Op, void (*)(void));
    double (*MPID_Wtick)(void);
    void (*MPID_Wtime_todouble)(MPID_Time_t *, double *);
    /*int (*MPIR_Dup_fn)(MPI_Comm, int, void *, void *, void *, int *);*/
    int (*MPIR_Err_create_code)(int , int , const char [], int , int , const char [], const char [], ...);
    int (*MPIR_Err_return_comm)(struct MPID_Comm *, const char [], int);

    /* global variables */
    MPI_Fint **MPI_F_STATUS_IGNORE;
    MPI_Fint **MPI_F_STATUSES_IGNORE;
} fn;

static HMODULE hMPIModule = NULL;
static HMODULE hPMPIModule = NULL;
static BOOL LoadFunctions(const char *dll_name, const char *wrapper_dll_name)
{
    /* Load the PMPI module */
    hPMPIModule = LoadLibrary(dll_name);
    if (hPMPIModule == NULL)
    {
	return FALSE;
    }

    /* Load the MPI wrapper module or set the MPI module equal to the PMPI module */
    if (wrapper_dll_name != NULL)
    {
	hMPIModule = LoadLibrary(wrapper_dll_name);
	if (hMPIModule == NULL)
	{
	    return FALSE;
	}
    }
    else
    {
	hMPIModule = hPMPIModule;
    }
    
    /* MPI */
    fn.MPI_Comm_f2c = (MPI_Comm (*)(MPI_Fint))GetProcAddress(hMPIModule, "MPI_Comm_f2c");
    if (fn.MPI_Comm_f2c == NULL) fn.MPI_Comm_f2c = (MPI_Comm (*)(MPI_Fint))GetProcAddress(hPMPIModule, "MPI_Comm_f2c");
    fn.MPI_Type_f2c = (MPI_Datatype (*)(MPI_Fint))GetProcAddress(hMPIModule, "MPI_Type_f2c");
    if (fn.MPI_Type_f2c == NULL) fn.MPI_Type_f2c = (MPI_Datatype (*)(MPI_Fint))GetProcAddress(hPMPIModule, "MPI_Type_f2c");
    fn.MPI_File_f2c = (MPI_File (*)(MPI_Fint))GetProcAddress(hMPIModule, "MPI_File_f2c");
    if (fn.MPI_File_f2c == NULL) fn.MPI_File_f2c = (MPI_File (*)(MPI_Fint))GetProcAddress(hPMPIModule, "MPI_File_f2c");
    fn.MPI_Comm_c2f = (MPI_Fint (*)(MPI_Comm))GetProcAddress(hMPIModule, "MPI_Comm_c2f");
    if (fn.MPI_Comm_c2f == NULL) fn.MPI_Comm_c2f = (MPI_Fint (*)(MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Comm_c2f");
    fn.MPI_File_c2f = (MPI_Fint (*)(MPI_File))GetProcAddress(hMPIModule, "MPI_File_c2f");
    if (fn.MPI_File_c2f == NULL) fn.MPI_File_c2f = (MPI_Fint (*)(MPI_File))GetProcAddress(hPMPIModule, "MPI_File_c2f");
    fn.MPI_Group_c2f = (MPI_Fint (*)(MPI_Group))GetProcAddress(hMPIModule, "MPI_Group_c2f");
    if (fn.MPI_Group_c2f == NULL) fn.MPI_Group_c2f = (MPI_Fint (*)(MPI_Group))GetProcAddress(hPMPIModule, "MPI_Group_c2f");
    fn.MPI_Info_c2f = (MPI_Fint (*)(MPI_Info))GetProcAddress(hMPIModule, "MPI_Info_c2f");
    if (fn.MPI_Info_c2f == NULL) fn.MPI_Info_c2f = (MPI_Fint (*)(MPI_Info))GetProcAddress(hPMPIModule, "MPI_Info_c2f");
    fn.MPI_Op_c2f = (MPI_Fint (*)(MPI_Op))GetProcAddress(hMPIModule, "MPI_Op_c2f");
    if (fn.MPI_Op_c2f == NULL) fn.MPI_Op_c2f = (MPI_Fint (*)(MPI_Op))GetProcAddress(hPMPIModule, "MPI_Op_c2f");
    fn.MPI_Request_c2f = (MPI_Fint (*)(MPI_Request))GetProcAddress(hMPIModule, "MPI_Request_c2f");
    if (fn.MPI_Request_c2f == NULL) fn.MPI_Request_c2f = (MPI_Fint (*)(MPI_Request))GetProcAddress(hPMPIModule, "MPI_Request_c2f");
    fn.MPI_Type_c2f = (MPI_Fint (*)(MPI_Datatype))GetProcAddress(hMPIModule, "MPI_Type_c2f");
    if (fn.MPI_Type_c2f == NULL) fn.MPI_Type_c2f = (MPI_Fint (*)(MPI_Datatype))GetProcAddress(hPMPIModule, "MPI_Type_c2f");
    fn.MPI_Win_c2f = (MPI_Fint (*)(MPI_Win))GetProcAddress(hMPIModule, "MPI_Win_c2f");
    if (fn.MPI_Win_c2f == NULL) fn.MPI_Win_c2f = (MPI_Fint (*)(MPI_Win))GetProcAddress(hPMPIModule, "MPI_Win_c2f");
    fn.MPI_Group_f2c = (MPI_Group (*)(MPI_Fint))GetProcAddress(hMPIModule, "MPI_Group_f2c");
    if (fn.MPI_Group_f2c == NULL) fn.MPI_Group_f2c = (MPI_Group (*)(MPI_Fint))GetProcAddress(hPMPIModule, "MPI_Group_f2c");
    fn.MPI_Info_f2c = (MPI_Info (*)(MPI_Fint))GetProcAddress(hMPIModule, "MPI_Info_f2c");
    if (fn.MPI_Info_f2c == NULL) fn.MPI_Info_f2c = (MPI_Info (*)(MPI_Fint))GetProcAddress(hPMPIModule, "MPI_Info_f2c");
    fn.MPI_Op_f2c = (MPI_Op (*)(MPI_Fint))GetProcAddress(hMPIModule, "MPI_Op_f2c");
    if (fn.MPI_Op_f2c == NULL) fn.MPI_Op_f2c = (MPI_Op (*)(MPI_Fint))GetProcAddress(hPMPIModule, "MPI_Op_f2c");
    fn.MPI_Request_f2c = (MPI_Request (*)(MPI_Fint))GetProcAddress(hMPIModule, "MPI_Request_f2c");
    if (fn.MPI_Request_f2c == NULL) fn.MPI_Request_f2c = (MPI_Request (*)(MPI_Fint))GetProcAddress(hPMPIModule, "MPI_Request_f2c");
    fn.MPI_Win_f2c = (MPI_Win (*)(MPI_Fint))GetProcAddress(hMPIModule, "MPI_Win_f2c");
    if (fn.MPI_Win_f2c == NULL) fn.MPI_Win_f2c = (MPI_Win (*)(MPI_Fint))GetProcAddress(hPMPIModule, "MPI_Win_f2c");
    fn.MPI_File_open = (int (*)(MPI_Comm, char *, int, MPI_Info, MPI_File *))GetProcAddress(hMPIModule, "MPI_File_open");
    if (fn.MPI_File_open == NULL) fn.MPI_File_open = (int (*)(MPI_Comm, char *, int, MPI_Info, MPI_File *))GetProcAddress(hPMPIModule, "MPI_File_open");
    fn.MPI_File_close = (int (*)(MPI_File *))GetProcAddress(hMPIModule, "MPI_File_close");
    if (fn.MPI_File_close == NULL) fn.MPI_File_close = (int (*)(MPI_File *))GetProcAddress(hPMPIModule, "MPI_File_close");
    fn.MPI_File_delete = (int (*)(char *, MPI_Info))GetProcAddress(hMPIModule, "MPI_File_delete");
    if (fn.MPI_File_delete == NULL) fn.MPI_File_delete = (int (*)(char *, MPI_Info))GetProcAddress(hPMPIModule, "MPI_File_delete");
    fn.MPI_File_set_size = (int (*)(MPI_File, MPI_Offset))GetProcAddress(hMPIModule, "MPI_File_set_size");
    if (fn.MPI_File_set_size == NULL) fn.MPI_File_set_size = (int (*)(MPI_File, MPI_Offset))GetProcAddress(hPMPIModule, "MPI_File_set_size");
    fn.MPI_File_preallocate = (int (*)(MPI_File, MPI_Offset))GetProcAddress(hMPIModule, "MPI_File_preallocate");
    if (fn.MPI_File_preallocate == NULL) fn.MPI_File_preallocate = (int (*)(MPI_File, MPI_Offset))GetProcAddress(hPMPIModule, "MPI_File_preallocate");
    fn.MPI_File_get_size = (int (*)(MPI_File, MPI_Offset *))GetProcAddress(hMPIModule, "MPI_File_get_size");
    if (fn.MPI_File_get_size == NULL) fn.MPI_File_get_size = (int (*)(MPI_File, MPI_Offset *))GetProcAddress(hPMPIModule, "MPI_File_get_size");
    fn.MPI_File_get_group = (int (*)(MPI_File, MPI_Group *))GetProcAddress(hMPIModule, "MPI_File_get_group");
    if (fn.MPI_File_get_group == NULL) fn.MPI_File_get_group = (int (*)(MPI_File, MPI_Group *))GetProcAddress(hPMPIModule, "MPI_File_get_group");
    fn.MPI_File_get_amode = (int (*)(MPI_File, int *))GetProcAddress(hMPIModule, "MPI_File_get_amode");
    if (fn.MPI_File_get_amode == NULL) fn.MPI_File_get_amode = (int (*)(MPI_File, int *))GetProcAddress(hPMPIModule, "MPI_File_get_amode");
    fn.MPI_File_set_info = (int (*)(MPI_File, MPI_Info))GetProcAddress(hMPIModule, "MPI_File_set_info");
    if (fn.MPI_File_set_info == NULL) fn.MPI_File_set_info = (int (*)(MPI_File, MPI_Info))GetProcAddress(hPMPIModule, "MPI_File_set_info");
    fn.MPI_File_get_info = (int (*)(MPI_File, MPI_Info *))GetProcAddress(hMPIModule, "MPI_File_get_info");
    if (fn.MPI_File_get_info == NULL) fn.MPI_File_get_info = (int (*)(MPI_File, MPI_Info *))GetProcAddress(hPMPIModule, "MPI_File_get_info");
    fn.MPI_File_set_view = (int (*)(MPI_File, MPI_Offset, MPI_Datatype, MPI_Datatype, char *, MPI_Info))GetProcAddress(hMPIModule, "MPI_File_set_view");
    if (fn.MPI_File_set_view == NULL) fn.MPI_File_set_view = (int (*)(MPI_File, MPI_Offset, MPI_Datatype, MPI_Datatype, char *, MPI_Info))GetProcAddress(hPMPIModule, "MPI_File_set_view");
    fn.MPI_File_get_view = (int (*)(MPI_File, MPI_Offset *, MPI_Datatype *, MPI_Datatype *, char *))GetProcAddress(hMPIModule, "MPI_File_get_view");
    if (fn.MPI_File_get_view == NULL) fn.MPI_File_get_view = (int (*)(MPI_File, MPI_Offset *, MPI_Datatype *, MPI_Datatype *, char *))GetProcAddress(hPMPIModule, "MPI_File_get_view");
    fn.MPI_File_read_at = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_read_at");
    if (fn.MPI_File_read_at == NULL) fn.MPI_File_read_at = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_read_at");
    fn.MPI_File_read_at_all = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_read_at_all");
    if (fn.MPI_File_read_at_all == NULL) fn.MPI_File_read_at_all = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_read_at_all");
    fn.MPI_File_write_at = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_write_at");
    if (fn.MPI_File_write_at == NULL) fn.MPI_File_write_at = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_write_at");
    fn.MPI_File_write_at_all = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_write_at_all");
    if (fn.MPI_File_write_at_all == NULL) fn.MPI_File_write_at_all = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_write_at_all");
    fn.MPI_File_iread_at = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hMPIModule, "MPI_File_iread_at");
    if (fn.MPI_File_iread_at == NULL) fn.MPI_File_iread_at = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hPMPIModule, "MPI_File_iread_at");
    fn.MPI_File_iwrite_at = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hMPIModule, "MPI_File_iwrite_at");
    if (fn.MPI_File_iwrite_at == NULL) fn.MPI_File_iwrite_at = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hPMPIModule, "MPI_File_iwrite_at");
    fn.MPI_File_read = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_read"); 
    if (fn.MPI_File_read == NULL) fn.MPI_File_read = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_read"); 
    fn.MPI_File_read_all = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_read_all"); 
    if (fn.MPI_File_read_all == NULL) fn.MPI_File_read_all = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_read_all"); 
    fn.MPI_File_write = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_write");
    if (fn.MPI_File_write == NULL) fn.MPI_File_write = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_write");
    fn.MPI_File_write_all = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_write_all");
    if (fn.MPI_File_write_all == NULL) fn.MPI_File_write_all = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_write_all");
    fn.MPI_File_iread = (int (*)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hMPIModule, "MPI_File_iread"); 
    if (fn.MPI_File_iread == NULL) fn.MPI_File_iread = (int (*)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hPMPIModule, "MPI_File_iread"); 
    fn.MPI_File_iwrite = (int (*)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hMPIModule, "MPI_File_iwrite");
    if (fn.MPI_File_iwrite == NULL) fn.MPI_File_iwrite = (int (*)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hPMPIModule, "MPI_File_iwrite");
    fn.MPI_File_seek = (int (*)(MPI_File, MPI_Offset, int))GetProcAddress(hMPIModule, "MPI_File_seek");
    if (fn.MPI_File_seek == NULL) fn.MPI_File_seek = (int (*)(MPI_File, MPI_Offset, int))GetProcAddress(hPMPIModule, "MPI_File_seek");
    fn.MPI_File_get_position = (int (*)(MPI_File, MPI_Offset *))GetProcAddress(hMPIModule, "MPI_File_get_position");
    if (fn.MPI_File_get_position == NULL) fn.MPI_File_get_position = (int (*)(MPI_File, MPI_Offset *))GetProcAddress(hPMPIModule, "MPI_File_get_position");
    fn.MPI_File_get_byte_offset = (int (*)(MPI_File, MPI_Offset, MPI_Offset *))GetProcAddress(hMPIModule, "MPI_File_get_byte_offset");
    if (fn.MPI_File_get_byte_offset == NULL) fn.MPI_File_get_byte_offset = (int (*)(MPI_File, MPI_Offset, MPI_Offset *))GetProcAddress(hPMPIModule, "MPI_File_get_byte_offset");
    fn.MPI_File_read_shared = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_read_shared");
    if (fn.MPI_File_read_shared == NULL) fn.MPI_File_read_shared = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_read_shared");
    fn.MPI_File_write_shared = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_write_shared");
    if (fn.MPI_File_write_shared == NULL) fn.MPI_File_write_shared = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_write_shared");
    fn.MPI_File_iread_shared = (int (*)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hMPIModule, "MPI_File_iread_shared");
    if (fn.MPI_File_iread_shared == NULL) fn.MPI_File_iread_shared = (int (*)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hPMPIModule, "MPI_File_iread_shared");
    fn.MPI_File_iwrite_shared = (int (*)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hMPIModule, "MPI_File_iwrite_shared");
    if (fn.MPI_File_iwrite_shared == NULL) fn.MPI_File_iwrite_shared = (int (*)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hPMPIModule, "MPI_File_iwrite_shared");
    fn.MPI_File_read_ordered = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_read_ordered");
    if (fn.MPI_File_read_ordered == NULL) fn.MPI_File_read_ordered = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_read_ordered");
    fn.MPI_File_write_ordered = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_write_ordered");
    if (fn.MPI_File_write_ordered == NULL) fn.MPI_File_write_ordered = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_write_ordered");
    fn.MPI_File_seek_shared = (int (*)(MPI_File, MPI_Offset, int))GetProcAddress(hMPIModule, "MPI_File_seek_shared");
    if (fn.MPI_File_seek_shared == NULL) fn.MPI_File_seek_shared = (int (*)(MPI_File, MPI_Offset, int))GetProcAddress(hPMPIModule, "MPI_File_seek_shared");
    fn.MPI_File_get_position_shared = (int (*)(MPI_File, MPI_Offset *))GetProcAddress(hMPIModule, "MPI_File_get_position_shared");
    if (fn.MPI_File_get_position_shared == NULL) fn.MPI_File_get_position_shared = (int (*)(MPI_File, MPI_Offset *))GetProcAddress(hPMPIModule, "MPI_File_get_position_shared");
    fn.MPI_File_read_at_all_begin = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype))GetProcAddress(hMPIModule, "MPI_File_read_at_all_begin");
    if (fn.MPI_File_read_at_all_begin == NULL) fn.MPI_File_read_at_all_begin = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype))GetProcAddress(hPMPIModule, "MPI_File_read_at_all_begin");
    fn.MPI_File_read_at_all_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_read_at_all_end");
    if (fn.MPI_File_read_at_all_end == NULL) fn.MPI_File_read_at_all_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_read_at_all_end");
    fn.MPI_File_write_at_all_begin = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype))GetProcAddress(hMPIModule, "MPI_File_write_at_all_begin");
    if (fn.MPI_File_write_at_all_begin == NULL) fn.MPI_File_write_at_all_begin = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype))GetProcAddress(hPMPIModule, "MPI_File_write_at_all_begin");
    fn.MPI_File_write_at_all_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_write_at_all_end");
    if (fn.MPI_File_write_at_all_end == NULL) fn.MPI_File_write_at_all_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_write_at_all_end");
    fn.MPI_File_read_all_begin = (int (*)(MPI_File, void *, int, MPI_Datatype))GetProcAddress(hMPIModule, "MPI_File_read_all_begin");
    if (fn.MPI_File_read_all_begin == NULL) fn.MPI_File_read_all_begin = (int (*)(MPI_File, void *, int, MPI_Datatype))GetProcAddress(hPMPIModule, "MPI_File_read_all_begin");
    fn.MPI_File_read_all_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_read_all_end");
    if (fn.MPI_File_read_all_end == NULL) fn.MPI_File_read_all_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_read_all_end");
    fn.MPI_File_write_all_begin = (int (*)(MPI_File, void *, int, MPI_Datatype))GetProcAddress(hMPIModule, "MPI_File_write_all_begin");
    if (fn.MPI_File_write_all_begin == NULL) fn.MPI_File_write_all_begin = (int (*)(MPI_File, void *, int, MPI_Datatype))GetProcAddress(hPMPIModule, "MPI_File_write_all_begin");
    fn.MPI_File_write_all_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_write_all_end");
    if (fn.MPI_File_write_all_end == NULL) fn.MPI_File_write_all_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_write_all_end");
    fn.MPI_File_read_ordered_begin = (int (*)(MPI_File, void *, int, MPI_Datatype))GetProcAddress(hMPIModule, "MPI_File_read_ordered_begin");
    if (fn.MPI_File_read_ordered_begin == NULL) fn.MPI_File_read_ordered_begin = (int (*)(MPI_File, void *, int, MPI_Datatype))GetProcAddress(hPMPIModule, "MPI_File_read_ordered_begin");
    fn.MPI_File_read_ordered_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_read_ordered_end");
    if (fn.MPI_File_read_ordered_end == NULL) fn.MPI_File_read_ordered_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_read_ordered_end");
    fn.MPI_File_write_ordered_begin = (int (*)(MPI_File, void *, int, MPI_Datatype))GetProcAddress(hMPIModule, "MPI_File_write_ordered_begin");
    if (fn.MPI_File_write_ordered_begin == NULL) fn.MPI_File_write_ordered_begin = (int (*)(MPI_File, void *, int, MPI_Datatype))GetProcAddress(hPMPIModule, "MPI_File_write_ordered_begin");
    fn.MPI_File_write_ordered_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hMPIModule, "MPI_File_write_ordered_end");
    if (fn.MPI_File_write_ordered_end == NULL) fn.MPI_File_write_ordered_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_File_write_ordered_end");
    fn.MPI_File_get_type_extent = (int (*)(MPI_File, MPI_Datatype, MPI_Aint *))GetProcAddress(hMPIModule, "MPI_File_get_type_extent");
    if (fn.MPI_File_get_type_extent == NULL) fn.MPI_File_get_type_extent = (int (*)(MPI_File, MPI_Datatype, MPI_Aint *))GetProcAddress(hPMPIModule, "MPI_File_get_type_extent");
    fn.MPI_Register_datarep = (int (*)(char *, MPI_Datarep_conversion_function *, MPI_Datarep_conversion_function *, MPI_Datarep_extent_function *, void *))GetProcAddress(hMPIModule, "MPI_Register_datarep");
    if (fn.MPI_Register_datarep == NULL) fn.MPI_Register_datarep = (int (*)(char *, MPI_Datarep_conversion_function *, MPI_Datarep_conversion_function *, MPI_Datarep_extent_function *, void *))GetProcAddress(hPMPIModule, "MPI_Register_datarep");
    fn.MPI_File_set_atomicity = (int (*)(MPI_File, int))GetProcAddress(hMPIModule, "MPI_File_set_atomicity");
    if (fn.MPI_File_set_atomicity == NULL) fn.MPI_File_set_atomicity = (int (*)(MPI_File, int))GetProcAddress(hPMPIModule, "MPI_File_set_atomicity");
    fn.MPI_File_get_atomicity = (int (*)(MPI_File, int *))GetProcAddress(hMPIModule, "MPI_File_get_atomicity");
    if (fn.MPI_File_get_atomicity == NULL) fn.MPI_File_get_atomicity = (int (*)(MPI_File, int *))GetProcAddress(hPMPIModule, "MPI_File_get_atomicity");
    fn.MPI_File_sync = (int (*)(MPI_File))GetProcAddress(hMPIModule, "MPI_File_sync");
    if (fn.MPI_File_sync == NULL) fn.MPI_File_sync = (int (*)(MPI_File))GetProcAddress(hPMPIModule, "MPI_File_sync");
    fn.MPI_Send = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm))GetProcAddress(hMPIModule, "MPI_Send");
    if (fn.MPI_Send == NULL) fn.MPI_Send = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Send");
    fn.MPI_Recv = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *))GetProcAddress(hMPIModule, "MPI_Recv");
    if (fn.MPI_Recv == NULL) fn.MPI_Recv = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_Recv");
    fn.MPI_Get_count = (int (*)(MPI_Status *, MPI_Datatype, int *))GetProcAddress(hMPIModule, "MPI_Get_count");
    if (fn.MPI_Get_count == NULL) fn.MPI_Get_count = (int (*)(MPI_Status *, MPI_Datatype, int *))GetProcAddress(hPMPIModule, "MPI_Get_count");
    fn.MPI_Bsend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm))GetProcAddress(hMPIModule, "MPI_Bsend");
    if (fn.MPI_Bsend == NULL) fn.MPI_Bsend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Bsend");
    fn.MPI_Ssend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm))GetProcAddress(hMPIModule, "MPI_Ssend");
    if (fn.MPI_Ssend == NULL) fn.MPI_Ssend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Ssend");
    fn.MPI_Rsend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm))GetProcAddress(hMPIModule, "MPI_Rsend");
    if (fn.MPI_Rsend == NULL) fn.MPI_Rsend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Rsend");
    fn.MPI_Buffer_attach = (int (*)( void*, int))GetProcAddress(hMPIModule, "MPI_Buffer_attach");
    if (fn.MPI_Buffer_attach == NULL) fn.MPI_Buffer_attach = (int (*)( void*, int))GetProcAddress(hPMPIModule, "MPI_Buffer_attach");
    fn.MPI_Buffer_detach = (int (*)( void*, int *))GetProcAddress(hMPIModule, "MPI_Buffer_detach");
    if (fn.MPI_Buffer_detach == NULL) fn.MPI_Buffer_detach = (int (*)( void*, int *))GetProcAddress(hPMPIModule, "MPI_Buffer_detach");
    fn.MPI_Isend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hMPIModule, "MPI_Isend");
    if (fn.MPI_Isend == NULL) fn.MPI_Isend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "MPI_Isend");
    fn.MPI_Ibsend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hMPIModule, "MPI_Ibsend");
    if (fn.MPI_Ibsend == NULL) fn.MPI_Ibsend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "MPI_Ibsend");
    fn.MPI_Issend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hMPIModule, "MPI_Issend");
    if (fn.MPI_Issend == NULL) fn.MPI_Issend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "MPI_Issend");
    fn.MPI_Irsend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hMPIModule, "MPI_Irsend");
    if (fn.MPI_Irsend == NULL) fn.MPI_Irsend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "MPI_Irsend");
    fn.MPI_Irecv = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hMPIModule, "MPI_Irecv");
    if (fn.MPI_Irecv == NULL) fn.MPI_Irecv = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "MPI_Irecv");
    fn.MPI_Wait = (int (*)(MPI_Request *, MPI_Status *))GetProcAddress(hMPIModule, "MPI_Wait");
    if (fn.MPI_Wait == NULL) fn.MPI_Wait = (int (*)(MPI_Request *, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_Wait");
    fn.MPI_Test = (int (*)(MPI_Request *, int *, MPI_Status *))GetProcAddress(hMPIModule, "MPI_Test");
    if (fn.MPI_Test == NULL) fn.MPI_Test = (int (*)(MPI_Request *, int *, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_Test");
    fn.MPI_Request_free = (int (*)(MPI_Request *))GetProcAddress(hMPIModule, "MPI_Request_free");
    if (fn.MPI_Request_free == NULL) fn.MPI_Request_free = (int (*)(MPI_Request *))GetProcAddress(hPMPIModule, "MPI_Request_free");
    fn.MPI_Waitany = (int (*)(int, MPI_Request *, int *, MPI_Status *))GetProcAddress(hMPIModule, "MPI_Waitany");
    if (fn.MPI_Waitany == NULL) fn.MPI_Waitany = (int (*)(int, MPI_Request *, int *, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_Waitany");
    fn.MPI_Testany = (int (*)(int, MPI_Request *, int *, int *, MPI_Status *))GetProcAddress(hMPIModule, "MPI_Testany");
    if (fn.MPI_Testany == NULL) fn.MPI_Testany = (int (*)(int, MPI_Request *, int *, int *, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_Testany");
    fn.MPI_Waitall = (int (*)(int, MPI_Request *, MPI_Status *))GetProcAddress(hMPIModule, "MPI_Waitall");
    if (fn.MPI_Waitall == NULL) fn.MPI_Waitall = (int (*)(int, MPI_Request *, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_Waitall");
    fn.MPI_Testall = (int (*)(int, MPI_Request *, int *, MPI_Status *))GetProcAddress(hMPIModule, "MPI_Testall");
    if (fn.MPI_Testall == NULL) fn.MPI_Testall = (int (*)(int, MPI_Request *, int *, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_Testall");
    fn.MPI_Waitsome = (int (*)(int, MPI_Request *, int *, int *, MPI_Status *))GetProcAddress(hMPIModule, "MPI_Waitsome");
    if (fn.MPI_Waitsome == NULL) fn.MPI_Waitsome = (int (*)(int, MPI_Request *, int *, int *, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_Waitsome");
    fn.MPI_Testsome = (int (*)(int, MPI_Request *, int *, int *, MPI_Status *))GetProcAddress(hMPIModule, "MPI_Testsome");
    if (fn.MPI_Testsome == NULL) fn.MPI_Testsome = (int (*)(int, MPI_Request *, int *, int *, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_Testsome");
    fn.MPI_Iprobe = (int (*)(int, int, MPI_Comm, int *, MPI_Status *))GetProcAddress(hMPIModule, "MPI_Iprobe");
    if (fn.MPI_Iprobe == NULL) fn.MPI_Iprobe = (int (*)(int, int, MPI_Comm, int *, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_Iprobe");
    fn.MPI_Probe = (int (*)(int, int, MPI_Comm, MPI_Status *))GetProcAddress(hMPIModule, "MPI_Probe");
    if (fn.MPI_Probe == NULL) fn.MPI_Probe = (int (*)(int, int, MPI_Comm, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_Probe");
    fn.MPI_Cancel = (int (*)(MPI_Request *))GetProcAddress(hMPIModule, "MPI_Cancel");
    if (fn.MPI_Cancel == NULL) fn.MPI_Cancel = (int (*)(MPI_Request *))GetProcAddress(hPMPIModule, "MPI_Cancel");
    fn.MPI_Test_cancelled = (int (*)(MPI_Status *, int *))GetProcAddress(hMPIModule, "MPI_Test_cancelled");
    if (fn.MPI_Test_cancelled == NULL) fn.MPI_Test_cancelled = (int (*)(MPI_Status *, int *))GetProcAddress(hPMPIModule, "MPI_Test_cancelled");
    fn.MPI_Send_init = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hMPIModule, "MPI_Send_init");
    if (fn.MPI_Send_init == NULL) fn.MPI_Send_init = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "MPI_Send_init");
    fn.MPI_Bsend_init = (int (*)(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *))GetProcAddress(hMPIModule, "MPI_Bsend_init");
    if (fn.MPI_Bsend_init == NULL) fn.MPI_Bsend_init = (int (*)(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "MPI_Bsend_init");
    fn.MPI_Ssend_init = (int (*)(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *))GetProcAddress(hMPIModule, "MPI_Ssend_init");
    if (fn.MPI_Ssend_init == NULL) fn.MPI_Ssend_init = (int (*)(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "MPI_Ssend_init");
    fn.MPI_Rsend_init = (int (*)(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *))GetProcAddress(hMPIModule, "MPI_Rsend_init");
    if (fn.MPI_Rsend_init == NULL) fn.MPI_Rsend_init = (int (*)(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "MPI_Rsend_init");
    fn.MPI_Recv_init = (int (*)(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *))GetProcAddress(hMPIModule, "MPI_Recv_init");
    if (fn.MPI_Recv_init == NULL) fn.MPI_Recv_init = (int (*)(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "MPI_Recv_init");
    fn.MPI_Start = (int (*)(MPI_Request *))GetProcAddress(hMPIModule, "MPI_Start");
    if (fn.MPI_Start == NULL) fn.MPI_Start = (int (*)(MPI_Request *))GetProcAddress(hPMPIModule, "MPI_Start");
    fn.MPI_Startall = (int (*)(int, MPI_Request *))GetProcAddress(hMPIModule, "MPI_Startall");
    if (fn.MPI_Startall == NULL) fn.MPI_Startall = (int (*)(int, MPI_Request *))GetProcAddress(hPMPIModule, "MPI_Startall");
    fn.MPI_Sendrecv = (int (*)(void *, int, MPI_Datatype,int, int, void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *))GetProcAddress(hMPIModule, "MPI_Sendrecv");
    if (fn.MPI_Sendrecv == NULL) fn.MPI_Sendrecv = (int (*)(void *, int, MPI_Datatype,int, int, void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_Sendrecv");
    fn.MPI_Sendrecv_replace = (int (*)(void*, int, MPI_Datatype, int, int, int, int, MPI_Comm, MPI_Status *))GetProcAddress(hMPIModule, "MPI_Sendrecv_replace");
    if (fn.MPI_Sendrecv_replace == NULL) fn.MPI_Sendrecv_replace = (int (*)(void*, int, MPI_Datatype, int, int, int, int, MPI_Comm, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_Sendrecv_replace");
    fn.MPI_Type_contiguous = (int (*)(int, MPI_Datatype, MPI_Datatype *))GetProcAddress(hMPIModule, "MPI_Type_contiguous");
    if (fn.MPI_Type_contiguous == NULL) fn.MPI_Type_contiguous = (int (*)(int, MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "MPI_Type_contiguous");
    fn.MPI_Type_vector = (int (*)(int, int, int, MPI_Datatype, MPI_Datatype *))GetProcAddress(hMPIModule, "MPI_Type_vector");
    if (fn.MPI_Type_vector == NULL) fn.MPI_Type_vector = (int (*)(int, int, int, MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "MPI_Type_vector");
    fn.MPI_Type_hvector = (int (*)(int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *))GetProcAddress(hMPIModule, "MPI_Type_hvector");
    if (fn.MPI_Type_hvector == NULL) fn.MPI_Type_hvector = (int (*)(int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "MPI_Type_hvector");
    fn.MPI_Type_indexed = (int (*)(int, int *, int *, MPI_Datatype, MPI_Datatype *))GetProcAddress(hMPIModule, "MPI_Type_indexed");
    if (fn.MPI_Type_indexed == NULL) fn.MPI_Type_indexed = (int (*)(int, int *, int *, MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "MPI_Type_indexed");
    fn.MPI_Type_hindexed = (int (*)(int, int *, MPI_Aint *, MPI_Datatype, MPI_Datatype *))GetProcAddress(hMPIModule, "MPI_Type_hindexed");
    if (fn.MPI_Type_hindexed == NULL) fn.MPI_Type_hindexed = (int (*)(int, int *, MPI_Aint *, MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "MPI_Type_hindexed");
    fn.MPI_Type_struct = (int (*)(int, int *, MPI_Aint *, MPI_Datatype *, MPI_Datatype *))GetProcAddress(hMPIModule, "MPI_Type_struct");
    if (fn.MPI_Type_struct == NULL) fn.MPI_Type_struct = (int (*)(int, int *, MPI_Aint *, MPI_Datatype *, MPI_Datatype *))GetProcAddress(hPMPIModule, "MPI_Type_struct");
    fn.MPI_Address = (int (*)(void*, MPI_Aint *))GetProcAddress(hMPIModule, "MPI_Address");
    if (fn.MPI_Address == NULL) fn.MPI_Address = (int (*)(void*, MPI_Aint *))GetProcAddress(hPMPIModule, "MPI_Address");
    fn.MPI_Type_extent = (int (*)(MPI_Datatype, MPI_Aint *))GetProcAddress(hMPIModule, "MPI_Type_extent");
    if (fn.MPI_Type_extent == NULL) fn.MPI_Type_extent = (int (*)(MPI_Datatype, MPI_Aint *))GetProcAddress(hPMPIModule, "MPI_Type_extent");
    fn.MPI_Type_size = (int (*)(MPI_Datatype, int *))GetProcAddress(hMPIModule, "MPI_Type_size");
    if (fn.MPI_Type_size == NULL) fn.MPI_Type_size = (int (*)(MPI_Datatype, int *))GetProcAddress(hPMPIModule, "MPI_Type_size");
    fn.MPI_Type_lb = (int (*)(MPI_Datatype, MPI_Aint *))GetProcAddress(hMPIModule, "MPI_Type_lb");
    if (fn.MPI_Type_lb == NULL) fn.MPI_Type_lb = (int (*)(MPI_Datatype, MPI_Aint *))GetProcAddress(hPMPIModule, "MPI_Type_lb");
    fn.MPI_Type_ub = (int (*)(MPI_Datatype, MPI_Aint *))GetProcAddress(hMPIModule, "MPI_Type_ub");
    if (fn.MPI_Type_ub == NULL) fn.MPI_Type_ub = (int (*)(MPI_Datatype, MPI_Aint *))GetProcAddress(hPMPIModule, "MPI_Type_ub");
    fn.MPI_Type_commit = (int (*)(MPI_Datatype *))GetProcAddress(hMPIModule, "MPI_Type_commit");
    if (fn.MPI_Type_commit == NULL) fn.MPI_Type_commit = (int (*)(MPI_Datatype *))GetProcAddress(hPMPIModule, "MPI_Type_commit");
    fn.MPI_Type_free = (int (*)(MPI_Datatype *))GetProcAddress(hMPIModule, "MPI_Type_free");
    if (fn.MPI_Type_free == NULL) fn.MPI_Type_free = (int (*)(MPI_Datatype *))GetProcAddress(hPMPIModule, "MPI_Type_free");
    fn.MPI_Get_elements = (int (*)(MPI_Status *, MPI_Datatype, int *))GetProcAddress(hMPIModule, "MPI_Get_elements");
    if (fn.MPI_Get_elements == NULL) fn.MPI_Get_elements = (int (*)(MPI_Status *, MPI_Datatype, int *))GetProcAddress(hPMPIModule, "MPI_Get_elements");
    fn.MPI_Pack = (int (*)(void*, int, MPI_Datatype, void *, int, int *,  MPI_Comm))GetProcAddress(hMPIModule, "MPI_Pack");
    if (fn.MPI_Pack == NULL) fn.MPI_Pack = (int (*)(void*, int, MPI_Datatype, void *, int, int *,  MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Pack");
    fn.MPI_Unpack = (int (*)(void*, int, int *, void *, int, MPI_Datatype, MPI_Comm))GetProcAddress(hMPIModule, "MPI_Unpack");
    if (fn.MPI_Unpack == NULL) fn.MPI_Unpack = (int (*)(void*, int, int *, void *, int, MPI_Datatype, MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Unpack");
    fn.MPI_Pack_size = (int (*)(int, MPI_Datatype, MPI_Comm, int *))GetProcAddress(hMPIModule, "MPI_Pack_size");
    if (fn.MPI_Pack_size == NULL) fn.MPI_Pack_size = (int (*)(int, MPI_Datatype, MPI_Comm, int *))GetProcAddress(hPMPIModule, "MPI_Pack_size");
    fn.MPI_Barrier = (int (*)(MPI_Comm ))GetProcAddress(hMPIModule, "MPI_Barrier");
    if (fn.MPI_Barrier == NULL) fn.MPI_Barrier = (int (*)(MPI_Comm ))GetProcAddress(hPMPIModule, "MPI_Barrier");
    fn.MPI_Bcast = (int (*)(void*, int, MPI_Datatype, int, MPI_Comm ))GetProcAddress(hMPIModule, "MPI_Bcast");
    if (fn.MPI_Bcast == NULL) fn.MPI_Bcast = (int (*)(void*, int, MPI_Datatype, int, MPI_Comm ))GetProcAddress(hPMPIModule, "MPI_Bcast");
    fn.MPI_Gather = (int (*)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm))GetProcAddress(hMPIModule, "MPI_Gather"); 
    if (fn.MPI_Gather == NULL) fn.MPI_Gather = (int (*)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Gather"); 
    fn.MPI_Gatherv = (int (*)(void* , int, MPI_Datatype, void*, int *, int *, MPI_Datatype, int, MPI_Comm))GetProcAddress(hMPIModule, "MPI_Gatherv"); 
    if (fn.MPI_Gatherv == NULL) fn.MPI_Gatherv = (int (*)(void* , int, MPI_Datatype, void*, int *, int *, MPI_Datatype, int, MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Gatherv"); 
    fn.MPI_Scatter = (int (*)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm))GetProcAddress(hMPIModule, "MPI_Scatter");
    if (fn.MPI_Scatter == NULL) fn.MPI_Scatter = (int (*)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Scatter");
    fn.MPI_Scatterv = (int (*)(void* , int *, int *,  MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm))GetProcAddress(hMPIModule, "MPI_Scatterv");
    if (fn.MPI_Scatterv == NULL) fn.MPI_Scatterv = (int (*)(void* , int *, int *,  MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Scatterv");
    fn.MPI_Allgather = (int (*)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm))GetProcAddress(hMPIModule, "MPI_Allgather");
    if (fn.MPI_Allgather == NULL) fn.MPI_Allgather = (int (*)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Allgather");
    fn.MPI_Allgatherv = (int (*)(void* , int, MPI_Datatype, void*, int *, int *, MPI_Datatype, MPI_Comm))GetProcAddress(hMPIModule, "MPI_Allgatherv");
    if (fn.MPI_Allgatherv == NULL) fn.MPI_Allgatherv = (int (*)(void* , int, MPI_Datatype, void*, int *, int *, MPI_Datatype, MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Allgatherv");
    fn.MPI_Alltoall = (int (*)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm))GetProcAddress(hMPIModule, "MPI_Alltoall");
    if (fn.MPI_Alltoall == NULL) fn.MPI_Alltoall = (int (*)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Alltoall");
    fn.MPI_Alltoallv = (int (*)(void* , int *, int *, MPI_Datatype, void*, int *, int *, MPI_Datatype, MPI_Comm))GetProcAddress(hMPIModule, "MPI_Alltoallv");
    if (fn.MPI_Alltoallv == NULL) fn.MPI_Alltoallv = (int (*)(void* , int *, int *, MPI_Datatype, void*, int *, int *, MPI_Datatype, MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Alltoallv");
    fn.MPI_Reduce = (int (*)(void* , void*, int, MPI_Datatype, MPI_Op, int, MPI_Comm))GetProcAddress(hMPIModule, "MPI_Reduce");
    if (fn.MPI_Reduce == NULL) fn.MPI_Reduce = (int (*)(void* , void*, int, MPI_Datatype, MPI_Op, int, MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Reduce");
    fn.MPI_Op_create = (int (*)(MPI_User_function *, int, MPI_Op *))GetProcAddress(hMPIModule, "MPI_Op_create");
    if (fn.MPI_Op_create == NULL) fn.MPI_Op_create = (int (*)(MPI_User_function *, int, MPI_Op *))GetProcAddress(hPMPIModule, "MPI_Op_create");
    fn.MPI_Op_free = (int (*)( MPI_Op *))GetProcAddress(hMPIModule, "MPI_Op_free");
    if (fn.MPI_Op_free == NULL) fn.MPI_Op_free = (int (*)( MPI_Op *))GetProcAddress(hPMPIModule, "MPI_Op_free");
    fn.MPI_Allreduce = (int (*)(void* , void*, int, MPI_Datatype, MPI_Op, MPI_Comm))GetProcAddress(hMPIModule, "MPI_Allreduce");
    if (fn.MPI_Allreduce == NULL) fn.MPI_Allreduce = (int (*)(void* , void*, int, MPI_Datatype, MPI_Op, MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Allreduce");
    fn.MPI_Reduce_scatter = (int (*)(void* , void*, int *, MPI_Datatype, MPI_Op, MPI_Comm))GetProcAddress(hMPIModule, "MPI_Reduce_scatter");
    if (fn.MPI_Reduce_scatter == NULL) fn.MPI_Reduce_scatter = (int (*)(void* , void*, int *, MPI_Datatype, MPI_Op, MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Reduce_scatter");
    fn.MPI_Scan = (int (*)(void* , void*, int, MPI_Datatype, MPI_Op, MPI_Comm ))GetProcAddress(hMPIModule, "MPI_Scan");
    if (fn.MPI_Scan == NULL) fn.MPI_Scan = (int (*)(void* , void*, int, MPI_Datatype, MPI_Op, MPI_Comm ))GetProcAddress(hPMPIModule, "MPI_Scan");
    fn.MPI_Group_size = (int (*)(MPI_Group, int *))GetProcAddress(hMPIModule, "MPI_Group_size");
    if (fn.MPI_Group_size == NULL) fn.MPI_Group_size = (int (*)(MPI_Group, int *))GetProcAddress(hPMPIModule, "MPI_Group_size");
    fn.MPI_Group_rank = (int (*)(MPI_Group, int *))GetProcAddress(hMPIModule, "MPI_Group_rank");
    if (fn.MPI_Group_rank == NULL) fn.MPI_Group_rank = (int (*)(MPI_Group, int *))GetProcAddress(hPMPIModule, "MPI_Group_rank");
    fn.MPI_Group_translate_ranks = (int (* )(MPI_Group, int, int *, MPI_Group, int *))GetProcAddress(hMPIModule, "MPI_Group_translate_ranks");
    if (fn.MPI_Group_translate_ranks == NULL) fn.MPI_Group_translate_ranks = (int (* )(MPI_Group, int, int *, MPI_Group, int *))GetProcAddress(hPMPIModule, "MPI_Group_translate_ranks");
    fn.MPI_Group_compare = (int (*)(MPI_Group, MPI_Group, int *))GetProcAddress(hMPIModule, "MPI_Group_compare");
    if (fn.MPI_Group_compare == NULL) fn.MPI_Group_compare = (int (*)(MPI_Group, MPI_Group, int *))GetProcAddress(hPMPIModule, "MPI_Group_compare");
    fn.MPI_Comm_group = (int (*)(MPI_Comm, MPI_Group *))GetProcAddress(hMPIModule, "MPI_Comm_group");
    if (fn.MPI_Comm_group == NULL) fn.MPI_Comm_group = (int (*)(MPI_Comm, MPI_Group *))GetProcAddress(hPMPIModule, "MPI_Comm_group");
    fn.MPI_Group_union = (int (*)(MPI_Group, MPI_Group, MPI_Group *))GetProcAddress(hMPIModule, "MPI_Group_union");
    if (fn.MPI_Group_union == NULL) fn.MPI_Group_union = (int (*)(MPI_Group, MPI_Group, MPI_Group *))GetProcAddress(hPMPIModule, "MPI_Group_union");
    fn.MPI_Group_intersection = (int (*)(MPI_Group, MPI_Group, MPI_Group *))GetProcAddress(hMPIModule, "MPI_Group_intersection");
    if (fn.MPI_Group_intersection == NULL) fn.MPI_Group_intersection = (int (*)(MPI_Group, MPI_Group, MPI_Group *))GetProcAddress(hPMPIModule, "MPI_Group_intersection");
    fn.MPI_Group_difference = (int (*)(MPI_Group, MPI_Group, MPI_Group *))GetProcAddress(hMPIModule, "MPI_Group_difference");
    if (fn.MPI_Group_difference == NULL) fn.MPI_Group_difference = (int (*)(MPI_Group, MPI_Group, MPI_Group *))GetProcAddress(hPMPIModule, "MPI_Group_difference");
    fn.MPI_Group_incl = (int (*)(MPI_Group, int, int *, MPI_Group *))GetProcAddress(hMPIModule, "MPI_Group_incl");
    if (fn.MPI_Group_incl == NULL) fn.MPI_Group_incl = (int (*)(MPI_Group, int, int *, MPI_Group *))GetProcAddress(hPMPIModule, "MPI_Group_incl");
    fn.MPI_Group_excl = (int (*)(MPI_Group, int, int *, MPI_Group *))GetProcAddress(hMPIModule, "MPI_Group_excl");
    if (fn.MPI_Group_excl == NULL) fn.MPI_Group_excl = (int (*)(MPI_Group, int, int *, MPI_Group *))GetProcAddress(hPMPIModule, "MPI_Group_excl");
    fn.MPI_Group_range_incl = (int (*)(MPI_Group, int, int [][3], MPI_Group *))GetProcAddress(hMPIModule, "MPI_Group_range_incl");
    if (fn.MPI_Group_range_incl == NULL) fn.MPI_Group_range_incl = (int (*)(MPI_Group, int, int [][3], MPI_Group *))GetProcAddress(hPMPIModule, "MPI_Group_range_incl");
    fn.MPI_Group_range_excl = (int (*)(MPI_Group, int, int [][3], MPI_Group *))GetProcAddress(hMPIModule, "MPI_Group_range_excl");
    if (fn.MPI_Group_range_excl == NULL) fn.MPI_Group_range_excl = (int (*)(MPI_Group, int, int [][3], MPI_Group *))GetProcAddress(hPMPIModule, "MPI_Group_range_excl");
    fn.MPI_Group_free = (int (*)(MPI_Group *))GetProcAddress(hMPIModule, "MPI_Group_free");
    if (fn.MPI_Group_free == NULL) fn.MPI_Group_free = (int (*)(MPI_Group *))GetProcAddress(hPMPIModule, "MPI_Group_free");
    fn.MPI_Comm_size = (int (*)(MPI_Comm, int *))GetProcAddress(hMPIModule, "MPI_Comm_size");
    if (fn.MPI_Comm_size == NULL) fn.MPI_Comm_size = (int (*)(MPI_Comm, int *))GetProcAddress(hPMPIModule, "MPI_Comm_size");
    fn.MPI_Comm_rank = (int (*)(MPI_Comm, int *))GetProcAddress(hMPIModule, "MPI_Comm_rank");
    if (fn.MPI_Comm_rank == NULL) fn.MPI_Comm_rank = (int (*)(MPI_Comm, int *))GetProcAddress(hPMPIModule, "MPI_Comm_rank");
    fn.MPI_Comm_compare = (int (*)(MPI_Comm, MPI_Comm, int *))GetProcAddress(hMPIModule, "MPI_Comm_compare");
    if (fn.MPI_Comm_compare == NULL) fn.MPI_Comm_compare = (int (*)(MPI_Comm, MPI_Comm, int *))GetProcAddress(hPMPIModule, "MPI_Comm_compare");
    fn.MPI_Comm_dup = (int (*)(MPI_Comm, MPI_Comm *))GetProcAddress(hMPIModule, "MPI_Comm_dup");
    if (fn.MPI_Comm_dup == NULL) fn.MPI_Comm_dup = (int (*)(MPI_Comm, MPI_Comm *))GetProcAddress(hPMPIModule, "MPI_Comm_dup");
    fn.MPI_Comm_create = (int (*)(MPI_Comm, MPI_Group, MPI_Comm *))GetProcAddress(hMPIModule, "MPI_Comm_create");
    if (fn.MPI_Comm_create == NULL) fn.MPI_Comm_create = (int (*)(MPI_Comm, MPI_Group, MPI_Comm *))GetProcAddress(hPMPIModule, "MPI_Comm_create");
    fn.MPI_Comm_split = (int (*)(MPI_Comm, int, int, MPI_Comm *))GetProcAddress(hMPIModule, "MPI_Comm_split");
    if (fn.MPI_Comm_split == NULL) fn.MPI_Comm_split = (int (*)(MPI_Comm, int, int, MPI_Comm *))GetProcAddress(hPMPIModule, "MPI_Comm_split");
    fn.MPI_Comm_free = (int (*)(MPI_Comm *))GetProcAddress(hMPIModule, "MPI_Comm_free");
    if (fn.MPI_Comm_free == NULL) fn.MPI_Comm_free = (int (*)(MPI_Comm *))GetProcAddress(hPMPIModule, "MPI_Comm_free");
    fn.MPI_Comm_test_inter = (int (*)(MPI_Comm, int *))GetProcAddress(hMPIModule, "MPI_Comm_test_inter");
    if (fn.MPI_Comm_test_inter == NULL) fn.MPI_Comm_test_inter = (int (*)(MPI_Comm, int *))GetProcAddress(hPMPIModule, "MPI_Comm_test_inter");
    fn.MPI_Comm_remote_size = (int (*)(MPI_Comm, int *))GetProcAddress(hMPIModule, "MPI_Comm_remote_size");
    if (fn.MPI_Comm_remote_size == NULL) fn.MPI_Comm_remote_size = (int (*)(MPI_Comm, int *))GetProcAddress(hPMPIModule, "MPI_Comm_remote_size");
    fn.MPI_Comm_remote_group = (int (*)(MPI_Comm, MPI_Group *))GetProcAddress(hMPIModule, "MPI_Comm_remote_group");
    if (fn.MPI_Comm_remote_group == NULL) fn.MPI_Comm_remote_group = (int (*)(MPI_Comm, MPI_Group *))GetProcAddress(hPMPIModule, "MPI_Comm_remote_group");
    fn.MPI_Intercomm_create = (int (*)(MPI_Comm, int, MPI_Comm, int, int, MPI_Comm * ))GetProcAddress(hMPIModule, "MPI_Intercomm_create");
    if (fn.MPI_Intercomm_create == NULL) fn.MPI_Intercomm_create = (int (*)(MPI_Comm, int, MPI_Comm, int, int, MPI_Comm * ))GetProcAddress(hPMPIModule, "MPI_Intercomm_create");
    fn.MPI_Intercomm_merge = (int (*)(MPI_Comm, int, MPI_Comm *))GetProcAddress(hMPIModule, "MPI_Intercomm_merge");
    if (fn.MPI_Intercomm_merge == NULL) fn.MPI_Intercomm_merge = (int (*)(MPI_Comm, int, MPI_Comm *))GetProcAddress(hPMPIModule, "MPI_Intercomm_merge");
    fn.MPI_Keyval_create = (int (*)(MPI_Copy_function *, MPI_Delete_function *, int *, void*))GetProcAddress(hMPIModule, "MPI_Keyval_create");
    if (fn.MPI_Keyval_create == NULL) fn.MPI_Keyval_create = (int (*)(MPI_Copy_function *, MPI_Delete_function *, int *, void*))GetProcAddress(hPMPIModule, "MPI_Keyval_create");
    fn.MPI_Keyval_free = (int (*)(int *))GetProcAddress(hMPIModule, "MPI_Keyval_free");
    if (fn.MPI_Keyval_free == NULL) fn.MPI_Keyval_free = (int (*)(int *))GetProcAddress(hPMPIModule, "MPI_Keyval_free");
    fn.MPI_Attr_put = (int (*)(MPI_Comm, int, void*))GetProcAddress(hMPIModule, "MPI_Attr_put");
    if (fn.MPI_Attr_put == NULL) fn.MPI_Attr_put = (int (*)(MPI_Comm, int, void*))GetProcAddress(hPMPIModule, "MPI_Attr_put");
    fn.MPI_Attr_get = (int (*)(MPI_Comm, int, void *, int *))GetProcAddress(hMPIModule, "MPI_Attr_get");
    if (fn.MPI_Attr_get == NULL) fn.MPI_Attr_get = (int (*)(MPI_Comm, int, void *, int *))GetProcAddress(hPMPIModule, "MPI_Attr_get");
    fn.MPI_Attr_delete = (int (*)(MPI_Comm, int))GetProcAddress(hMPIModule, "MPI_Attr_delete");
    if (fn.MPI_Attr_delete == NULL) fn.MPI_Attr_delete = (int (*)(MPI_Comm, int))GetProcAddress(hPMPIModule, "MPI_Attr_delete");
    fn.MPI_Topo_test = (int (*)(MPI_Comm, int *))GetProcAddress(hMPIModule, "MPI_Topo_test");
    if (fn.MPI_Topo_test == NULL) fn.MPI_Topo_test = (int (*)(MPI_Comm, int *))GetProcAddress(hPMPIModule, "MPI_Topo_test");
    fn.MPI_Cart_create = (int (*)(MPI_Comm, int, int *, int *, int, MPI_Comm *))GetProcAddress(hMPIModule, "MPI_Cart_create");
    if (fn.MPI_Cart_create == NULL) fn.MPI_Cart_create = (int (*)(MPI_Comm, int, int *, int *, int, MPI_Comm *))GetProcAddress(hPMPIModule, "MPI_Cart_create");
    fn.MPI_Dims_create = (int (*)(int, int, int *))GetProcAddress(hMPIModule, "MPI_Dims_create");
    if (fn.MPI_Dims_create == NULL) fn.MPI_Dims_create = (int (*)(int, int, int *))GetProcAddress(hPMPIModule, "MPI_Dims_create");
    fn.MPI_Graph_create = (int (*)(MPI_Comm, int, int *, int *, int, MPI_Comm *))GetProcAddress(hMPIModule, "MPI_Graph_create");
    if (fn.MPI_Graph_create == NULL) fn.MPI_Graph_create = (int (*)(MPI_Comm, int, int *, int *, int, MPI_Comm *))GetProcAddress(hPMPIModule, "MPI_Graph_create");
    fn.MPI_Graphdims_get = (int (*)(MPI_Comm, int *, int *))GetProcAddress(hMPIModule, "MPI_Graphdims_get");
    if (fn.MPI_Graphdims_get == NULL) fn.MPI_Graphdims_get = (int (*)(MPI_Comm, int *, int *))GetProcAddress(hPMPIModule, "MPI_Graphdims_get");
    fn.MPI_Graph_get = (int (*)(MPI_Comm, int, int, int *, int *))GetProcAddress(hMPIModule, "MPI_Graph_get");
    if (fn.MPI_Graph_get == NULL) fn.MPI_Graph_get = (int (*)(MPI_Comm, int, int, int *, int *))GetProcAddress(hPMPIModule, "MPI_Graph_get");
    fn.MPI_Cartdim_get = (int (*)(MPI_Comm, int *))GetProcAddress(hMPIModule, "MPI_Cartdim_get");
    if (fn.MPI_Cartdim_get == NULL) fn.MPI_Cartdim_get = (int (*)(MPI_Comm, int *))GetProcAddress(hPMPIModule, "MPI_Cartdim_get");
    fn.MPI_Cart_get = (int (*)(MPI_Comm, int, int *, int *, int *))GetProcAddress(hMPIModule, "MPI_Cart_get");
    if (fn.MPI_Cart_get == NULL) fn.MPI_Cart_get = (int (*)(MPI_Comm, int, int *, int *, int *))GetProcAddress(hPMPIModule, "MPI_Cart_get");
    fn.MPI_Cart_rank = (int (*)(MPI_Comm, int *, int *))GetProcAddress(hMPIModule, "MPI_Cart_rank");
    if (fn.MPI_Cart_rank == NULL) fn.MPI_Cart_rank = (int (*)(MPI_Comm, int *, int *))GetProcAddress(hPMPIModule, "MPI_Cart_rank");
    fn.MPI_Cart_coords = (int (*)(MPI_Comm, int, int, int *))GetProcAddress(hMPIModule, "MPI_Cart_coords");
    if (fn.MPI_Cart_coords == NULL) fn.MPI_Cart_coords = (int (*)(MPI_Comm, int, int, int *))GetProcAddress(hPMPIModule, "MPI_Cart_coords");
    fn.MPI_Graph_neighbors_count = (int (*)(MPI_Comm, int, int *))GetProcAddress(hMPIModule, "MPI_Graph_neighbors_count");
    if (fn.MPI_Graph_neighbors_count == NULL) fn.MPI_Graph_neighbors_count = (int (*)(MPI_Comm, int, int *))GetProcAddress(hPMPIModule, "MPI_Graph_neighbors_count");
    fn.MPI_Graph_neighbors = (int (*)(MPI_Comm, int, int, int *))GetProcAddress(hMPIModule, "MPI_Graph_neighbors");
    if (fn.MPI_Graph_neighbors == NULL) fn.MPI_Graph_neighbors = (int (*)(MPI_Comm, int, int, int *))GetProcAddress(hPMPIModule, "MPI_Graph_neighbors");
    fn.MPI_Cart_shift = (int (*)(MPI_Comm, int, int, int *, int *))GetProcAddress(hMPIModule, "MPI_Cart_shift");
    if (fn.MPI_Cart_shift == NULL) fn.MPI_Cart_shift = (int (*)(MPI_Comm, int, int, int *, int *))GetProcAddress(hPMPIModule, "MPI_Cart_shift");
    fn.MPI_Cart_sub = (int (*)(MPI_Comm, int *, MPI_Comm *))GetProcAddress(hMPIModule, "MPI_Cart_sub");
    if (fn.MPI_Cart_sub == NULL) fn.MPI_Cart_sub = (int (*)(MPI_Comm, int *, MPI_Comm *))GetProcAddress(hPMPIModule, "MPI_Cart_sub");
    fn.MPI_Cart_map = (int (*)(MPI_Comm, int, int *, int *, int *))GetProcAddress(hMPIModule, "MPI_Cart_map");
    if (fn.MPI_Cart_map == NULL) fn.MPI_Cart_map = (int (*)(MPI_Comm, int, int *, int *, int *))GetProcAddress(hPMPIModule, "MPI_Cart_map");
    fn.MPI_Graph_map = (int (*)(MPI_Comm, int, int *, int *, int *))GetProcAddress(hMPIModule, "MPI_Graph_map");
    if (fn.MPI_Graph_map == NULL) fn.MPI_Graph_map = (int (*)(MPI_Comm, int, int *, int *, int *))GetProcAddress(hPMPIModule, "MPI_Graph_map");
    fn.MPI_Get_processor_name = (int (*)(char *, int *))GetProcAddress(hMPIModule, "MPI_Get_processor_name");
    if (fn.MPI_Get_processor_name == NULL) fn.MPI_Get_processor_name = (int (*)(char *, int *))GetProcAddress(hPMPIModule, "MPI_Get_processor_name");
    fn.MPI_Get_version = (int (*)(int *, int *))GetProcAddress(hMPIModule, "MPI_Get_version");
    if (fn.MPI_Get_version == NULL) fn.MPI_Get_version = (int (*)(int *, int *))GetProcAddress(hPMPIModule, "MPI_Get_version");
    fn.MPI_Errhandler_create = (int (*)(MPI_Handler_function *, MPI_Errhandler *))GetProcAddress(hMPIModule, "MPI_Errhandler_create");
    if (fn.MPI_Errhandler_create == NULL) fn.MPI_Errhandler_create = (int (*)(MPI_Handler_function *, MPI_Errhandler *))GetProcAddress(hPMPIModule, "MPI_Errhandler_create");
    fn.MPI_Errhandler_set = (int (*)(MPI_Comm, MPI_Errhandler))GetProcAddress(hMPIModule, "MPI_Errhandler_set");
    if (fn.MPI_Errhandler_set == NULL) fn.MPI_Errhandler_set = (int (*)(MPI_Comm, MPI_Errhandler))GetProcAddress(hPMPIModule, "MPI_Errhandler_set");
    fn.MPI_Errhandler_get = (int (*)(MPI_Comm, MPI_Errhandler *))GetProcAddress(hMPIModule, "MPI_Errhandler_get");
    if (fn.MPI_Errhandler_get == NULL) fn.MPI_Errhandler_get = (int (*)(MPI_Comm, MPI_Errhandler *))GetProcAddress(hPMPIModule, "MPI_Errhandler_get");
    fn.MPI_Errhandler_free = (int (*)(MPI_Errhandler *))GetProcAddress(hMPIModule, "MPI_Errhandler_free");
    if (fn.MPI_Errhandler_free == NULL) fn.MPI_Errhandler_free = (int (*)(MPI_Errhandler *))GetProcAddress(hPMPIModule, "MPI_Errhandler_free");
    fn.MPI_Error_string = (int (*)(int, char *, int *))GetProcAddress(hMPIModule, "MPI_Error_string");
    if (fn.MPI_Error_string == NULL) fn.MPI_Error_string = (int (*)(int, char *, int *))GetProcAddress(hPMPIModule, "MPI_Error_string");
    fn.MPI_Error_class = (int (*)(int, int *))GetProcAddress(hMPIModule, "MPI_Error_class");
    if (fn.MPI_Error_class == NULL) fn.MPI_Error_class = (int (*)(int, int *))GetProcAddress(hPMPIModule, "MPI_Error_class");
    fn.MPI_Wtime = (double (*)(void))GetProcAddress(hMPIModule, "MPI_Wtime");
    if (fn.MPI_Wtime == NULL) fn.MPI_Wtime = (double (*)(void))GetProcAddress(hPMPIModule, "MPI_Wtime");
    if (fn.MPI_Wtime == NULL) fn.MPI_Wtime = (double (*)(void))GetProcAddress(hPMPIModule, "MPI_Wtime");
    fn.MPI_Wtick = (double (*)(void))GetProcAddress(hMPIModule, "MPI_Wtick");
    if (fn.MPI_Wtick == NULL) fn.MPI_Wtick = (double (*)(void))GetProcAddress(hPMPIModule, "MPI_Wtick");
    fn.MPI_Init = (int (*)(int *, char ***))GetProcAddress(hMPIModule, "MPI_Init");
    if (fn.MPI_Init == NULL) fn.MPI_Init = (int (*)(int *, char ***))GetProcAddress(hPMPIModule, "MPI_Init");
    fn.MPI_Finalize = (int (*)(void))GetProcAddress(hMPIModule, "MPI_Finalize");
    if (fn.MPI_Finalize == NULL) fn.MPI_Finalize = (int (*)(void))GetProcAddress(hPMPIModule, "MPI_Finalize");
    fn.MPI_Initialized = (int (*)(int *))GetProcAddress(hMPIModule, "MPI_Initialized");
    if (fn.MPI_Initialized == NULL) fn.MPI_Initialized = (int (*)(int *))GetProcAddress(hPMPIModule, "MPI_Initialized");
    fn.MPI_Abort = (int (*)(MPI_Comm, int))GetProcAddress(hMPIModule, "MPI_Abort");
    if (fn.MPI_Abort == NULL) fn.MPI_Abort = (int (*)(MPI_Comm, int))GetProcAddress(hPMPIModule, "MPI_Abort");
    fn.MPI_Pcontrol = (int (*)(const int, ...))GetProcAddress(hMPIModule, "MPI_Pcontrol");
    if (fn.MPI_Pcontrol == NULL) fn.MPI_Pcontrol = (int (*)(const int, ...))GetProcAddress(hPMPIModule, "MPI_Pcontrol");
    fn.MPI_DUP_FN = (int (* )( MPI_Comm, int, void *, void *, void *, int * ))GetProcAddress(hMPIModule, "MPIR_Dup_fn"/*"MPI_DUP_FN"*/);
    if (fn.MPI_DUP_FN == NULL) fn.MPI_DUP_FN = (int (* )( MPI_Comm, int, void *, void *, void *, int * ))GetProcAddress(hPMPIModule, "MPIR_Dup_fn"/*"MPI_DUP_FN"*/);
    fn.MPI_Close_port = (int (*)(char *))GetProcAddress(hMPIModule, "MPI_Close_port");
    if (fn.MPI_Close_port == NULL) fn.MPI_Close_port = (int (*)(char *))GetProcAddress(hPMPIModule, "MPI_Close_port");
    fn.MPI_Comm_accept = (int (*)(char *, MPI_Info, int, MPI_Comm, MPI_Comm *))GetProcAddress(hMPIModule, "MPI_Comm_accept");
    if (fn.MPI_Comm_accept == NULL) fn.MPI_Comm_accept = (int (*)(char *, MPI_Info, int, MPI_Comm, MPI_Comm *))GetProcAddress(hPMPIModule, "MPI_Comm_accept");
    fn.MPI_Comm_connect = (int (*)(char *, MPI_Info, int, MPI_Comm, MPI_Comm *))GetProcAddress(hMPIModule, "MPI_Comm_connect");
    if (fn.MPI_Comm_connect == NULL) fn.MPI_Comm_connect = (int (*)(char *, MPI_Info, int, MPI_Comm, MPI_Comm *))GetProcAddress(hPMPIModule, "MPI_Comm_connect");
    fn.MPI_Comm_disconnect = (int (*)(MPI_Comm *))GetProcAddress(hMPIModule, "MPI_Comm_disconnect");
    if (fn.MPI_Comm_disconnect == NULL) fn.MPI_Comm_disconnect = (int (*)(MPI_Comm *))GetProcAddress(hPMPIModule, "MPI_Comm_disconnect");
    fn.MPI_Comm_get_parent = (int (*)(MPI_Comm *))GetProcAddress(hMPIModule, "MPI_Comm_get_parent");
    if (fn.MPI_Comm_get_parent == NULL) fn.MPI_Comm_get_parent = (int (*)(MPI_Comm *))GetProcAddress(hPMPIModule, "MPI_Comm_get_parent");
    fn.MPI_Comm_join = (int (*)(int, MPI_Comm *))GetProcAddress(hMPIModule, "MPI_Comm_join");
    if (fn.MPI_Comm_join == NULL) fn.MPI_Comm_join = (int (*)(int, MPI_Comm *))GetProcAddress(hPMPIModule, "MPI_Comm_join");
    fn.MPI_Comm_spawn = (int (*)(char *, char *[], int, MPI_Info, int, MPI_Comm, MPI_Comm *, int []))GetProcAddress(hMPIModule, "MPI_Comm_spawn");
    if (fn.MPI_Comm_spawn == NULL) fn.MPI_Comm_spawn = (int (*)(char *, char *[], int, MPI_Info, int, MPI_Comm, MPI_Comm *, int []))GetProcAddress(hPMPIModule, "MPI_Comm_spawn");
    fn.MPI_Comm_spawn_multiple = (int (*)(int, char *[], char **[], int [], MPI_Info [], int, MPI_Comm, MPI_Comm *, int []))GetProcAddress(hMPIModule, "MPI_Comm_spawn_multiple"); 
    if (fn.MPI_Comm_spawn_multiple == NULL) fn.MPI_Comm_spawn_multiple = (int (*)(int, char *[], char **[], int [], MPI_Info [], int, MPI_Comm, MPI_Comm *, int []))GetProcAddress(hPMPIModule, "MPI_Comm_spawn_multiple"); 
    fn.MPI_Lookup_name = (int (*)(char *, MPI_Info, char *))GetProcAddress(hMPIModule, "MPI_Lookup_name");
    if (fn.MPI_Lookup_name == NULL) fn.MPI_Lookup_name = (int (*)(char *, MPI_Info, char *))GetProcAddress(hPMPIModule, "MPI_Lookup_name");
    fn.MPI_Open_port = (int (*)(MPI_Info, char *))GetProcAddress(hMPIModule, "MPI_Open_port");
    if (fn.MPI_Open_port == NULL) fn.MPI_Open_port = (int (*)(MPI_Info, char *))GetProcAddress(hPMPIModule, "MPI_Open_port");
    fn.MPI_Publish_name = (int (*)(char *, MPI_Info, char *))GetProcAddress(hMPIModule, "MPI_Publish_name");
    if (fn.MPI_Publish_name == NULL) fn.MPI_Publish_name = (int (*)(char *, MPI_Info, char *))GetProcAddress(hPMPIModule, "MPI_Publish_name");
    fn.MPI_Unpublish_name = (int (*)(char *, MPI_Info, char *))GetProcAddress(hMPIModule, "MPI_Unpublish_name");
    if (fn.MPI_Unpublish_name == NULL) fn.MPI_Unpublish_name = (int (*)(char *, MPI_Info, char *))GetProcAddress(hPMPIModule, "MPI_Unpublish_name");
    fn.MPI_Accumulate = (int (*)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype,  MPI_Op, MPI_Win))GetProcAddress(hMPIModule, "MPI_Accumulate");
    if (fn.MPI_Accumulate == NULL) fn.MPI_Accumulate = (int (*)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype,  MPI_Op, MPI_Win))GetProcAddress(hPMPIModule, "MPI_Accumulate");
    fn.MPI_Get = (int (*)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win))GetProcAddress(hMPIModule, "MPI_Get");
    if (fn.MPI_Get == NULL) fn.MPI_Get = (int (*)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win))GetProcAddress(hPMPIModule, "MPI_Get");
    fn.MPI_Put = (int (*)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win))GetProcAddress(hMPIModule, "MPI_Put");
    if (fn.MPI_Put == NULL) fn.MPI_Put = (int (*)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win))GetProcAddress(hPMPIModule, "MPI_Put");
    fn.MPI_Win_complete = (int (*)(MPI_Win))GetProcAddress(hMPIModule, "MPI_Win_complete");
    if (fn.MPI_Win_complete == NULL) fn.MPI_Win_complete = (int (*)(MPI_Win))GetProcAddress(hPMPIModule, "MPI_Win_complete");
    fn.MPI_Win_create = (int (*)(void *, MPI_Aint, int, MPI_Info, MPI_Comm, MPI_Win *))GetProcAddress(hMPIModule, "MPI_Win_create");
    if (fn.MPI_Win_create == NULL) fn.MPI_Win_create = (int (*)(void *, MPI_Aint, int, MPI_Info, MPI_Comm, MPI_Win *))GetProcAddress(hPMPIModule, "MPI_Win_create");
    fn.MPI_Win_fence = (int (*)(int, MPI_Win))GetProcAddress(hMPIModule, "MPI_Win_fence");
    if (fn.MPI_Win_fence == NULL) fn.MPI_Win_fence = (int (*)(int, MPI_Win))GetProcAddress(hPMPIModule, "MPI_Win_fence");
    fn.MPI_Win_free = (int (*)(MPI_Win *))GetProcAddress(hMPIModule, "MPI_Win_free");
    if (fn.MPI_Win_free == NULL) fn.MPI_Win_free = (int (*)(MPI_Win *))GetProcAddress(hPMPIModule, "MPI_Win_free");
    fn.MPI_Win_get_group = (int (*)(MPI_Win, MPI_Group *))GetProcAddress(hMPIModule, "MPI_Win_get_group");
    if (fn.MPI_Win_get_group == NULL) fn.MPI_Win_get_group = (int (*)(MPI_Win, MPI_Group *))GetProcAddress(hPMPIModule, "MPI_Win_get_group");
    fn.MPI_Win_lock = (int (*)(int, int, int, MPI_Win))GetProcAddress(hMPIModule, "MPI_Win_lock");
    if (fn.MPI_Win_lock == NULL) fn.MPI_Win_lock = (int (*)(int, int, int, MPI_Win))GetProcAddress(hPMPIModule, "MPI_Win_lock");
    fn.MPI_Win_post = (int (*)(MPI_Group, int, MPI_Win))GetProcAddress(hMPIModule, "MPI_Win_post");
    if (fn.MPI_Win_post == NULL) fn.MPI_Win_post = (int (*)(MPI_Group, int, MPI_Win))GetProcAddress(hPMPIModule, "MPI_Win_post");
    fn.MPI_Win_start = (int (*)(MPI_Group, int, MPI_Win))GetProcAddress(hMPIModule, "MPI_Win_start");
    if (fn.MPI_Win_start == NULL) fn.MPI_Win_start = (int (*)(MPI_Group, int, MPI_Win))GetProcAddress(hPMPIModule, "MPI_Win_start");
    fn.MPI_Win_test = (int (*)(MPI_Win, int *))GetProcAddress(hMPIModule, "MPI_Win_test");
    if (fn.MPI_Win_test == NULL) fn.MPI_Win_test = (int (*)(MPI_Win, int *))GetProcAddress(hPMPIModule, "MPI_Win_test");
    fn.MPI_Win_unlock = (int (*)(int, MPI_Win))GetProcAddress(hMPIModule, "MPI_Win_unlock");
    if (fn.MPI_Win_unlock == NULL) fn.MPI_Win_unlock = (int (*)(int, MPI_Win))GetProcAddress(hPMPIModule, "MPI_Win_unlock");
    fn.MPI_Win_wait = (int (*)(MPI_Win))GetProcAddress(hMPIModule, "MPI_Win_wait");
    if (fn.MPI_Win_wait == NULL) fn.MPI_Win_wait = (int (*)(MPI_Win))GetProcAddress(hPMPIModule, "MPI_Win_wait");
    fn.MPI_Alltoallw = (int (*)(void *, int [], int [], MPI_Datatype [], void *, int [], int [], MPI_Datatype [], MPI_Comm))GetProcAddress(hMPIModule, "MPI_Alltoallw");
    if (fn.MPI_Alltoallw == NULL) fn.MPI_Alltoallw = (int (*)(void *, int [], int [], MPI_Datatype [], void *, int [], int [], MPI_Datatype [], MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Alltoallw");
    fn.MPI_Exscan = (int (*)(void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm))GetProcAddress(hMPIModule, "MPI_Exscan");
    if (fn.MPI_Exscan == NULL) fn.MPI_Exscan = (int (*)(void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm))GetProcAddress(hPMPIModule, "MPI_Exscan");
    fn.MPI_Add_error_class = (int (*)(int *))GetProcAddress(hMPIModule, "MPI_Add_error_class");
    if (fn.MPI_Add_error_class == NULL) fn.MPI_Add_error_class = (int (*)(int *))GetProcAddress(hPMPIModule, "MPI_Add_error_class");
    fn.MPI_Add_error_code = (int (*)(int, int *))GetProcAddress(hMPIModule, "MPI_Add_error_code");
    if (fn.MPI_Add_error_code == NULL) fn.MPI_Add_error_code = (int (*)(int, int *))GetProcAddress(hPMPIModule, "MPI_Add_error_code");
    fn.MPI_Add_error_string = (int (*)(int, char *))GetProcAddress(hMPIModule, "MPI_Add_error_string");
    if (fn.MPI_Add_error_string == NULL) fn.MPI_Add_error_string = (int (*)(int, char *))GetProcAddress(hPMPIModule, "MPI_Add_error_string");
    fn.MPI_Comm_call_errhandler = (int (*)(MPI_Comm, int))GetProcAddress(hMPIModule, "MPI_Comm_call_errhandler");
    if (fn.MPI_Comm_call_errhandler == NULL) fn.MPI_Comm_call_errhandler = (int (*)(MPI_Comm, int))GetProcAddress(hPMPIModule, "MPI_Comm_call_errhandler");
    fn.MPI_Comm_create_keyval = (int (*)(MPI_Comm_copy_attr_function *, MPI_Comm_delete_attr_function *, int *, void *))GetProcAddress(hMPIModule, "MPI_Comm_create_keyval");
    if (fn.MPI_Comm_create_keyval == NULL) fn.MPI_Comm_create_keyval = (int (*)(MPI_Comm_copy_attr_function *, MPI_Comm_delete_attr_function *, int *, void *))GetProcAddress(hPMPIModule, "MPI_Comm_create_keyval");
    fn.MPI_Comm_delete_attr = (int (*)(MPI_Comm, int))GetProcAddress(hMPIModule, "MPI_Comm_delete_attr");
    if (fn.MPI_Comm_delete_attr == NULL) fn.MPI_Comm_delete_attr = (int (*)(MPI_Comm, int))GetProcAddress(hPMPIModule, "MPI_Comm_delete_attr");
    fn.MPI_Comm_free_keyval = (int (*)(int *))GetProcAddress(hMPIModule, "MPI_Comm_free_keyval");
    if (fn.MPI_Comm_free_keyval == NULL) fn.MPI_Comm_free_keyval = (int (*)(int *))GetProcAddress(hPMPIModule, "MPI_Comm_free_keyval");
    fn.MPI_Comm_get_attr = (int (*)(MPI_Comm, int, void *, int *))GetProcAddress(hMPIModule, "MPI_Comm_get_attr");
    if (fn.MPI_Comm_get_attr == NULL) fn.MPI_Comm_get_attr = (int (*)(MPI_Comm, int, void *, int *))GetProcAddress(hPMPIModule, "MPI_Comm_get_attr");
    fn.MPI_Comm_get_name = (int (*)(MPI_Comm, char *, int *))GetProcAddress(hMPIModule, "MPI_Comm_get_name");
    if (fn.MPI_Comm_get_name == NULL) fn.MPI_Comm_get_name = (int (*)(MPI_Comm, char *, int *))GetProcAddress(hPMPIModule, "MPI_Comm_get_name");
    fn.MPI_Comm_set_attr = (int (*)(MPI_Comm, int, void *))GetProcAddress(hMPIModule, "MPI_Comm_set_attr");
    if (fn.MPI_Comm_set_attr == NULL) fn.MPI_Comm_set_attr = (int (*)(MPI_Comm, int, void *))GetProcAddress(hPMPIModule, "MPI_Comm_set_attr");
    fn.MPI_Comm_set_name = (int (*)(MPI_Comm, char *))GetProcAddress(hMPIModule, "MPI_Comm_set_name");
    if (fn.MPI_Comm_set_name == NULL) fn.MPI_Comm_set_name = (int (*)(MPI_Comm, char *))GetProcAddress(hPMPIModule, "MPI_Comm_set_name");
    fn.MPI_File_call_errhandler = (int (*)(MPI_File, int))GetProcAddress(hMPIModule, "MPI_File_call_errhandler");
    if (fn.MPI_File_call_errhandler == NULL) fn.MPI_File_call_errhandler = (int (*)(MPI_File, int))GetProcAddress(hPMPIModule, "MPI_File_call_errhandler");
    fn.MPI_Grequest_complete = (int (*)(MPI_Request))GetProcAddress(hMPIModule, "MPI_Grequest_complete");
    if (fn.MPI_Grequest_complete == NULL) fn.MPI_Grequest_complete = (int (*)(MPI_Request))GetProcAddress(hPMPIModule, "MPI_Grequest_complete");
    fn.MPI_Grequest_start = (int (*)(MPI_Grequest_query_function *, MPI_Grequest_free_function *, MPI_Grequest_cancel_function *, void *, MPI_Request *))GetProcAddress(hMPIModule, "MPI_Grequest_start");
    if (fn.MPI_Grequest_start == NULL) fn.MPI_Grequest_start = (int (*)(MPI_Grequest_query_function *, MPI_Grequest_free_function *, MPI_Grequest_cancel_function *, void *, MPI_Request *))GetProcAddress(hPMPIModule, "MPI_Grequest_start");
    fn.MPI_Init_thread = (int (*)(int *, char ***, int, int *))GetProcAddress(hMPIModule, "MPI_Init_thread");
    if (fn.MPI_Init_thread == NULL) fn.MPI_Init_thread = (int (*)(int *, char ***, int, int *))GetProcAddress(hPMPIModule, "MPI_Init_thread");
    fn.MPI_Is_thread_main = (int (*)(int *))GetProcAddress(hMPIModule, "MPI_Is_thread_main");
    if (fn.MPI_Is_thread_main == NULL) fn.MPI_Is_thread_main = (int (*)(int *))GetProcAddress(hPMPIModule, "MPI_Is_thread_main");
    fn.MPI_Query_thread = (int (*)(int *))GetProcAddress(hMPIModule, "MPI_Query_thread");
    if (fn.MPI_Query_thread == NULL) fn.MPI_Query_thread = (int (*)(int *))GetProcAddress(hPMPIModule, "MPI_Query_thread");
    fn.MPI_Status_set_cancelled = (int (*)(MPI_Status *, int))GetProcAddress(hMPIModule, "MPI_Status_set_cancelled");
    if (fn.MPI_Status_set_cancelled == NULL) fn.MPI_Status_set_cancelled = (int (*)(MPI_Status *, int))GetProcAddress(hPMPIModule, "MPI_Status_set_cancelled");
    fn.MPI_Status_set_elements = (int (*)(MPI_Status *, MPI_Datatype, int))GetProcAddress(hMPIModule, "MPI_Status_set_elements");
    if (fn.MPI_Status_set_elements == NULL) fn.MPI_Status_set_elements = (int (*)(MPI_Status *, MPI_Datatype, int))GetProcAddress(hPMPIModule, "MPI_Status_set_elements");
    fn.MPI_Type_create_keyval = (int (*)(MPI_Type_copy_attr_function *, MPI_Type_delete_attr_function *, int *, void *))GetProcAddress(hMPIModule, "MPI_Type_create_keyval");
    if (fn.MPI_Type_create_keyval == NULL) fn.MPI_Type_create_keyval = (int (*)(MPI_Type_copy_attr_function *, MPI_Type_delete_attr_function *, int *, void *))GetProcAddress(hPMPIModule, "MPI_Type_create_keyval");
    fn.MPI_Type_delete_attr = (int (*)(MPI_Datatype, int))GetProcAddress(hMPIModule, "MPI_Type_delete_attr");
    if (fn.MPI_Type_delete_attr == NULL) fn.MPI_Type_delete_attr = (int (*)(MPI_Datatype, int))GetProcAddress(hPMPIModule, "MPI_Type_delete_attr");
    fn.MPI_Type_dup = (int (*)(MPI_Datatype, MPI_Datatype *))GetProcAddress(hMPIModule, "MPI_Type_dup");
    if (fn.MPI_Type_dup == NULL) fn.MPI_Type_dup = (int (*)(MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "MPI_Type_dup");
    fn.MPI_Type_free_keyval = (int (*)(int *))GetProcAddress(hMPIModule, "MPI_Type_free_keyval");
    if (fn.MPI_Type_free_keyval == NULL) fn.MPI_Type_free_keyval = (int (*)(int *))GetProcAddress(hPMPIModule, "MPI_Type_free_keyval");
    fn.MPI_Type_get_attr = (int (*)(MPI_Datatype, int, void *, int *))GetProcAddress(hMPIModule, "MPI_Type_get_attr");
    if (fn.MPI_Type_get_attr == NULL) fn.MPI_Type_get_attr = (int (*)(MPI_Datatype, int, void *, int *))GetProcAddress(hPMPIModule, "MPI_Type_get_attr");
    fn.MPI_Type_get_contents = (int (*)(MPI_Datatype, int, int, int, int [], MPI_Aint [], MPI_Datatype []))GetProcAddress(hMPIModule, "MPI_Type_get_contents");
    if (fn.MPI_Type_get_contents == NULL) fn.MPI_Type_get_contents = (int (*)(MPI_Datatype, int, int, int, int [], MPI_Aint [], MPI_Datatype []))GetProcAddress(hPMPIModule, "MPI_Type_get_contents");
    fn.MPI_Type_get_envelope = (int (*)(MPI_Datatype, int *, int *, int *, int *))GetProcAddress(hMPIModule, "MPI_Type_get_envelope");
    if (fn.MPI_Type_get_envelope == NULL) fn.MPI_Type_get_envelope = (int (*)(MPI_Datatype, int *, int *, int *, int *))GetProcAddress(hPMPIModule, "MPI_Type_get_envelope");
    fn.MPI_Type_get_name = (int (*)(MPI_Datatype, char *, int *))GetProcAddress(hMPIModule, "MPI_Type_get_name");
    if (fn.MPI_Type_get_name == NULL) fn.MPI_Type_get_name = (int (*)(MPI_Datatype, char *, int *))GetProcAddress(hPMPIModule, "MPI_Type_get_name");
    fn.MPI_Type_set_attr = (int (*)(MPI_Datatype, int, void *))GetProcAddress(hMPIModule, "MPI_Type_set_attr");
    if (fn.MPI_Type_set_attr == NULL) fn.MPI_Type_set_attr = (int (*)(MPI_Datatype, int, void *))GetProcAddress(hPMPIModule, "MPI_Type_set_attr");
    fn.MPI_Type_set_name = (int (*)(MPI_Datatype, char *))GetProcAddress(hMPIModule, "MPI_Type_set_name");
    if (fn.MPI_Type_set_name == NULL) fn.MPI_Type_set_name = (int (*)(MPI_Datatype, char *))GetProcAddress(hPMPIModule, "MPI_Type_set_name");
    fn.MPI_Type_match_size = (int (*)( int, int, MPI_Datatype *))GetProcAddress(hMPIModule, "MPI_Type_match_size");
    if (fn.MPI_Type_match_size == NULL) fn.MPI_Type_match_size = (int (*)( int, int, MPI_Datatype *))GetProcAddress(hPMPIModule, "MPI_Type_match_size");
    fn.MPI_Win_call_errhandler = (int (*)(MPI_Win, int))GetProcAddress(hMPIModule, "MPI_Win_call_errhandler");
    if (fn.MPI_Win_call_errhandler == NULL) fn.MPI_Win_call_errhandler = (int (*)(MPI_Win, int))GetProcAddress(hPMPIModule, "MPI_Win_call_errhandler");
    fn.MPI_Win_create_keyval = (int (*)(MPI_Win_copy_attr_function *, MPI_Win_delete_attr_function *, int *, void *))GetProcAddress(hMPIModule, "MPI_Win_create_keyval");
    if (fn.MPI_Win_create_keyval == NULL) fn.MPI_Win_create_keyval = (int (*)(MPI_Win_copy_attr_function *, MPI_Win_delete_attr_function *, int *, void *))GetProcAddress(hPMPIModule, "MPI_Win_create_keyval");
    fn.MPI_Win_delete_attr = (int (*)(MPI_Win, int))GetProcAddress(hMPIModule, "MPI_Win_delete_attr");
    if (fn.MPI_Win_delete_attr == NULL) fn.MPI_Win_delete_attr = (int (*)(MPI_Win, int))GetProcAddress(hPMPIModule, "MPI_Win_delete_attr");
    fn.MPI_Win_free_keyval = (int (*)(int *))GetProcAddress(hMPIModule, "MPI_Win_free_keyval");
    if (fn.MPI_Win_free_keyval == NULL) fn.MPI_Win_free_keyval = (int (*)(int *))GetProcAddress(hPMPIModule, "MPI_Win_free_keyval");
    fn.MPI_Win_get_attr = (int (*)(MPI_Win, int, void *, int *))GetProcAddress(hMPIModule, "MPI_Win_get_attr");
    if (fn.MPI_Win_get_attr == NULL) fn.MPI_Win_get_attr = (int (*)(MPI_Win, int, void *, int *))GetProcAddress(hPMPIModule, "MPI_Win_get_attr");
    fn.MPI_Win_get_name = (int (*)(MPI_Win, char *, int *))GetProcAddress(hMPIModule, "MPI_Win_get_name");
    if (fn.MPI_Win_get_name == NULL) fn.MPI_Win_get_name = (int (*)(MPI_Win, char *, int *))GetProcAddress(hPMPIModule, "MPI_Win_get_name");
    fn.MPI_Win_set_attr = (int (*)(MPI_Win, int, void *))GetProcAddress(hMPIModule, "MPI_Win_set_attr");
    if (fn.MPI_Win_set_attr == NULL) fn.MPI_Win_set_attr = (int (*)(MPI_Win, int, void *))GetProcAddress(hPMPIModule, "MPI_Win_set_attr");
    fn.MPI_Win_set_name = (int (*)(MPI_Win, char *))GetProcAddress(hMPIModule, "MPI_Win_set_name");
    if (fn.MPI_Win_set_name == NULL) fn.MPI_Win_set_name = (int (*)(MPI_Win, char *))GetProcAddress(hPMPIModule, "MPI_Win_set_name");
    fn.MPI_Alloc_mem = (int (*)(MPI_Aint, MPI_Info info, void *baseptr))GetProcAddress(hMPIModule, "MPI_Alloc_mem");
    if (fn.MPI_Alloc_mem == NULL) fn.MPI_Alloc_mem = (int (*)(MPI_Aint, MPI_Info info, void *baseptr))GetProcAddress(hPMPIModule, "MPI_Alloc_mem");
    fn.MPI_Comm_create_errhandler = (int (*)(MPI_Comm_errhandler_fn *, MPI_Errhandler *))GetProcAddress(hMPIModule, "MPI_Comm_create_errhandler");
    if (fn.MPI_Comm_create_errhandler == NULL) fn.MPI_Comm_create_errhandler = (int (*)(MPI_Comm_errhandler_fn *, MPI_Errhandler *))GetProcAddress(hPMPIModule, "MPI_Comm_create_errhandler");
    fn.MPI_Comm_get_errhandler = (int (*)(MPI_Comm, MPI_Errhandler *))GetProcAddress(hMPIModule, "MPI_Comm_get_errhandler");
    if (fn.MPI_Comm_get_errhandler == NULL) fn.MPI_Comm_get_errhandler = (int (*)(MPI_Comm, MPI_Errhandler *))GetProcAddress(hPMPIModule, "MPI_Comm_get_errhandler");
    fn.MPI_Comm_set_errhandler = (int (*)(MPI_Comm, MPI_Errhandler))GetProcAddress(hMPIModule, "MPI_Comm_set_errhandler");
    if (fn.MPI_Comm_set_errhandler == NULL) fn.MPI_Comm_set_errhandler = (int (*)(MPI_Comm, MPI_Errhandler))GetProcAddress(hPMPIModule, "MPI_Comm_set_errhandler");
    fn.MPI_File_create_errhandler = (int (*)(MPI_File_errhandler_fn *, MPI_Errhandler *))GetProcAddress(hMPIModule, "MPI_File_create_errhandler");
    if (fn.MPI_File_create_errhandler == NULL) fn.MPI_File_create_errhandler = (int (*)(MPI_File_errhandler_fn *, MPI_Errhandler *))GetProcAddress(hPMPIModule, "MPI_File_create_errhandler");
    fn.MPI_File_get_errhandler = (int (*)(MPI_File, MPI_Errhandler *))GetProcAddress(hMPIModule, "MPI_File_get_errhandler");
    if (fn.MPI_File_get_errhandler == NULL) fn.MPI_File_get_errhandler = (int (*)(MPI_File, MPI_Errhandler *))GetProcAddress(hPMPIModule, "MPI_File_get_errhandler");
    fn.MPI_File_set_errhandler = (int (*)(MPI_File, MPI_Errhandler))GetProcAddress(hMPIModule, "MPI_File_set_errhandler");
    if (fn.MPI_File_set_errhandler == NULL) fn.MPI_File_set_errhandler = (int (*)(MPI_File, MPI_Errhandler))GetProcAddress(hPMPIModule, "MPI_File_set_errhandler");
    fn.MPI_Finalized = (int (*)(int *))GetProcAddress(hMPIModule, "MPI_Finalized");
    if (fn.MPI_Finalized == NULL) fn.MPI_Finalized = (int (*)(int *))GetProcAddress(hPMPIModule, "MPI_Finalized");
    fn.MPI_Free_mem = (int (*)(void *))GetProcAddress(hMPIModule, "MPI_Free_mem");
    if (fn.MPI_Free_mem == NULL) fn.MPI_Free_mem = (int (*)(void *))GetProcAddress(hPMPIModule, "MPI_Free_mem");
    fn.MPI_Get_address = (int (*)(void *, MPI_Aint *))GetProcAddress(hMPIModule, "MPI_Get_address");
    if (fn.MPI_Get_address == NULL) fn.MPI_Get_address = (int (*)(void *, MPI_Aint *))GetProcAddress(hPMPIModule, "MPI_Get_address");
    fn.MPI_Info_create = (int (*)(MPI_Info *))GetProcAddress(hMPIModule, "MPI_Info_create");
    if (fn.MPI_Info_create == NULL) fn.MPI_Info_create = (int (*)(MPI_Info *))GetProcAddress(hPMPIModule, "MPI_Info_create");
    fn.MPI_Info_delete = (int (*)(MPI_Info, char *))GetProcAddress(hMPIModule, "MPI_Info_delete");
    if (fn.MPI_Info_delete == NULL) fn.MPI_Info_delete = (int (*)(MPI_Info, char *))GetProcAddress(hPMPIModule, "MPI_Info_delete");
    fn.MPI_Info_dup = (int (*)(MPI_Info, MPI_Info *))GetProcAddress(hMPIModule, "MPI_Info_dup");
    if (fn.MPI_Info_dup == NULL) fn.MPI_Info_dup = (int (*)(MPI_Info, MPI_Info *))GetProcAddress(hPMPIModule, "MPI_Info_dup");
    fn.MPI_Info_free = (int (*)(MPI_Info *info))GetProcAddress(hMPIModule, "MPI_Info_free");
    if (fn.MPI_Info_free == NULL) fn.MPI_Info_free = (int (*)(MPI_Info *info))GetProcAddress(hPMPIModule, "MPI_Info_free");
    fn.MPI_Info_get = (int (*)(MPI_Info, char *, int, char *, int *))GetProcAddress(hMPIModule, "MPI_Info_get");
    if (fn.MPI_Info_get == NULL) fn.MPI_Info_get = (int (*)(MPI_Info, char *, int, char *, int *))GetProcAddress(hPMPIModule, "MPI_Info_get");
    fn.MPI_Info_get_nkeys = (int (*)(MPI_Info, int *))GetProcAddress(hMPIModule, "MPI_Info_get_nkeys");
    if (fn.MPI_Info_get_nkeys == NULL) fn.MPI_Info_get_nkeys = (int (*)(MPI_Info, int *))GetProcAddress(hPMPIModule, "MPI_Info_get_nkeys");
    fn.MPI_Info_get_nthkey = (int (*)(MPI_Info, int, char *))GetProcAddress(hMPIModule, "MPI_Info_get_nthkey");
    if (fn.MPI_Info_get_nthkey == NULL) fn.MPI_Info_get_nthkey = (int (*)(MPI_Info, int, char *))GetProcAddress(hPMPIModule, "MPI_Info_get_nthkey");
    fn.MPI_Info_get_valuelen = (int (*)(MPI_Info, char *, int *, int *))GetProcAddress(hMPIModule, "MPI_Info_get_valuelen");
    if (fn.MPI_Info_get_valuelen == NULL) fn.MPI_Info_get_valuelen = (int (*)(MPI_Info, char *, int *, int *))GetProcAddress(hPMPIModule, "MPI_Info_get_valuelen");
    fn.MPI_Info_set = (int (*)(MPI_Info, char *, char *))GetProcAddress(hMPIModule, "MPI_Info_set");
    if (fn.MPI_Info_set == NULL) fn.MPI_Info_set = (int (*)(MPI_Info, char *, char *))GetProcAddress(hPMPIModule, "MPI_Info_set");
    fn.MPI_Pack_external = (int (*)(char *, void *, int, MPI_Datatype, void *, MPI_Aint, MPI_Aint *))GetProcAddress(hMPIModule, "MPI_Pack_external"); 
    if (fn.MPI_Pack_external == NULL) fn.MPI_Pack_external = (int (*)(char *, void *, int, MPI_Datatype, void *, MPI_Aint, MPI_Aint *))GetProcAddress(hPMPIModule, "MPI_Pack_external"); 
    fn.MPI_Pack_external_size = (int (*)(char *, int, MPI_Datatype, MPI_Aint *))GetProcAddress(hMPIModule, "MPI_Pack_external_size"); 
    if (fn.MPI_Pack_external_size == NULL) fn.MPI_Pack_external_size = (int (*)(char *, int, MPI_Datatype, MPI_Aint *))GetProcAddress(hPMPIModule, "MPI_Pack_external_size"); 
    fn.MPI_Request_get_status = (int (*)(MPI_Request, int *, MPI_Status *))GetProcAddress(hMPIModule, "MPI_Request_get_status");
    if (fn.MPI_Request_get_status == NULL) fn.MPI_Request_get_status = (int (*)(MPI_Request, int *, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_Request_get_status");
    fn.MPI_Status_c2f = (int (*)(MPI_Status *, MPI_Fint *))GetProcAddress(hMPIModule, "MPI_Status_c2f");
    if (fn.MPI_Status_c2f == NULL) fn.MPI_Status_c2f = (int (*)(MPI_Status *, MPI_Fint *))GetProcAddress(hPMPIModule, "MPI_Status_c2f");
    fn.MPI_Status_f2c = (int (*)(MPI_Fint *, MPI_Status *))GetProcAddress(hMPIModule, "MPI_Status_f2c");
    if (fn.MPI_Status_f2c == NULL) fn.MPI_Status_f2c = (int (*)(MPI_Fint *, MPI_Status *))GetProcAddress(hPMPIModule, "MPI_Status_f2c");
    fn.MPI_Type_create_darray = (int (*)(int, int, int, int [], int [], int [], int [], int, MPI_Datatype, MPI_Datatype *))GetProcAddress(hMPIModule, "MPI_Type_create_darray");
    if (fn.MPI_Type_create_darray == NULL) fn.MPI_Type_create_darray = (int (*)(int, int, int, int [], int [], int [], int [], int, MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "MPI_Type_create_darray");
    fn.MPI_Type_create_hindexed = (int (*)(int, int [], MPI_Aint [], MPI_Datatype, MPI_Datatype *))GetProcAddress(hMPIModule, "MPI_Type_create_hindexed");
    if (fn.MPI_Type_create_hindexed == NULL) fn.MPI_Type_create_hindexed = (int (*)(int, int [], MPI_Aint [], MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "MPI_Type_create_hindexed");
    fn.MPI_Type_create_hvector = (int (*)(int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *))GetProcAddress(hMPIModule, "MPI_Type_create_hvector");
    if (fn.MPI_Type_create_hvector == NULL) fn.MPI_Type_create_hvector = (int (*)(int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "MPI_Type_create_hvector");
    fn.MPI_Type_create_indexed_block = (int (*)(int, int, int [], MPI_Datatype, MPI_Datatype *))GetProcAddress(hMPIModule, "MPI_Type_create_indexed_block");
    if (fn.MPI_Type_create_indexed_block == NULL) fn.MPI_Type_create_indexed_block = (int (*)(int, int, int [], MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "MPI_Type_create_indexed_block");
    fn.MPI_Type_create_resized = (int (*)(MPI_Datatype, MPI_Aint, MPI_Aint, MPI_Datatype *))GetProcAddress(hMPIModule, "MPI_Type_create_resized");
    if (fn.MPI_Type_create_resized == NULL) fn.MPI_Type_create_resized = (int (*)(MPI_Datatype, MPI_Aint, MPI_Aint, MPI_Datatype *))GetProcAddress(hPMPIModule, "MPI_Type_create_resized");
    fn.MPI_Type_create_struct = (int (*)(int, int [], MPI_Aint [], MPI_Datatype [], MPI_Datatype *))GetProcAddress(hMPIModule, "MPI_Type_create_struct");
    if (fn.MPI_Type_create_struct == NULL) fn.MPI_Type_create_struct = (int (*)(int, int [], MPI_Aint [], MPI_Datatype [], MPI_Datatype *))GetProcAddress(hPMPIModule, "MPI_Type_create_struct");
    fn.MPI_Type_create_subarray = (int (*)(int, int [], int [], int [], int, MPI_Datatype, MPI_Datatype *))GetProcAddress(hMPIModule, "MPI_Type_create_subarray");
    if (fn.MPI_Type_create_subarray == NULL) fn.MPI_Type_create_subarray = (int (*)(int, int [], int [], int [], int, MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "MPI_Type_create_subarray");
    fn.MPI_Type_get_extent = (int (*)(MPI_Datatype, MPI_Aint *, MPI_Aint *))GetProcAddress(hMPIModule, "MPI_Type_get_extent");
    if (fn.MPI_Type_get_extent == NULL) fn.MPI_Type_get_extent = (int (*)(MPI_Datatype, MPI_Aint *, MPI_Aint *))GetProcAddress(hPMPIModule, "MPI_Type_get_extent");
    fn.MPI_Type_get_true_extent = (int (*)(MPI_Datatype, MPI_Aint *, MPI_Aint *))GetProcAddress(hMPIModule, "MPI_Type_get_true_extent");
    if (fn.MPI_Type_get_true_extent == NULL) fn.MPI_Type_get_true_extent = (int (*)(MPI_Datatype, MPI_Aint *, MPI_Aint *))GetProcAddress(hPMPIModule, "MPI_Type_get_true_extent");
    fn.MPI_Unpack_external = (int (*)(char *, void *, MPI_Aint, MPI_Aint *, void *, int, MPI_Datatype))GetProcAddress(hMPIModule, "MPI_Unpack_external"); 
    if (fn.MPI_Unpack_external == NULL) fn.MPI_Unpack_external = (int (*)(char *, void *, MPI_Aint, MPI_Aint *, void *, int, MPI_Datatype))GetProcAddress(hPMPIModule, "MPI_Unpack_external"); 
    fn.MPI_Win_create_errhandler = (int (*)(MPI_Win_errhandler_fn *, MPI_Errhandler *))GetProcAddress(hMPIModule, "MPI_Win_create_errhandler");
    if (fn.MPI_Win_create_errhandler == NULL) fn.MPI_Win_create_errhandler = (int (*)(MPI_Win_errhandler_fn *, MPI_Errhandler *))GetProcAddress(hPMPIModule, "MPI_Win_create_errhandler");
    fn.MPI_Win_get_errhandler = (int (*)(MPI_Win, MPI_Errhandler *))GetProcAddress(hMPIModule, "MPI_Win_get_errhandler");
    if (fn.MPI_Win_get_errhandler == NULL) fn.MPI_Win_get_errhandler = (int (*)(MPI_Win, MPI_Errhandler *))GetProcAddress(hPMPIModule, "MPI_Win_get_errhandler");
    fn.MPI_Win_set_errhandler = (int (*)(MPI_Win, MPI_Errhandler))GetProcAddress(hMPIModule, "MPI_Win_set_errhandler");
    if (fn.MPI_Win_set_errhandler == NULL) fn.MPI_Win_set_errhandler = (int (*)(MPI_Win, MPI_Errhandler))GetProcAddress(hPMPIModule, "MPI_Win_set_errhandler");
    fn.MPI_Type_create_f90_integer = (int (*)( int, MPI_Datatype * ))GetProcAddress(hMPIModule, "MPI_Type_create_f90_integer");
    if (fn.MPI_Type_create_f90_integer == NULL) fn.MPI_Type_create_f90_integer = (int (*)( int, MPI_Datatype * ))GetProcAddress(hPMPIModule, "MPI_Type_create_f90_integer");
    fn.MPI_Type_create_f90_real = (int (*)( int, int, MPI_Datatype * ))GetProcAddress(hMPIModule, "MPI_Type_create_f90_real");
    if (fn.MPI_Type_create_f90_real == NULL) fn.MPI_Type_create_f90_real = (int (*)( int, int, MPI_Datatype * ))GetProcAddress(hPMPIModule, "MPI_Type_create_f90_real");
    fn.MPI_Type_create_f90_complex = (int (*)( int, int, MPI_Datatype * ))GetProcAddress(hMPIModule, "MPI_Type_create_f90_complex");
    if (fn.MPI_Type_create_f90_complex == NULL) fn.MPI_Type_create_f90_complex = (int (*)( int, int, MPI_Datatype * ))GetProcAddress(hPMPIModule, "MPI_Type_create_f90_complex");
    /* PMPI */
    fn.PMPI_Comm_f2c = (MPI_Comm (*)(MPI_Fint))GetProcAddress(hPMPIModule, "PMPI_Comm_f2c");
    fn.PMPI_Type_f2c = (MPI_Datatype (*)(MPI_Fint))GetProcAddress(hPMPIModule, "PMPI_Type_f2c");
    fn.PMPI_File_f2c = (MPI_File (*)(MPI_Fint))GetProcAddress(hPMPIModule, "PMPI_File_f2c");
    fn.PMPI_Comm_c2f = (MPI_Fint (*)(MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Comm_c2f");
    fn.PMPI_File_c2f = (MPI_Fint (*)(MPI_File))GetProcAddress(hPMPIModule, "PMPI_File_c2f");
    fn.PMPI_Group_c2f = (MPI_Fint (*)(MPI_Group))GetProcAddress(hPMPIModule, "PMPI_Group_c2f");
    fn.PMPI_Info_c2f = (MPI_Fint (*)(MPI_Info))GetProcAddress(hPMPIModule, "PMPI_Info_c2f");
    fn.PMPI_Op_c2f = (MPI_Fint (*)(MPI_Op))GetProcAddress(hPMPIModule, "PMPI_Op_c2f");
    fn.PMPI_Request_c2f = (MPI_Fint (*)(MPI_Request))GetProcAddress(hPMPIModule, "PMPI_Request_c2f");
    fn.PMPI_Type_c2f = (MPI_Fint (*)(MPI_Datatype))GetProcAddress(hPMPIModule, "PMPI_Type_c2f");
    fn.PMPI_Win_c2f = (MPI_Fint (*)(MPI_Win))GetProcAddress(hPMPIModule, "PMPI_Win_c2f");
    fn.PMPI_Group_f2c = (MPI_Group (*)(MPI_Fint))GetProcAddress(hPMPIModule, "PMPI_Group_f2c");
    fn.PMPI_Info_f2c = (MPI_Info (*)(MPI_Fint))GetProcAddress(hPMPIModule, "PMPI_Info_f2c");
    fn.PMPI_Op_f2c = (MPI_Op (*)(MPI_Fint))GetProcAddress(hPMPIModule, "PMPI_Op_f2c");
    fn.PMPI_Request_f2c = (MPI_Request (*)(MPI_Fint))GetProcAddress(hPMPIModule, "PMPI_Request_f2c");
    fn.PMPI_Win_f2c = (MPI_Win (*)(MPI_Fint))GetProcAddress(hPMPIModule, "PMPI_Win_f2c");
    fn.PMPI_File_open = (int (*)(MPI_Comm, char *, int, MPI_Info, MPI_File *))GetProcAddress(hPMPIModule, "PMPI_File_open");
    fn.PMPI_File_close = (int (*)(MPI_File *))GetProcAddress(hPMPIModule, "PMPI_File_close");
    fn.PMPI_File_delete = (int (*)(char *, MPI_Info))GetProcAddress(hPMPIModule, "PMPI_File_delete");
    fn.PMPI_File_set_size = (int (*)(MPI_File, MPI_Offset))GetProcAddress(hPMPIModule, "PMPI_File_set_size");
    fn.PMPI_File_preallocate = (int (*)(MPI_File, MPI_Offset))GetProcAddress(hPMPIModule, "PMPI_File_preallocate");
    fn.PMPI_File_get_size = (int (*)(MPI_File, MPI_Offset *))GetProcAddress(hPMPIModule, "PMPI_File_get_size");
    fn.PMPI_File_get_group = (int (*)(MPI_File, MPI_Group *))GetProcAddress(hPMPIModule, "PMPI_File_get_group");
    fn.PMPI_File_get_amode = (int (*)(MPI_File, int *))GetProcAddress(hPMPIModule, "PMPI_File_get_amode");
    fn.PMPI_File_set_info = (int (*)(MPI_File, MPI_Info))GetProcAddress(hPMPIModule, "PMPI_File_set_info");
    fn.PMPI_File_get_info = (int (*)(MPI_File, MPI_Info *))GetProcAddress(hPMPIModule, "PMPI_File_get_info");
    fn.PMPI_File_set_view = (int (*)(MPI_File, MPI_Offset, MPI_Datatype, MPI_Datatype, char *, MPI_Info))GetProcAddress(hPMPIModule, "PMPI_File_set_view");
    fn.PMPI_File_get_view = (int (*)(MPI_File, MPI_Offset *, MPI_Datatype *, MPI_Datatype *, char *))GetProcAddress(hPMPIModule, "PMPI_File_get_view");
    fn.PMPI_File_read_at = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_read_at");
    fn.PMPI_File_read_at_all = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_read_at_all");
    fn.PMPI_File_write_at = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_write_at");
    fn.PMPI_File_write_at_all = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_write_at_all");
    fn.PMPI_File_iread_at = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hPMPIModule, "PMPI_File_iread_at");
    fn.PMPI_File_iwrite_at = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hPMPIModule, "PMPI_File_iwrite_at");
    fn.PMPI_File_read = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_read"); 
    fn.PMPI_File_read_all = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_read_all"); 
    fn.PMPI_File_write = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_write");
    fn.PMPI_File_write_all = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_write_all");
    fn.PMPI_File_iread = (int (*)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hPMPIModule, "PMPI_File_iread"); 
    fn.PMPI_File_iwrite = (int (*)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hPMPIModule, "PMPI_File_iwrite");
    fn.PMPI_File_seek = (int (*)(MPI_File, MPI_Offset, int))GetProcAddress(hPMPIModule, "PMPI_File_seek");
    fn.PMPI_File_get_position = (int (*)(MPI_File, MPI_Offset *))GetProcAddress(hPMPIModule, "PMPI_File_get_position");
    fn.PMPI_File_get_byte_offset = (int (*)(MPI_File, MPI_Offset, MPI_Offset *))GetProcAddress(hPMPIModule, "PMPI_File_get_byte_offset");
    fn.PMPI_File_read_shared = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_read_shared");
    fn.PMPI_File_write_shared = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_write_shared");
    fn.PMPI_File_iread_shared = (int (*)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hPMPIModule, "PMPI_File_iread_shared");
    fn.PMPI_File_iwrite_shared = (int (*)(MPI_File, void *, int, MPI_Datatype, MPIO_Request *))GetProcAddress(hPMPIModule, "PMPI_File_iwrite_shared");
    fn.PMPI_File_read_ordered = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_read_ordered");
    fn.PMPI_File_write_ordered = (int (*)(MPI_File, void *, int, MPI_Datatype, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_write_ordered");
    fn.PMPI_File_seek_shared = (int (*)(MPI_File, MPI_Offset, int))GetProcAddress(hPMPIModule, "PMPI_File_seek_shared");
    fn.PMPI_File_get_position_shared = (int (*)(MPI_File, MPI_Offset *))GetProcAddress(hPMPIModule, "PMPI_File_get_position_shared");
    fn.PMPI_File_read_at_all_begin = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype))GetProcAddress(hPMPIModule, "PMPI_File_read_at_all_begin");
    fn.PMPI_File_read_at_all_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_read_at_all_end");
    fn.PMPI_File_write_at_all_begin = (int (*)(MPI_File, MPI_Offset, void *, int, MPI_Datatype))GetProcAddress(hPMPIModule, "PMPI_File_write_at_all_begin");
    fn.PMPI_File_write_at_all_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_write_at_all_end");
    fn.PMPI_File_read_all_begin = (int (*)(MPI_File, void *, int, MPI_Datatype))GetProcAddress(hPMPIModule, "PMPI_File_read_all_begin");
    fn.PMPI_File_read_all_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_read_all_end");
    fn.PMPI_File_write_all_begin = (int (*)(MPI_File, void *, int, MPI_Datatype))GetProcAddress(hPMPIModule, "PMPI_File_write_all_begin");
    fn.PMPI_File_write_all_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_write_all_end");
    fn.PMPI_File_read_ordered_begin = (int (*)(MPI_File, void *, int, MPI_Datatype))GetProcAddress(hPMPIModule, "PMPI_File_read_ordered_begin");
    fn.PMPI_File_read_ordered_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_read_ordered_end");
    fn.PMPI_File_write_ordered_begin = (int (*)(MPI_File, void *, int, MPI_Datatype))GetProcAddress(hPMPIModule, "PMPI_File_write_ordered_begin");
    fn.PMPI_File_write_ordered_end = (int (*)(MPI_File, void *, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_File_write_ordered_end");
    fn.PMPI_File_get_type_extent = (int (*)(MPI_File, MPI_Datatype, MPI_Aint *))GetProcAddress(hPMPIModule, "PMPI_File_get_type_extent");
    fn.PMPI_Register_datarep = (int (*)(char *, MPI_Datarep_conversion_function *, MPI_Datarep_conversion_function *, MPI_Datarep_extent_function *, void *))GetProcAddress(hPMPIModule, "PMPI_Register_datarep");
    fn.PMPI_File_set_atomicity = (int (*)(MPI_File, int))GetProcAddress(hPMPIModule, "PMPI_File_set_atomicity");
    fn.PMPI_File_get_atomicity = (int (*)(MPI_File, int *))GetProcAddress(hPMPIModule, "PMPI_File_get_atomicity");
    fn.PMPI_File_sync = (int (*)(MPI_File))GetProcAddress(hPMPIModule, "PMPI_File_sync");
    fn.PMPI_Send = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Send");
    fn.PMPI_Recv = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_Recv");
    fn.PMPI_Get_count = (int (*)(MPI_Status *, MPI_Datatype, int *))GetProcAddress(hPMPIModule, "PMPI_Get_count");
    fn.PMPI_Bsend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Bsend");
    fn.PMPI_Ssend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Ssend");
    fn.PMPI_Rsend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Rsend");
    fn.PMPI_Buffer_attach = (int (*)( void*, int))GetProcAddress(hPMPIModule, "PMPI_Buffer_attach");
    fn.PMPI_Buffer_detach = (int (*)( void*, int *))GetProcAddress(hPMPIModule, "PMPI_Buffer_detach");
    fn.PMPI_Isend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "PMPI_Isend");
    fn.PMPI_Ibsend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "PMPI_Ibsend");
    fn.PMPI_Issend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "PMPI_Issend");
    fn.PMPI_Irsend = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "PMPI_Irsend");
    fn.PMPI_Irecv = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "PMPI_Irecv");
    fn.PMPI_Wait = (int (*)(MPI_Request *, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_Wait");
    fn.PMPI_Test = (int (*)(MPI_Request *, int *, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_Test");
    fn.PMPI_Request_free = (int (*)(MPI_Request *))GetProcAddress(hPMPIModule, "PMPI_Request_free");
    fn.PMPI_Waitany = (int (*)(int, MPI_Request *, int *, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_Waitany");
    fn.PMPI_Testany = (int (*)(int, MPI_Request *, int *, int *, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_Testany");
    fn.PMPI_Waitall = (int (*)(int, MPI_Request *, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_Waitall");
    fn.PMPI_Testall = (int (*)(int, MPI_Request *, int *, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_Testall");
    fn.PMPI_Waitsome = (int (*)(int, MPI_Request *, int *, int *, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_Waitsome");
    fn.PMPI_Testsome = (int (*)(int, MPI_Request *, int *, int *, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_Testsome");
    fn.PMPI_Iprobe = (int (*)(int, int, MPI_Comm, int *, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_Iprobe");
    fn.PMPI_Probe = (int (*)(int, int, MPI_Comm, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_Probe");
    fn.PMPI_Cancel = (int (*)(MPI_Request *))GetProcAddress(hPMPIModule, "PMPI_Cancel");
    fn.PMPI_Test_cancelled = (int (*)(MPI_Status *, int *))GetProcAddress(hPMPIModule, "PMPI_Test_cancelled");
    fn.PMPI_Send_init = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "PMPI_Send_init");
    fn.PMPI_Bsend_init = (int (*)(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "PMPI_Bsend_init");
    fn.PMPI_Ssend_init = (int (*)(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "PMPI_Ssend_init");
    fn.PMPI_Rsend_init = (int (*)(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "PMPI_Rsend_init");
    fn.PMPI_Recv_init = (int (*)(void*, int, MPI_Datatype, int,int, MPI_Comm, MPI_Request *))GetProcAddress(hPMPIModule, "PMPI_Recv_init");
    fn.PMPI_Start = (int (*)(MPI_Request *))GetProcAddress(hPMPIModule, "PMPI_Start");
    fn.PMPI_Startall = (int (*)(int, MPI_Request *))GetProcAddress(hPMPIModule, "PMPI_Startall");
    fn.PMPI_Sendrecv = (int (*)(void *, int, MPI_Datatype,int, int, void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_Sendrecv");
    fn.PMPI_Sendrecv_replace = (int (*)(void*, int, MPI_Datatype, int, int, int, int, MPI_Comm, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_Sendrecv_replace");
    fn.PMPI_Type_contiguous = (int (*)(int, MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "PMPI_Type_contiguous");
    fn.PMPI_Type_vector = (int (*)(int, int, int, MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "PMPI_Type_vector");
    fn.PMPI_Type_hvector = (int (*)(int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "PMPI_Type_hvector");
    fn.PMPI_Type_indexed = (int (*)(int, int *, int *, MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "PMPI_Type_indexed");
    fn.PMPI_Type_hindexed = (int (*)(int, int *, MPI_Aint *, MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "PMPI_Type_hindexed");
    fn.PMPI_Type_struct = (int (*)(int, int *, MPI_Aint *, MPI_Datatype *, MPI_Datatype *))GetProcAddress(hPMPIModule, "PMPI_Type_struct");
    fn.PMPI_Address = (int (*)(void*, MPI_Aint *))GetProcAddress(hPMPIModule, "PMPI_Address");
    fn.PMPI_Type_extent = (int (*)(MPI_Datatype, MPI_Aint *))GetProcAddress(hPMPIModule, "PMPI_Type_extent");
    fn.PMPI_Type_size = (int (*)(MPI_Datatype, int *))GetProcAddress(hPMPIModule, "PMPI_Type_size");
    fn.PMPI_Type_lb = (int (*)(MPI_Datatype, MPI_Aint *))GetProcAddress(hPMPIModule, "PMPI_Type_lb");
    fn.PMPI_Type_ub = (int (*)(MPI_Datatype, MPI_Aint *))GetProcAddress(hPMPIModule, "PMPI_Type_ub");
    fn.PMPI_Type_commit = (int (*)(MPI_Datatype *))GetProcAddress(hPMPIModule, "PMPI_Type_commit");
    fn.PMPI_Type_free = (int (*)(MPI_Datatype *))GetProcAddress(hPMPIModule, "PMPI_Type_free");
    fn.PMPI_Get_elements = (int (*)(MPI_Status *, MPI_Datatype, int *))GetProcAddress(hPMPIModule, "PMPI_Get_elements");
    fn.PMPI_Pack = (int (*)(void*, int, MPI_Datatype, void *, int, int *,  MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Pack");
    fn.PMPI_Unpack = (int (*)(void*, int, int *, void *, int, MPI_Datatype, MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Unpack");
    fn.PMPI_Pack_size = (int (*)(int, MPI_Datatype, MPI_Comm, int *))GetProcAddress(hPMPIModule, "PMPI_Pack_size");
    fn.PMPI_Barrier = (int (*)(MPI_Comm ))GetProcAddress(hPMPIModule, "PMPI_Barrier");
    fn.PMPI_Bcast = (int (*)(void*, int, MPI_Datatype, int, MPI_Comm ))GetProcAddress(hPMPIModule, "PMPI_Bcast");
    fn.PMPI_Gather = (int (*)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Gather"); 
    fn.PMPI_Gatherv = (int (*)(void* , int, MPI_Datatype, void*, int *, int *, MPI_Datatype, int, MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Gatherv"); 
    fn.PMPI_Scatter = (int (*)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Scatter");
    fn.PMPI_Scatterv = (int (*)(void* , int *, int *,  MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Scatterv");
    fn.PMPI_Allgather = (int (*)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Allgather");
    fn.PMPI_Allgatherv = (int (*)(void* , int, MPI_Datatype, void*, int *, int *, MPI_Datatype, MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Allgatherv");
    fn.PMPI_Alltoall = (int (*)(void* , int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Alltoall");
    fn.PMPI_Alltoallv = (int (*)(void* , int *, int *, MPI_Datatype, void*, int *, int *, MPI_Datatype, MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Alltoallv");
    fn.PMPI_Reduce = (int (*)(void* , void*, int, MPI_Datatype, MPI_Op, int, MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Reduce");
    fn.PMPI_Op_create = (int (*)(MPI_User_function *, int, MPI_Op *))GetProcAddress(hPMPIModule, "PMPI_Op_create");
    fn.PMPI_Op_free = (int (*)( MPI_Op *))GetProcAddress(hPMPIModule, "PMPI_Op_free");
    fn.PMPI_Allreduce = (int (*)(void* , void*, int, MPI_Datatype, MPI_Op, MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Allreduce");
    fn.PMPI_Reduce_scatter = (int (*)(void* , void*, int *, MPI_Datatype, MPI_Op, MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Reduce_scatter");
    fn.PMPI_Scan = (int (*)(void* , void*, int, MPI_Datatype, MPI_Op, MPI_Comm ))GetProcAddress(hPMPIModule, "PMPI_Scan");
    fn.PMPI_Group_size = (int (*)(MPI_Group, int *))GetProcAddress(hPMPIModule, "PMPI_Group_size");
    fn.PMPI_Group_rank = (int (*)(MPI_Group, int *))GetProcAddress(hPMPIModule, "PMPI_Group_rank");
    fn.PMPI_Group_translate_ranks = (int (* )(MPI_Group, int, int *, MPI_Group, int *))GetProcAddress(hPMPIModule, "PMPI_Group_translate_ranks");
    fn.PMPI_Group_compare = (int (*)(MPI_Group, MPI_Group, int *))GetProcAddress(hPMPIModule, "PMPI_Group_compare");
    fn.PMPI_Comm_group = (int (*)(MPI_Comm, MPI_Group *))GetProcAddress(hPMPIModule, "PMPI_Comm_group");
    fn.PMPI_Group_union = (int (*)(MPI_Group, MPI_Group, MPI_Group *))GetProcAddress(hPMPIModule, "PMPI_Group_union");
    fn.PMPI_Group_intersection = (int (*)(MPI_Group, MPI_Group, MPI_Group *))GetProcAddress(hPMPIModule, "PMPI_Group_intersection");
    fn.PMPI_Group_difference = (int (*)(MPI_Group, MPI_Group, MPI_Group *))GetProcAddress(hPMPIModule, "PMPI_Group_difference");
    fn.PMPI_Group_incl = (int (*)(MPI_Group, int, int *, MPI_Group *))GetProcAddress(hPMPIModule, "PMPI_Group_incl");
    fn.PMPI_Group_excl = (int (*)(MPI_Group, int, int *, MPI_Group *))GetProcAddress(hPMPIModule, "PMPI_Group_excl");
    fn.PMPI_Group_range_incl = (int (*)(MPI_Group, int, int [][3], MPI_Group *))GetProcAddress(hPMPIModule, "PMPI_Group_range_incl");
    fn.PMPI_Group_range_excl = (int (*)(MPI_Group, int, int [][3], MPI_Group *))GetProcAddress(hPMPIModule, "PMPI_Group_range_excl");
    fn.PMPI_Group_free = (int (*)(MPI_Group *))GetProcAddress(hPMPIModule, "PMPI_Group_free");
    fn.PMPI_Comm_size = (int (*)(MPI_Comm, int *))GetProcAddress(hPMPIModule, "PMPI_Comm_size");
    fn.PMPI_Comm_rank = (int (*)(MPI_Comm, int *))GetProcAddress(hPMPIModule, "PMPI_Comm_rank");
    fn.PMPI_Comm_compare = (int (*)(MPI_Comm, MPI_Comm, int *))GetProcAddress(hPMPIModule, "PMPI_Comm_compare");
    fn.PMPI_Comm_dup = (int (*)(MPI_Comm, MPI_Comm *))GetProcAddress(hPMPIModule, "PMPI_Comm_dup");
    fn.PMPI_Comm_create = (int (*)(MPI_Comm, MPI_Group, MPI_Comm *))GetProcAddress(hPMPIModule, "PMPI_Comm_create");
    fn.PMPI_Comm_split = (int (*)(MPI_Comm, int, int, MPI_Comm *))GetProcAddress(hPMPIModule, "PMPI_Comm_split");
    fn.PMPI_Comm_free = (int (*)(MPI_Comm *))GetProcAddress(hPMPIModule, "PMPI_Comm_free");
    fn.PMPI_Comm_test_inter = (int (*)(MPI_Comm, int *))GetProcAddress(hPMPIModule, "PMPI_Comm_test_inter");
    fn.PMPI_Comm_remote_size = (int (*)(MPI_Comm, int *))GetProcAddress(hPMPIModule, "PMPI_Comm_remote_size");
    fn.PMPI_Comm_remote_group = (int (*)(MPI_Comm, MPI_Group *))GetProcAddress(hPMPIModule, "PMPI_Comm_remote_group");
    fn.PMPI_Intercomm_create = (int (*)(MPI_Comm, int, MPI_Comm, int, int, MPI_Comm * ))GetProcAddress(hPMPIModule, "PMPI_Intercomm_create");
    fn.PMPI_Intercomm_merge = (int (*)(MPI_Comm, int, MPI_Comm *))GetProcAddress(hPMPIModule, "PMPI_Intercomm_merge");
    fn.PMPI_Keyval_create = (int (*)(MPI_Copy_function *, MPI_Delete_function *, int *, void*))GetProcAddress(hPMPIModule, "PMPI_Keyval_create");
    fn.PMPI_Keyval_free = (int (*)(int *))GetProcAddress(hPMPIModule, "PMPI_Keyval_free");
    fn.PMPI_Attr_put = (int (*)(MPI_Comm, int, void*))GetProcAddress(hPMPIModule, "PMPI_Attr_put");
    fn.PMPI_Attr_get = (int (*)(MPI_Comm, int, void *, int *))GetProcAddress(hPMPIModule, "PMPI_Attr_get");
    fn.PMPI_Attr_delete = (int (*)(MPI_Comm, int))GetProcAddress(hPMPIModule, "PMPI_Attr_delete");
    fn.PMPI_Topo_test = (int (*)(MPI_Comm, int *))GetProcAddress(hPMPIModule, "PMPI_Topo_test");
    fn.PMPI_Cart_create = (int (*)(MPI_Comm, int, int *, int *, int, MPI_Comm *))GetProcAddress(hPMPIModule, "PMPI_Cart_create");
    fn.PMPI_Dims_create = (int (*)(int, int, int *))GetProcAddress(hPMPIModule, "PMPI_Dims_create");
    fn.PMPI_Graph_create = (int (*)(MPI_Comm, int, int *, int *, int, MPI_Comm *))GetProcAddress(hPMPIModule, "PMPI_Graph_create");
    fn.PMPI_Graphdims_get = (int (*)(MPI_Comm, int *, int *))GetProcAddress(hPMPIModule, "PMPI_Graphdims_get");
    fn.PMPI_Graph_get = (int (*)(MPI_Comm, int, int, int *, int *))GetProcAddress(hPMPIModule, "PMPI_Graph_get");
    fn.PMPI_Cartdim_get = (int (*)(MPI_Comm, int *))GetProcAddress(hPMPIModule, "PMPI_Cartdim_get");
    fn.PMPI_Cart_get = (int (*)(MPI_Comm, int, int *, int *, int *))GetProcAddress(hPMPIModule, "PMPI_Cart_get");
    fn.PMPI_Cart_rank = (int (*)(MPI_Comm, int *, int *))GetProcAddress(hPMPIModule, "PMPI_Cart_rank");
    fn.PMPI_Cart_coords = (int (*)(MPI_Comm, int, int, int *))GetProcAddress(hPMPIModule, "PMPI_Cart_coords");
    fn.PMPI_Graph_neighbors_count = (int (*)(MPI_Comm, int, int *))GetProcAddress(hPMPIModule, "PMPI_Graph_neighbors_count");
    fn.PMPI_Graph_neighbors = (int (*)(MPI_Comm, int, int, int *))GetProcAddress(hPMPIModule, "PMPI_Graph_neighbors");
    fn.PMPI_Cart_shift = (int (*)(MPI_Comm, int, int, int *, int *))GetProcAddress(hPMPIModule, "PMPI_Cart_shift");
    fn.PMPI_Cart_sub = (int (*)(MPI_Comm, int *, MPI_Comm *))GetProcAddress(hPMPIModule, "PMPI_Cart_sub");
    fn.PMPI_Cart_map = (int (*)(MPI_Comm, int, int *, int *, int *))GetProcAddress(hPMPIModule, "PMPI_Cart_map");
    fn.PMPI_Graph_map = (int (*)(MPI_Comm, int, int *, int *, int *))GetProcAddress(hPMPIModule, "PMPI_Graph_map");
    fn.PMPI_Get_processor_name = (int (*)(char *, int *))GetProcAddress(hPMPIModule, "PMPI_Get_processor_name");
    fn.PMPI_Get_version = (int (*)(int *, int *))GetProcAddress(hPMPIModule, "PMPI_Get_version");
    fn.PMPI_Errhandler_create = (int (*)(MPI_Handler_function *, MPI_Errhandler *))GetProcAddress(hPMPIModule, "PMPI_Errhandler_create");
    fn.PMPI_Errhandler_set = (int (*)(MPI_Comm, MPI_Errhandler))GetProcAddress(hPMPIModule, "PMPI_Errhandler_set");
    fn.PMPI_Errhandler_get = (int (*)(MPI_Comm, MPI_Errhandler *))GetProcAddress(hPMPIModule, "PMPI_Errhandler_get");
    fn.PMPI_Errhandler_free = (int (*)(MPI_Errhandler *))GetProcAddress(hPMPIModule, "PMPI_Errhandler_free");
    fn.PMPI_Error_string = (int (*)(int, char *, int *))GetProcAddress(hPMPIModule, "PMPI_Error_string");
    fn.PMPI_Error_class = (int (*)(int, int *))GetProcAddress(hPMPIModule, "PMPI_Error_class");
    fn.PMPI_Wtime = (double (*)(void))GetProcAddress(hPMPIModule, "PMPI_Wtime");
    fn.PMPI_Wtick = (double (*)(void))GetProcAddress(hPMPIModule, "PMPI_Wtick");
    fn.PMPI_Init = (int (*)(int *, char ***))GetProcAddress(hPMPIModule, "PMPI_Init");
    fn.PMPI_Finalize = (int (*)(void))GetProcAddress(hPMPIModule, "PMPI_Finalize");
    fn.PMPI_Initialized = (int (*)(int *))GetProcAddress(hPMPIModule, "PMPI_Initialized");
    fn.PMPI_Abort = (int (*)(MPI_Comm, int))GetProcAddress(hPMPIModule, "PMPI_Abort");
    fn.PMPI_Pcontrol = (int (*)(const int, ...))GetProcAddress(hPMPIModule, "PMPI_Pcontrol");
    fn.PMPI_Close_port = (int (*)(char *))GetProcAddress(hPMPIModule, "PMPI_Close_port");
    fn.PMPI_Comm_accept = (int (*)(char *, MPI_Info, int, MPI_Comm, MPI_Comm *))GetProcAddress(hPMPIModule, "PMPI_Comm_accept");
    fn.PMPI_Comm_connect = (int (*)(char *, MPI_Info, int, MPI_Comm, MPI_Comm *))GetProcAddress(hPMPIModule, "PMPI_Comm_connect");
    fn.PMPI_Comm_disconnect = (int (*)(MPI_Comm *))GetProcAddress(hPMPIModule, "PMPI_Comm_disconnect");
    fn.PMPI_Comm_get_parent = (int (*)(MPI_Comm *))GetProcAddress(hPMPIModule, "PMPI_Comm_get_parent");
    fn.PMPI_Comm_join = (int (*)(int, MPI_Comm *))GetProcAddress(hPMPIModule, "PMPI_Comm_join");
    fn.PMPI_Comm_spawn = (int (*)(char *, char *[], int, MPI_Info, int, MPI_Comm, MPI_Comm *, int []))GetProcAddress(hPMPIModule, "PMPI_Comm_spawn");
    fn.PMPI_Comm_spawn_multiple = (int (*)(int, char *[], char **[], int [], MPI_Info [], int, MPI_Comm, MPI_Comm *, int []))GetProcAddress(hPMPIModule, "PMPI_Comm_spawn_multiple"); 
    fn.PMPI_Lookup_name = (int (*)(char *, MPI_Info, char *))GetProcAddress(hPMPIModule, "PMPI_Lookup_name");
    fn.PMPI_Open_port = (int (*)(MPI_Info, char *))GetProcAddress(hPMPIModule, "PMPI_Open_port");
    fn.PMPI_Publish_name = (int (*)(char *, MPI_Info, char *))GetProcAddress(hPMPIModule, "PMPI_Publish_name");
    fn.PMPI_Unpublish_name = (int (*)(char *, MPI_Info, char *))GetProcAddress(hPMPIModule, "PMPI_Unpublish_name");
    fn.PMPI_Accumulate = (int (*)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype,  MPI_Op, MPI_Win))GetProcAddress(hPMPIModule, "PMPI_Accumulate");
    fn.PMPI_Get = (int (*)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win))GetProcAddress(hPMPIModule, "PMPI_Get");
    fn.PMPI_Put = (int (*)(void *, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win))GetProcAddress(hPMPIModule, "PMPI_Put");
    fn.PMPI_Win_complete = (int (*)(MPI_Win))GetProcAddress(hPMPIModule, "PMPI_Win_complete");
    fn.PMPI_Win_create = (int (*)(void *, MPI_Aint, int, MPI_Info, MPI_Comm, MPI_Win *))GetProcAddress(hPMPIModule, "PMPI_Win_create");
    fn.PMPI_Win_fence = (int (*)(int, MPI_Win))GetProcAddress(hPMPIModule, "PMPI_Win_fence");
    fn.PMPI_Win_free = (int (*)(MPI_Win *))GetProcAddress(hPMPIModule, "PMPI_Win_free");
    fn.PMPI_Win_get_group = (int (*)(MPI_Win, MPI_Group *))GetProcAddress(hPMPIModule, "PMPI_Win_get_group");
    fn.PMPI_Win_lock = (int (*)(int, int, int, MPI_Win))GetProcAddress(hPMPIModule, "PMPI_Win_lock");
    fn.PMPI_Win_post = (int (*)(MPI_Group, int, MPI_Win))GetProcAddress(hPMPIModule, "PMPI_Win_post");
    fn.PMPI_Win_start = (int (*)(MPI_Group, int, MPI_Win))GetProcAddress(hPMPIModule, "PMPI_Win_start");
    fn.PMPI_Win_test = (int (*)(MPI_Win, int *))GetProcAddress(hPMPIModule, "PMPI_Win_test");
    fn.PMPI_Win_unlock = (int (*)(int, MPI_Win))GetProcAddress(hPMPIModule, "PMPI_Win_unlock");
    fn.PMPI_Win_wait = (int (*)(MPI_Win))GetProcAddress(hPMPIModule, "PMPI_Win_wait");
    fn.PMPI_Alltoallw = (int (*)(void *, int [], int [], MPI_Datatype [], void *, int [], int [], MPI_Datatype [], MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Alltoallw");
    fn.PMPI_Exscan = (int (*)(void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm))GetProcAddress(hPMPIModule, "PMPI_Exscan");
    fn.PMPI_Add_error_class = (int (*)(int *))GetProcAddress(hPMPIModule, "PMPI_Add_error_class");
    fn.PMPI_Add_error_code = (int (*)(int, int *))GetProcAddress(hPMPIModule, "PMPI_Add_error_code");
    fn.PMPI_Add_error_string = (int (*)(int, char *))GetProcAddress(hPMPIModule, "PMPI_Add_error_string");
    fn.PMPI_Comm_call_errhandler = (int (*)(MPI_Comm, int))GetProcAddress(hPMPIModule, "PMPI_Comm_call_errhandler");
    fn.PMPI_Comm_create_keyval = (int (*)(MPI_Comm_copy_attr_function *, MPI_Comm_delete_attr_function *, int *, void *))GetProcAddress(hPMPIModule, "PMPI_Comm_create_keyval");
    fn.PMPI_Comm_delete_attr = (int (*)(MPI_Comm, int))GetProcAddress(hPMPIModule, "PMPI_Comm_delete_attr");
    fn.PMPI_Comm_free_keyval = (int (*)(int *))GetProcAddress(hPMPIModule, "PMPI_Comm_free_keyval");
    fn.PMPI_Comm_get_attr = (int (*)(MPI_Comm, int, void *, int *))GetProcAddress(hPMPIModule, "PMPI_Comm_get_attr");
    fn.PMPI_Comm_get_name = (int (*)(MPI_Comm, char *, int *))GetProcAddress(hPMPIModule, "PMPI_Comm_get_name");
    fn.PMPI_Comm_set_attr = (int (*)(MPI_Comm, int, void *))GetProcAddress(hPMPIModule, "PMPI_Comm_set_attr");
    fn.PMPI_Comm_set_name = (int (*)(MPI_Comm, char *))GetProcAddress(hPMPIModule, "PMPI_Comm_set_name");
    fn.PMPI_File_call_errhandler = (int (*)(MPI_File, int))GetProcAddress(hPMPIModule, "PMPI_File_call_errhandler");
    fn.PMPI_Grequest_complete = (int (*)(MPI_Request))GetProcAddress(hPMPIModule, "PMPI_Grequest_complete");
    fn.PMPI_Grequest_start = (int (*)(MPI_Grequest_query_function *, MPI_Grequest_free_function *, MPI_Grequest_cancel_function *, void *, MPI_Request *))GetProcAddress(hPMPIModule, "PMPI_Grequest_start");
    fn.PMPI_Init_thread = (int (*)(int *, char ***, int, int *))GetProcAddress(hPMPIModule, "PMPI_Init_thread");
    fn.PMPI_Is_thread_main = (int (*)(int *))GetProcAddress(hPMPIModule, "PMPI_Is_thread_main");
    fn.PMPI_Query_thread = (int (*)(int *))GetProcAddress(hPMPIModule, "PMPI_Query_thread");
    fn.PMPI_Status_set_cancelled = (int (*)(MPI_Status *, int))GetProcAddress(hPMPIModule, "PMPI_Status_set_cancelled");
    fn.PMPI_Status_set_elements = (int (*)(MPI_Status *, MPI_Datatype, int))GetProcAddress(hPMPIModule, "PMPI_Status_set_elements");
    fn.PMPI_Type_create_keyval = (int (*)(MPI_Type_copy_attr_function *, MPI_Type_delete_attr_function *, int *, void *))GetProcAddress(hPMPIModule, "PMPI_Type_create_keyval");
    fn.PMPI_Type_delete_attr = (int (*)(MPI_Datatype, int))GetProcAddress(hPMPIModule, "PMPI_Type_delete_attr");
    fn.PMPI_Type_dup = (int (*)(MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "PMPI_Type_dup");
    fn.PMPI_Type_free_keyval = (int (*)(int *))GetProcAddress(hPMPIModule, "PMPI_Type_free_keyval");
    fn.PMPI_Type_get_attr = (int (*)(MPI_Datatype, int, void *, int *))GetProcAddress(hPMPIModule, "PMPI_Type_get_attr");
    fn.PMPI_Type_get_contents = (int (*)(MPI_Datatype, int, int, int, int [], MPI_Aint [], MPI_Datatype []))GetProcAddress(hPMPIModule, "PMPI_Type_get_contents");
    fn.PMPI_Type_get_envelope = (int (*)(MPI_Datatype, int *, int *, int *, int *))GetProcAddress(hPMPIModule, "PMPI_Type_get_envelope");
    fn.PMPI_Type_get_name = (int (*)(MPI_Datatype, char *, int *))GetProcAddress(hPMPIModule, "PMPI_Type_get_name");
    fn.PMPI_Type_set_attr = (int (*)(MPI_Datatype, int, void *))GetProcAddress(hPMPIModule, "PMPI_Type_set_attr");
    fn.PMPI_Type_set_name = (int (*)(MPI_Datatype, char *))GetProcAddress(hPMPIModule, "PMPI_Type_set_name");
    fn.PMPI_Type_match_size = (int (*)( int, int, MPI_Datatype *))GetProcAddress(hPMPIModule, "PMPI_Type_match_size");
    fn.PMPI_Win_call_errhandler = (int (*)(MPI_Win, int))GetProcAddress(hPMPIModule, "PMPI_Win_call_errhandler");
    fn.PMPI_Win_create_keyval = (int (*)(MPI_Win_copy_attr_function *, MPI_Win_delete_attr_function *, int *, void *))GetProcAddress(hPMPIModule, "PMPI_Win_create_keyval");
    fn.PMPI_Win_delete_attr = (int (*)(MPI_Win, int))GetProcAddress(hPMPIModule, "PMPI_Win_delete_attr");
    fn.PMPI_Win_free_keyval = (int (*)(int *))GetProcAddress(hPMPIModule, "PMPI_Win_free_keyval");
    fn.PMPI_Win_get_attr = (int (*)(MPI_Win, int, void *, int *))GetProcAddress(hPMPIModule, "PMPI_Win_get_attr");
    fn.PMPI_Win_get_name = (int (*)(MPI_Win, char *, int *))GetProcAddress(hPMPIModule, "PMPI_Win_get_name");
    fn.PMPI_Win_set_attr = (int (*)(MPI_Win, int, void *))GetProcAddress(hPMPIModule, "PMPI_Win_set_attr");
    fn.PMPI_Win_set_name = (int (*)(MPI_Win, char *))GetProcAddress(hPMPIModule, "PMPI_Win_set_name");
    fn.PMPI_Alloc_mem = (int (*)(MPI_Aint, MPI_Info info, void *baseptr))GetProcAddress(hPMPIModule, "PMPI_Alloc_mem");
    fn.PMPI_Comm_create_errhandler = (int (*)(MPI_Comm_errhandler_fn *, MPI_Errhandler *))GetProcAddress(hPMPIModule, "PMPI_Comm_create_errhandler");
    fn.PMPI_Comm_get_errhandler = (int (*)(MPI_Comm, MPI_Errhandler *))GetProcAddress(hPMPIModule, "PMPI_Comm_get_errhandler");
    fn.PMPI_Comm_set_errhandler = (int (*)(MPI_Comm, MPI_Errhandler))GetProcAddress(hPMPIModule, "PMPI_Comm_set_errhandler");
    fn.PMPI_File_create_errhandler = (int (*)(MPI_File_errhandler_fn *, MPI_Errhandler *))GetProcAddress(hPMPIModule, "PMPI_File_create_errhandler");
    fn.PMPI_File_get_errhandler = (int (*)(MPI_File, MPI_Errhandler *))GetProcAddress(hPMPIModule, "PMPI_File_get_errhandler");
    fn.PMPI_File_set_errhandler = (int (*)(MPI_File, MPI_Errhandler))GetProcAddress(hPMPIModule, "PMPI_File_set_errhandler");
    fn.PMPI_Finalized = (int (*)(int *))GetProcAddress(hPMPIModule, "PMPI_Finalized");
    fn.PMPI_Free_mem = (int (*)(void *))GetProcAddress(hPMPIModule, "PMPI_Free_mem");
    fn.PMPI_Get_address = (int (*)(void *, MPI_Aint *))GetProcAddress(hPMPIModule, "PMPI_Get_address");
    fn.PMPI_Info_create = (int (*)(MPI_Info *))GetProcAddress(hPMPIModule, "PMPI_Info_create");
    fn.PMPI_Info_delete = (int (*)(MPI_Info, char *))GetProcAddress(hPMPIModule, "PMPI_Info_delete");
    fn.PMPI_Info_dup = (int (*)(MPI_Info, MPI_Info *))GetProcAddress(hPMPIModule, "PMPI_Info_dup");
    fn.PMPI_Info_free = (int (*)(MPI_Info *info))GetProcAddress(hPMPIModule, "PMPI_Info_free");
    fn.PMPI_Info_get = (int (*)(MPI_Info, char *, int, char *, int *))GetProcAddress(hPMPIModule, "PMPI_Info_get");
    fn.PMPI_Info_get_nkeys = (int (*)(MPI_Info, int *))GetProcAddress(hPMPIModule, "PMPI_Info_get_nkeys");
    fn.PMPI_Info_get_nthkey = (int (*)(MPI_Info, int, char *))GetProcAddress(hPMPIModule, "PMPI_Info_get_nthkey");
    fn.PMPI_Info_get_valuelen = (int (*)(MPI_Info, char *, int *, int *))GetProcAddress(hPMPIModule, "PMPI_Info_get_valuelen");
    fn.PMPI_Info_set = (int (*)(MPI_Info, char *, char *))GetProcAddress(hPMPIModule, "PMPI_Info_set");
    fn.PMPI_Pack_external = (int (*)(char *, void *, int, MPI_Datatype, void *, MPI_Aint, MPI_Aint *))GetProcAddress(hPMPIModule, "PMPI_Pack_external"); 
    fn.PMPI_Pack_external_size = (int (*)(char *, int, MPI_Datatype, MPI_Aint *))GetProcAddress(hPMPIModule, "PMPI_Pack_external_size"); 
    fn.PMPI_Request_get_status = (int (*)(MPI_Request, int *, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_Request_get_status");
    fn.PMPI_Status_c2f = (int (*)(MPI_Status *, MPI_Fint *))GetProcAddress(hPMPIModule, "PMPI_Status_c2f");
    fn.PMPI_Status_f2c = (int (*)(MPI_Fint *, MPI_Status *))GetProcAddress(hPMPIModule, "PMPI_Status_f2c");
    fn.PMPI_Type_create_darray = (int (*)(int, int, int, int [], int [], int [], int [], int, MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "PMPI_Type_create_darray");
    fn.PMPI_Type_create_hindexed = (int (*)(int, int [], MPI_Aint [], MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "PMPI_Type_create_hindexed");
    fn.PMPI_Type_create_hvector = (int (*)(int, int, MPI_Aint, MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "PMPI_Type_create_hvector");
    fn.PMPI_Type_create_indexed_block = (int (*)(int, int, int [], MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "PMPI_Type_create_indexed_block");
    fn.PMPI_Type_create_resized = (int (*)(MPI_Datatype, MPI_Aint, MPI_Aint, MPI_Datatype *))GetProcAddress(hPMPIModule, "PMPI_Type_create_resized");
    fn.PMPI_Type_create_struct = (int (*)(int, int [], MPI_Aint [], MPI_Datatype [], MPI_Datatype *))GetProcAddress(hPMPIModule, "PMPI_Type_create_struct");
    fn.PMPI_Type_create_subarray = (int (*)(int, int [], int [], int [], int, MPI_Datatype, MPI_Datatype *))GetProcAddress(hPMPIModule, "PMPI_Type_create_subarray");
    fn.PMPI_Type_get_extent = (int (*)(MPI_Datatype, MPI_Aint *, MPI_Aint *))GetProcAddress(hPMPIModule, "PMPI_Type_get_extent");
    fn.PMPI_Type_get_true_extent = (int (*)(MPI_Datatype, MPI_Aint *, MPI_Aint *))GetProcAddress(hPMPIModule, "PMPI_Type_get_true_extent");
    fn.PMPI_Unpack_external = (int (*)(char *, void *, MPI_Aint, MPI_Aint *, void *, int, MPI_Datatype))GetProcAddress(hPMPIModule, "PMPI_Unpack_external"); 
    fn.PMPI_Win_create_errhandler = (int (*)(MPI_Win_errhandler_fn *, MPI_Errhandler *))GetProcAddress(hPMPIModule, "PMPI_Win_create_errhandler");
    fn.PMPI_Win_get_errhandler = (int (*)(MPI_Win, MPI_Errhandler *))GetProcAddress(hPMPIModule, "PMPI_Win_get_errhandler");
    fn.PMPI_Win_set_errhandler = (int (*)(MPI_Win, MPI_Errhandler))GetProcAddress(hPMPIModule, "PMPI_Win_set_errhandler");
    fn.PMPI_Type_create_f90_integer = (int (*)( int, MPI_Datatype * ))GetProcAddress(hPMPIModule, "PMPI_Type_create_f90_integer");
    fn.PMPI_Type_create_f90_real = (int (*)( int, int, MPI_Datatype * ))GetProcAddress(hPMPIModule, "PMPI_Type_create_f90_real");
    fn.PMPI_Type_create_f90_complex = (int (*)( int, int, MPI_Datatype * ))GetProcAddress(hPMPIModule, "PMPI_Type_create_f90_complex");

    /* Extra exported internal symbols */
    fn.MPIR_Keyval_set_fortran = (void (*)(int))GetProcAddress(hPMPIModule, "MPIR_Keyval_set_fortran");
    fn.MPIR_Keyval_set_fortran90 = (void (*)(int))GetProcAddress(hPMPIModule, "MPIR_Keyval_set_fortran90");
    fn.MPIR_Grequest_set_lang_f77 = (void (*)(MPI_Request))GetProcAddress(hPMPIModule, "MPIR_Grequest_set_lang_f77");
    fn.MPIR_Keyval_set_cxx = (void (*)(int, void (*)(void), void (*)(void)))GetProcAddress(hPMPIModule, "MPIR_Keyval_set_cxx");
    fn.MPIR_Errhandler_set_cxx = (void (*)(MPI_Errhandler, void (*)(void)))GetProcAddress(hPMPIModule, "MPIR_Errhandler_set_cxx");
    fn.MPIR_Op_set_cxx = (void (*)(MPI_Op, void (*)(void)))GetProcAddress(hPMPIModule, "MPIR_Op_set_cxx");
    fn.MPID_Wtick = (double (*)(void))GetProcAddress(hPMPIModule, "MPID_Wtick");
    fn.MPID_Wtime_todouble = (void (*)(MPID_Time_t *, double *))GetProcAddress(hPMPIModule, "MPID_Wtime_todouble");
    /*fn.MPIR_Dup_fn = (int (*)(MPI_Comm, int, void *, void *, void *, int *))GetProcAddress(hPMPIModule, "MPIR_Dup_fn");*/
    fn.MPIR_Err_create_code = (int (*)(int , int , const char [], int , int , const char [], const char [], ...))GetProcAddress(hPMPIModule, "MPIR_Err_create_code");
    fn.MPIR_Err_return_comm = (int (*)(struct MPID_Comm *, const char [], int ))GetProcAddress(hPMPIModule, "MPIR_Err_return_comm");

    /* global variables */
    fn.MPI_F_STATUS_IGNORE = (MPI_Fint**)GetProcAddress(hPMPIModule, "MPI_F_STATUS_IGNORE");
    fn.MPI_F_STATUSES_IGNORE = (MPI_Fint**)GetProcAddress(hPMPIModule, "MPI_F_STATUSES_IGNORE");

    return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    char *dll_name, *channel;
    char *wrapper_dll_name = NULL;
    char name[MAX_DLL_NAME];
    BOOL result = TRUE;

    MPIU_UNREFERENCED_ARG(hinstDLL);
    MPIU_UNREFERENCED_ARG(lpReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
	    /* Get the mpi dll name */
	    /* precedence goes to the dll name so check for it first */
	    dll_name = getenv(MPI_ENV_DLL_NAME);
	    if (!dll_name)
	    {
		/* no dll name specified so check for a channel */
		channel = getenv(MPI_ENV_CHANNEL_NAME);
		if (channel != NULL)
		{
		    /* ignore the sock channel since it is the default and is not named mpich2sock.dll */
		    if (strncmp(channel, "sock", 5))
		    {
			snprintf(name, MAX_DLL_NAME, DLL_FORMAT_STRING, channel);
			dll_name = name;
		    }
		}
		/* no dll or channel specified so use the default */
		if (!dll_name)
		{
		    dll_name = MPI_DEFAULT_DLL_NAME;
		}
	    }

	    /* Get the mpi wrapper dll name */
	    wrapper_dll_name = getenv(MPI_ENV_MPIWRAP_DLL_NAME);
	    if (wrapper_dll_name)
	    {
		/* FIXME: Should we allow for short wrapper names like 'mpe'? */
		if (strncmp(wrapper_dll_name, "mpe", 4) == 0)
		{
		    wrapper_dll_name = MPI_DEFAULT_WRAP_DLL_NAME;
		}
	    }

	    /* Load the functions */
	    result = LoadFunctions(dll_name, wrapper_dll_name);
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;
    }
    return result;
}

/* Extra exported internal functions to mpich2 */
int MPIR_Err_create_code(int lastcode, int fatal, const char fcname[], int line, int error_class, const char generic_msg[], const char specific_msg[], ...)
{
    int result;
    va_list args;
    va_start(args, specific_msg);
    result = fn.MPIR_Err_create_code(lastcode, fatal, fcname, line, error_class, generic_msg, specific_msg, args);
    va_end(args);
    return result;
}

int MPIR_Err_return_comm(struct MPID_Comm *comm_ptr, const char fcname[], int errcode)
{
    return fn.MPIR_Err_return_comm(comm_ptr, fcname, errcode);
}

int MPIR_Dup_fn(MPI_Comm comm, int keyval, void *extra_state, void *attr_in, void *attr_out, int *flag)
{
    return fn.MPIR_Dup_fn(comm, keyval, extra_state, attr_in, attr_out, flag);
}

void MPIR_Keyval_set_fortran(int keyval)
{
    fn.MPIR_Keyval_set_fortran(keyval);
}

void MPIR_Keyval_set_fortran90(int keyval)
{
    fn.MPIR_Keyval_set_fortran90(keyval);
}

void MPIR_Grequest_set_lang_f77(MPI_Request greq)
{
    fn.MPIR_Grequest_set_lang_f77(greq);
}

void MPIR_Keyval_set_cxx(int keyval, void (*delfn)(void), void (*copyfn)(void))
{
    fn.MPIR_Keyval_set_cxx(keyval, delfn, copyfn);
}

void MPIR_Errhandler_set_cxx(MPI_Errhandler errhand, void (*errcall)(void))
{
    fn.MPIR_Errhandler_set_cxx(errhand, errcall);
}

void MPIR_Op_set_cxx(MPI_Op op, void (*opcall)(void))
{
    fn.MPIR_Op_set_cxx(op, opcall);
}

double MPID_Wtick(void)
{
    return fn.MPID_Wtick();
}

void MPID_Wtime_todouble(MPID_Time_t *t, double *val)
{
    fn.MPID_Wtime_todouble(t, val);
}

/* MPI versions */
int MPI_Init( int *argc, char ***argv )
{
    int result;
    result = fn.MPI_Init(argc, argv);
    MPI_F_STATUS_IGNORE = *fn.MPI_F_STATUS_IGNORE;
    MPI_F_STATUSES_IGNORE = *fn.MPI_F_STATUSES_IGNORE;
    return result;
}

int MPI_Init_thread( int *argc, char ***argv, int required, int *provided )
{
    int result;
    result = fn.MPI_Init_thread(argc, argv, required, provided);
    MPI_F_STATUS_IGNORE = *fn.MPI_F_STATUS_IGNORE;
    MPI_F_STATUSES_IGNORE = *fn.MPI_F_STATUSES_IGNORE;
    return result;
}

int MPI_Status_c2f( MPI_Status *c_status, MPI_Fint *f_status )
{
    return fn.MPI_Status_c2f(c_status, f_status);
}

int MPI_Status_f2c( MPI_Fint *f_status, MPI_Status *c_status )
{
    return fn.MPI_Status_f2c(f_status, c_status);
}

int MPI_Attr_delete(MPI_Comm comm, int keyval)
{
    return fn.MPI_Attr_delete(comm, keyval);
}

int MPI_Attr_get(MPI_Comm comm, int keyval, void *attr_value, int *flag)
{
    return fn.MPI_Attr_get(comm, keyval, attr_value, flag);
}

int MPI_Attr_put(MPI_Comm comm, int keyval, void *attr_value)
{
    return fn.MPI_Attr_put(comm, keyval, attr_value);
}

int MPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn, 
			   MPI_Comm_delete_attr_function *comm_delete_attr_fn, 
			   int *comm_keyval, void *extra_state)
{
    return fn.MPI_Comm_create_keyval(comm_copy_attr_fn, comm_delete_attr_fn, comm_keyval, extra_state);
}

int MPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval)
{
    return fn.MPI_Comm_delete_attr(comm, comm_keyval);
}

int MPI_Comm_free_keyval(int *comm_keyval)
{
    return fn.MPI_Comm_free_keyval(comm_keyval);
}

int MPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag)
{
    return fn.MPI_Comm_get_attr(comm, comm_keyval, attribute_val, flag);
}

int MPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val)
{
    return fn.MPI_Comm_set_attr(comm, comm_keyval, attribute_val);
}

int MPI_Keyval_create(MPI_Copy_function *copy_fn, 
		      MPI_Delete_function *delete_fn, 
		      int *keyval, void *extra_state)
{
    return fn.MPI_Keyval_create(copy_fn, delete_fn, keyval, extra_state);
}

int MPI_Keyval_free(int *keyval)
{
    return fn.MPI_Keyval_free(keyval);
}

int MPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn, 
			   MPI_Type_delete_attr_function *type_delete_attr_fn,
			   int *type_keyval, void *extra_state)
{
    return fn.MPI_Type_create_keyval(type_copy_attr_fn, type_delete_attr_fn, type_keyval, extra_state);
}

int MPI_Type_delete_attr(MPI_Datatype type, int type_keyval)
{
    return fn.MPI_Type_delete_attr(type, type_keyval);
}

int MPI_Type_free_keyval(int *type_keyval)
{
    return fn.MPI_Type_free_keyval(type_keyval);
}

int MPI_Type_get_attr(MPI_Datatype type, int type_keyval, void *attribute_val, int *flag)
{
    return fn.MPI_Type_get_attr(type, type_keyval, attribute_val, flag);
}

int MPI_Type_set_attr(MPI_Datatype type, int type_keyval, void *attribute_val)
{
    return fn.MPI_Type_set_attr(type, type_keyval, attribute_val);
}

int MPI_Win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn, 
			  MPI_Win_delete_attr_function *win_delete_attr_fn, 
			  int *win_keyval, void *extra_state)
{
    return fn.MPI_Win_create_keyval(win_copy_attr_fn, win_delete_attr_fn, win_keyval, extra_state);
}

int MPI_Win_delete_attr(MPI_Win win, int win_keyval)
{
    return fn.MPI_Win_delete_attr(win, win_keyval);
}

int MPI_Win_free_keyval(int *win_keyval)
{
    return fn.MPI_Win_free_keyval(win_keyval);
}

int MPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, 
		     int *flag)
{
    return fn.MPI_Win_get_attr(win, win_keyval, attribute_val, flag);
}

int MPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val)
{
    return fn.MPI_Win_set_attr(win, win_keyval, attribute_val);
}

int MPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                  void *recvbuf, int recvcount, MPI_Datatype recvtype, 
                  MPI_Comm comm)
{
    return fn.MPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

int MPI_Allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                   void *recvbuf, int *recvcounts, int *displs, 
                   MPI_Datatype recvtype, MPI_Comm comm)
{
    return fn.MPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
}

int MPI_Allreduce ( void *sendbuf, void *recvbuf, int count, 
		    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm )
{
    return fn.MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
}

int MPI_Alltoall(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, 
                 MPI_Comm comm)
{
    return fn.MPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

int MPI_Alltoallv(void *sendbuf, int *sendcnts, int *sdispls, 
                  MPI_Datatype sendtype, void *recvbuf, int *recvcnts, 
                  int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
    return fn.MPI_Alltoallv(sendbuf, sendcnts, sdispls, sendtype, recvbuf, recvcnts, rdispls, recvtype, comm);
}

int MPI_Alltoallw(void *sendbuf, int *sendcnts, int *sdispls, 
                  MPI_Datatype *sendtypes, void *recvbuf, int *recvcnts, 
                  int *rdispls, MPI_Datatype *recvtypes, MPI_Comm comm)
{
    return fn.MPI_Alltoallw(sendbuf, sendcnts, sdispls, sendtypes, recvbuf, recvcnts, rdispls, recvtypes, comm);
}

int MPI_Barrier( MPI_Comm comm )
{
    return fn.MPI_Barrier(comm);
}

int MPI_Bcast( void *buffer, int count, MPI_Datatype datatype, int root, 
               MPI_Comm comm )
{
    return fn.MPI_Bcast(buffer, count, datatype, root, comm);
}

int MPI_Exscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, 
               MPI_Op op, MPI_Comm comm)
{
    return fn.MPI_Exscan(sendbuf, recvbuf, count, datatype, op, comm);
}

int MPI_Gather(void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
               void *recvbuf, int recvcnt, MPI_Datatype recvtype, 
               int root, MPI_Comm comm)
{
    return fn.MPI_Gather(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
}

int MPI_Gatherv(void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
                void *recvbuf, int *recvcnts, int *displs, 
                MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    return fn.MPI_Gatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcnts, displs, recvtype, root, comm);
}

int MPI_Op_create(MPI_User_function *function, int commute, MPI_Op *op)
{
    return fn.MPI_Op_create(function, commute, op);
}

int MPI_Op_free(MPI_Op *op)
{
    return fn.MPI_Op_free(op);
}

int MPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, 
	       MPI_Op op, int root, MPI_Comm comm)
{
    return fn.MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
}

int MPI_Reduce_scatter(void *sendbuf, void *recvbuf, int *recvcnts, 
		       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    return fn.MPI_Reduce_scatter(sendbuf, recvbuf, recvcnts, datatype, op, comm);
}

int MPI_Scan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, 
	     MPI_Op op, MPI_Comm comm)
{
    return fn.MPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
}

int MPI_Scatter(void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
		void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root, 
		MPI_Comm comm)
{
    return fn.MPI_Scatter(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
}

int MPI_Scatterv( void *sendbuf, int *sendcnts, int *displs, 
		  MPI_Datatype sendtype, void *recvbuf, int recvcnt,
		  MPI_Datatype recvtype,
		  int root, MPI_Comm comm)
{
    return fn.MPI_Scatterv(sendbuf, sendcnts, displs, sendtype, recvbuf, recvcnt, recvtype, root, comm);
}

int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
    return fn.MPI_Comm_compare(comm1, comm2, result);
}

int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
    return fn.MPI_Comm_create(comm, group, newcomm);
}

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
    return fn.MPI_Comm_dup(comm, newcomm);
}

int MPI_Comm_free(MPI_Comm *comm)
{
    return fn.MPI_Comm_free(comm);
}

int MPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen)
{
    return fn.MPI_Comm_get_name(comm, comm_name, resultlen);
}

int MPI_Comm_group(MPI_Comm comm, MPI_Group *group)
{
    return fn.MPI_Comm_group(comm, group);
}

int MPI_Comm_rank( MPI_Comm comm, int *rank )
{
    return fn.MPI_Comm_rank(comm, rank);
}

int MPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group)
{
    return fn.MPI_Comm_remote_group(comm, group);
}

int MPI_Comm_remote_size(MPI_Comm comm, int *size)
{
    return fn.MPI_Comm_remote_size(comm, size);
}

int MPI_Comm_set_name(MPI_Comm comm, char *comm_name)
{
    return fn.MPI_Comm_set_name(comm, comm_name);
}

int MPI_Comm_size( MPI_Comm comm, int *size )
{
    return fn.MPI_Comm_size(comm, size);
}

int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
    return fn.MPI_Comm_split(comm, color, key, newcomm);
}

int MPI_Comm_test_inter(MPI_Comm comm, int *flag)
{
    return fn.MPI_Comm_test_inter(comm, flag);
}

int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, 
			 MPI_Comm peer_comm, int remote_leader, int tag, 
			 MPI_Comm *newintercomm)
{
    return fn.MPI_Intercomm_create(local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm);
}

int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm)
{
    return fn.MPI_Intercomm_merge(intercomm, high, newintracomm);
}

int MPI_Address( void *location, MPI_Aint *address )
{
    return fn.MPI_Address(location, address);
}

int MPI_Get_address(void *location, MPI_Aint *address)
{
    return fn.MPI_Get_address(location, address);
}

int MPI_Get_count( MPI_Status *status, 	MPI_Datatype datatype, int *count )
{
    return fn.MPI_Get_count(status, datatype, count);
}

int MPI_Get_elements(MPI_Status *status, MPI_Datatype datatype, int *elements)
{
    return fn.MPI_Get_elements(status, datatype, elements);
}

int MPI_Pack(void *inbuf,
	     int incount,
	     MPI_Datatype datatype,
	     void *outbuf, 
	     int outcount,
	     int *position,
	     MPI_Comm comm)
{
    return fn.MPI_Pack(inbuf, incount, datatype, outbuf, outcount, position, comm);
}

int MPI_Pack_external(char *datarep,
		      void *inbuf,
		      int incount,
		      MPI_Datatype datatype,
		      void *outbuf,
		      MPI_Aint outcount,
		      MPI_Aint *position)
{
    return fn.MPI_Pack_external(datarep, inbuf, incount, datatype, outbuf, outcount, position);
}

int MPI_Pack_external_size(char *datarep,
			   int incount,
			   MPI_Datatype datatype,
			   MPI_Aint *size)
{
    return fn.MPI_Pack_external_size(datarep, incount, datatype, size);
}

int MPI_Pack_size(int incount,
		  MPI_Datatype datatype,
		  MPI_Comm comm,
		  int *size)
{
    return fn.MPI_Pack_size(incount, datatype, comm, size);
}

int MPI_Register_datarep(char *datarep, 
			 MPI_Datarep_conversion_function *read_conversion_fn, 
			 MPI_Datarep_conversion_function *write_conversion_fn, 
			 MPI_Datarep_extent_function *dtype_file_extent_fn, 
			 void *extra_state)
{
    return fn.MPI_Register_datarep(datarep, read_conversion_fn, write_conversion_fn, dtype_file_extent_fn, extra_state);
}

int MPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype, 
			    int count)
{
    return fn.MPI_Status_set_elements(status, datatype, count);
}

int MPI_Type_commit(MPI_Datatype *datatype)
{
    return fn.MPI_Type_commit(datatype);
}

int MPI_Type_contiguous(int count,
			MPI_Datatype old_type,
			MPI_Datatype *new_type_p)
{
    return fn.MPI_Type_contiguous(count, old_type, new_type_p);
}

int MPI_Type_create_darray(int size,
			   int rank,
			   int ndims,
			   int array_of_gsizes[],
			   int array_of_distribs[],
			   int array_of_dargs[],
			   int array_of_psizes[],
			   int order,
			   MPI_Datatype oldtype,
			   MPI_Datatype *newtype)
{
    return fn.MPI_Type_create_darray(size, rank, ndims, array_of_gsizes, array_of_distribs, array_of_dargs, array_of_psizes, order, oldtype, newtype);
}

int MPI_Type_create_hindexed(int count,
			     int blocklengths[],
			     MPI_Aint displacements[],
			     MPI_Datatype oldtype,
			     MPI_Datatype *newtype)
{
    return fn.MPI_Type_create_hindexed(count, blocklengths, displacements, oldtype, newtype);
}

int MPI_Type_create_hvector(int count,
			    int blocklength,
			    MPI_Aint stride,
			    MPI_Datatype oldtype,
			    MPI_Datatype *newtype)
{
    return fn.MPI_Type_create_hvector(count, blocklength, stride, oldtype, newtype);
}

int MPI_Type_create_indexed_block(int count,
				  int blocklength,
				  int array_of_displacements[],
				  MPI_Datatype oldtype,
				  MPI_Datatype *newtype)
{
    return fn.MPI_Type_create_indexed_block(count, blocklength, array_of_displacements, oldtype, newtype);
}

int MPI_Type_create_resized(MPI_Datatype oldtype,
			    MPI_Aint lb,
			    MPI_Aint extent,
			    MPI_Datatype *newtype)
{
    return fn.MPI_Type_create_resized(oldtype, lb, extent, newtype);
}

int MPI_Type_create_struct(int count,
			   int array_of_blocklengths[],
			   MPI_Aint array_of_displacements[],
			   MPI_Datatype array_of_types[],
			   MPI_Datatype *newtype)
{
    return fn.MPI_Type_create_struct(count, array_of_blocklengths, array_of_displacements, array_of_types, newtype);
}

int MPI_Type_create_subarray(int ndims,
			     int array_of_sizes[],
			     int array_of_subsizes[],
			     int array_of_starts[],
			     int order,
			     MPI_Datatype oldtype,
			     MPI_Datatype *newtype)
{
    return fn.MPI_Type_create_subarray(ndims, array_of_sizes, array_of_subsizes, array_of_starts, order, oldtype, newtype);
}

int MPI_Type_dup(MPI_Datatype datatype, MPI_Datatype *newtype)
{
    return fn.MPI_Type_dup(datatype, newtype);
}

int MPI_Type_extent(MPI_Datatype datatype, MPI_Aint *extent)
{
    return fn.MPI_Type_extent(datatype, extent);
}

int MPI_Type_free(MPI_Datatype *datatype)
{
    return fn.MPI_Type_free(datatype);
}

int MPI_Type_get_contents(MPI_Datatype datatype,
			  int max_integers,
			  int max_addresses,
			  int max_datatypes,
			  int array_of_integers[],
			  MPI_Aint array_of_addresses[],
			  MPI_Datatype array_of_datatypes[])
{
    return fn.MPI_Type_get_contents(datatype, max_integers, max_addresses, max_datatypes, array_of_integers, array_of_addresses, array_of_datatypes);
}

int MPI_Type_get_envelope(MPI_Datatype datatype,
			  int *num_integers,
			  int *num_addresses,
			  int *num_datatypes,
			  int *combiner)
{
    return fn.MPI_Type_get_envelope(datatype, num_integers, num_addresses, num_datatypes, combiner);
}

int MPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent)
{
    return fn.MPI_Type_get_extent(datatype, lb, extent);
}

int MPI_Type_get_name(MPI_Datatype datatype, char *type_name, int *resultlen)
{
    return fn.MPI_Type_get_name(datatype, type_name, resultlen);
}

int MPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb, 
			     MPI_Aint *true_extent)
{
    return fn.MPI_Type_get_true_extent(datatype, true_lb, true_extent);
}

int MPI_Type_hindexed(int count,
		      int blocklens[],
		      MPI_Aint indices[],
		      MPI_Datatype old_type,
		      MPI_Datatype *newtype)
{
    return fn.MPI_Type_hindexed(count, blocklens, indices, old_type, newtype);
}

int MPI_Type_hvector(int count,
		     int blocklen,
		     MPI_Aint stride,
		     MPI_Datatype old_type,
		     MPI_Datatype *newtype_p)
{
    return fn.MPI_Type_hvector(count, blocklen, stride, old_type, newtype_p);
}

int MPI_Type_indexed(int count,
		     int blocklens[],
		     int indices[],
		     MPI_Datatype old_type,
		     MPI_Datatype *newtype)
{
    return fn.MPI_Type_indexed(count, blocklens, indices, old_type, newtype);
}

int MPI_Type_lb(MPI_Datatype datatype, MPI_Aint *displacement)
{
    return fn.MPI_Type_lb(datatype, displacement);
}

int MPI_Type_match_size(int typeclass, int size, MPI_Datatype *datatype)
{
    return fn.MPI_Type_match_size(typeclass, size, datatype);
}

int MPI_Type_set_name(MPI_Datatype type, char *type_name)
{
    return fn.MPI_Type_set_name(type, type_name);
}

int MPI_Type_size(MPI_Datatype datatype, int *size)
{
    return fn.MPI_Type_size(datatype, size);
}

int MPI_Type_struct(int count,
		    int blocklens[],
		    MPI_Aint indices[],
		    MPI_Datatype old_types[],
		    MPI_Datatype *newtype)
{
    return fn.MPI_Type_struct(count, blocklens, indices, old_types, newtype);
}

int MPI_Type_ub(MPI_Datatype datatype, MPI_Aint *displacement)
{
    return fn.MPI_Type_ub(datatype, displacement);
}

int MPI_Type_vector(int count,
		    int blocklength,
		    int stride, 
		    MPI_Datatype old_type,
		    MPI_Datatype *newtype_p)
{
    return fn.MPI_Type_vector(count, blocklength, stride, old_type, newtype_p);
}

int MPI_Unpack(void *inbuf,
	       int insize,
	       int *position,
	       void *outbuf,
	       int outcount,
	       MPI_Datatype datatype,
	       MPI_Comm comm)
{
    return fn.MPI_Unpack(inbuf, insize, position, outbuf, outcount, datatype, comm);
}

int MPI_Unpack_external(char *datarep,
			void *inbuf,
			MPI_Aint insize,
			MPI_Aint *position,
			void *outbuf,
			int outcount,
			MPI_Datatype datatype)
{
    return fn.MPI_Unpack_external(datarep, inbuf, insize, position, outbuf, outcount, datatype);
}

int MPI_Add_error_class(int *errorclass)
{
    return fn.MPI_Add_error_class(errorclass);
}

int MPI_Add_error_code(int errorclass, int *errorcode)
{
    return fn.MPI_Add_error_code(errorclass, errorcode);
}

int MPI_Add_error_string(int errorcode, char *string)
{
    return fn.MPI_Add_error_string(errorcode, string);
}

int MPI_Comm_call_errhandler(MPI_Comm comm, int errorcode)
{
    return fn.MPI_Comm_call_errhandler(comm, errorcode);
}

int MPI_Comm_create_errhandler(MPI_Comm_errhandler_fn *function, 
                               MPI_Errhandler *errhandler)
{
    return fn.MPI_Comm_create_errhandler(function, errhandler);
}

int MPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler)
{
    return fn.MPI_Comm_get_errhandler(comm, errhandler);
}

int MPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler)
{
    return fn.MPI_Comm_set_errhandler(comm, errhandler);
}

int MPI_Errhandler_create(MPI_Handler_function *function, 
                          MPI_Errhandler *errhandler)
{
    return fn.MPI_Errhandler_create(function, errhandler);
}

int MPI_Errhandler_free(MPI_Errhandler *errhandler)
{
    return fn.MPI_Errhandler_free(errhandler);
}

int MPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler *errhandler)
{
    return fn.MPI_Errhandler_get(comm, errhandler);
}

int MPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler)
{
    return fn.MPI_Errhandler_set(comm, errhandler);
}

int MPI_Error_class(int errorcode, int *errorclass)
{
    return fn.MPI_Error_class(errorcode, errorclass);
}

int MPI_Error_string(int errorcode, char *string, int *resultlen)
{
    return fn.MPI_Error_string(errorcode, string, resultlen);
}

int MPI_File_call_errhandler(MPI_File fh, int errorcode)
{
    return fn.MPI_File_call_errhandler(fh, errorcode);
}

int MPI_File_create_errhandler(MPI_File_errhandler_fn *function, 
                               MPI_Errhandler *errhandler)
{
    return fn.MPI_File_create_errhandler(function, errhandler);
}

int MPI_File_get_errhandler(MPI_File file, MPI_Errhandler *errhandler)
{
    return fn.MPI_File_get_errhandler(file, errhandler);
}

int MPI_File_set_errhandler(MPI_File file, MPI_Errhandler errhandler)
{
    return fn.MPI_File_set_errhandler(file, errhandler);
}

int MPI_Win_call_errhandler(MPI_Win win, int errorcode)
{
    return fn.MPI_Win_call_errhandler(win, errorcode);
}

int MPI_Win_create_errhandler(MPI_Win_errhandler_fn *function, 
			      MPI_Errhandler *errhandler)
{
    return fn.MPI_Win_create_errhandler(function, errhandler);
}

int MPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler *errhandler)
{
    return fn.MPI_Win_get_errhandler(win, errhandler);
}

int MPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler)
{
    return fn.MPI_Win_set_errhandler(win, errhandler);
}

int MPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result)
{
    return fn.MPI_Group_compare(group1, group2, result);
}

int MPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup)
{
    return fn.MPI_Group_difference(group1, group2, newgroup);
}

int MPI_Group_excl(MPI_Group group, int n, int *ranks, MPI_Group *newgroup)
{
    return fn.MPI_Group_excl(group, n, ranks, newgroup);
}

int MPI_Group_free(MPI_Group *group)
{
    return fn.MPI_Group_free(group);
}

int MPI_Group_incl(MPI_Group group, int n, int *ranks, MPI_Group *newgroup)
{
    return fn.MPI_Group_incl(group, n, ranks, newgroup);
}

int MPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup)
{
    return fn.MPI_Group_intersection(group1, group2, newgroup);
}

int MPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], 
                         MPI_Group *newgroup)
{
    return fn.MPI_Group_range_excl(group, n, ranges, newgroup);
}

int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], 
                         MPI_Group *newgroup)
{
    return fn.MPI_Group_range_incl(group, n, ranges, newgroup);
}

int MPI_Group_rank(MPI_Group group, int *rank)
{
    return fn.MPI_Group_rank(group, rank);
}

int MPI_Group_size(MPI_Group group, int *size)
{
    return fn.MPI_Group_size(group, size);
}

int MPI_Group_translate_ranks(MPI_Group group1, int n, int *ranks1, MPI_Group group2, int *ranks2)
{
    return fn.MPI_Group_translate_ranks(group1, n, ranks1, group2, ranks2);
}

int MPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup)
{
    return fn.MPI_Group_union(group1, group2, newgroup);
}

int MPI_Abort(MPI_Comm comm, int errorcode)
{
    return fn.MPI_Abort(comm, errorcode);
}

int MPI_Finalize( void )
{
    return fn.MPI_Finalize();
}

int MPI_Finalized( int *flag )
{
    return fn.MPI_Finalized(flag);
}

int MPI_Initialized( int *flag )
{
    return fn.MPI_Initialized(flag);
}

int MPI_Is_thread_main( int *flag )
{
    return fn.MPI_Is_thread_main(flag);
}

int MPI_Query_thread( int *provided )
{
    return fn.MPI_Query_thread(provided);
}

int MPI_Get_processor_name( char *name, int *resultlen )
{
    return fn.MPI_Get_processor_name(name, resultlen);
}

int MPI_Pcontrol(const int level, ...)
{
    int ret_val;
    va_list list;

    va_start(list, level);
    ret_val = fn.MPI_Pcontrol(level, list);
    va_end(list);
    return ret_val;
}

int MPI_Get_version( int *version, int *subversion )
{
    return fn.MPI_Get_version(version, subversion);
}

int MPI_Bsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, 
	      MPI_Comm comm)
{
    return fn.MPI_Bsend(buf, count, datatype, dest, tag, comm);
}

int MPI_Bsend_init(void *buf, int count, MPI_Datatype datatype, 
                   int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    return fn.MPI_Bsend_init(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Buffer_attach(void *buffer, int size)
{
    return fn.MPI_Buffer_attach(buffer, size);
}

int MPI_Buffer_detach(void *buffer, int *size)
{
    return fn.MPI_Buffer_detach(buffer, size);
}

int MPI_Cancel(MPI_Request *request)
{
    return fn.MPI_Cancel(request);
}

int MPI_Grequest_complete( MPI_Request request )
{
    return fn.MPI_Grequest_complete(request);
}

int MPI_Grequest_start( MPI_Grequest_query_function *query_fn, 
			MPI_Grequest_free_function *free_fn, 
			MPI_Grequest_cancel_function *cancel_fn, 
			void *extra_state, MPI_Request *request )
{
    return fn.MPI_Grequest_start(query_fn, free_fn, cancel_fn, extra_state, request);
}

int MPI_Ibsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, 
	       MPI_Comm comm, MPI_Request *request)
{
    return fn.MPI_Ibsend(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, 
	       MPI_Status *status)
{
    return fn.MPI_Iprobe(source, tag, comm, flag, status);
}

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source,
	      int tag, MPI_Comm comm, MPI_Request *request)
{
    return fn.MPI_Irecv(buf, count, datatype, source, tag, comm, request);
}

int MPI_Irsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	       MPI_Comm comm, MPI_Request *request)
{
    return fn.MPI_Irsend(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	      MPI_Comm comm, MPI_Request *request)
{
    return fn.MPI_Isend(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Issend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	       MPI_Comm comm, MPI_Request *request)
{
    return fn.MPI_Issend(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status)
{
    return fn.MPI_Probe(source, tag, comm, status);
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
	     MPI_Comm comm, MPI_Status *status)
{
    return fn.MPI_Recv(buf, count, datatype, source, tag, comm, status);
}

int MPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source, 
		  int tag, MPI_Comm comm, MPI_Request *request)
{
    return fn.MPI_Recv_init(buf, count, datatype, source, tag, comm, request);
}

int MPI_Request_free(MPI_Request *request)
{
    return fn.MPI_Request_free(request);
}

int MPI_Request_get_status(MPI_Request request, int *flag, MPI_Status *status)
{
    return fn.MPI_Request_get_status(request, flag, status);
}

int MPI_Rsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	      MPI_Comm comm)
{
    return fn.MPI_Rsend(buf, count, datatype, dest, tag, comm);
}

int MPI_Rsend_init(void *buf, int count, MPI_Datatype datatype, int dest,
		   int tag, MPI_Comm comm, MPI_Request *request)
{
    return fn.MPI_Rsend_init(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm)
{
    return fn.MPI_Send(buf, count, datatype, dest, tag, comm);
}

int MPI_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
		 int dest, int sendtag,
		 void *recvbuf, int recvcount, MPI_Datatype recvtype, 
		 int source, int recvtag,
		 MPI_Comm comm, MPI_Status *status)
{
    return fn.MPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status);
}

int MPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, 
			 int dest, int sendtag, int source, int recvtag,
			 MPI_Comm comm, MPI_Status *status)
{
    return fn.MPI_Sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, comm, status);
}

int MPI_Send_init(void *buf, int count, MPI_Datatype datatype, int dest,
		  int tag, MPI_Comm comm, MPI_Request *request)
{
    return fn.MPI_Send_init(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Ssend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	      MPI_Comm comm)
{
    return fn.MPI_Ssend(buf, count, datatype, dest, tag, comm);
}

int MPI_Ssend_init(void *buf, int count, MPI_Datatype datatype, int dest,
		   int tag, MPI_Comm comm, MPI_Request *request)
{
    return fn.MPI_Ssend_init(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Start(MPI_Request *request)
{
    return fn.MPI_Start(request);
}

int MPI_Startall(int count, MPI_Request array_of_requests[])
{
    return fn.MPI_Startall(count, array_of_requests);
}

int MPI_Status_set_cancelled(MPI_Status *status, int flag)
{
    return fn.MPI_Status_set_cancelled(status, flag);
}

int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
{
    return fn.MPI_Test(request, flag, status);
}

int MPI_Testall(int count, MPI_Request array_of_requests[], int *flag, 
		MPI_Status array_of_statuses[])
{
    return fn.MPI_Testall(count, array_of_requests, flag, array_of_statuses);
}

int MPI_Testany(int count, MPI_Request array_of_requests[], int *index, 
		int *flag, MPI_Status *status)
{
    return fn.MPI_Testany(count, array_of_requests, index, flag, status);
}

int MPI_Testsome(int incount, MPI_Request array_of_requests[], int *outcount, 
		 int array_of_indices[], MPI_Status array_of_statuses[])
{
    return fn.MPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
}

int MPI_Test_cancelled(MPI_Status *status, int *flag)
{
    return fn.MPI_Test_cancelled(status, flag);
}

int MPI_Wait(MPI_Request *request, MPI_Status *status)
{
    return fn.MPI_Wait(request, status);
}

int MPI_Waitall(int count, MPI_Request array_of_requests[], 
		MPI_Status array_of_statuses[])
{
    return fn.MPI_Waitall(count, array_of_requests, array_of_statuses);
}

int MPI_Waitany(int count, MPI_Request array_of_requests[], int *index, 
		MPI_Status *status)
{
    return fn.MPI_Waitany(count, array_of_requests, index, status);
}

int MPI_Waitsome(int incount, MPI_Request array_of_requests[], 
		 int *outcount, int array_of_indices[],
		 MPI_Status array_of_statuses[])
{
    return fn.MPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
}

int MPI_Accumulate(void *origin_addr, int origin_count, MPI_Datatype
                   origin_datatype, int target_rank, MPI_Aint
                   target_disp, int target_count, MPI_Datatype
                   target_datatype, MPI_Op op, MPI_Win win)
{
    return fn.MPI_Accumulate(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, op, win);
}

int MPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr)
{
    return fn.MPI_Alloc_mem(size, info, baseptr);
}

int MPI_Free_mem(void *base)
{
    return fn.MPI_Free_mem(base);
}

int MPI_Get(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPI_Win
            win)
{
    return fn.MPI_Get(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win);
}

int MPI_Put(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPI_Win
            win)
{
    return fn.MPI_Put(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win);
}

int MPI_Win_complete(MPI_Win win)
{
    return fn.MPI_Win_complete(win);
}

int MPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, 
		   MPI_Comm comm, MPI_Win *win)
{
    return fn.MPI_Win_create(base, size, disp_unit, info, comm, win);
}

int MPI_Win_fence(int assert, MPI_Win win)
{
    return fn.MPI_Win_fence(assert, win);
}

int MPI_Win_free(MPI_Win *win)
{
    return fn.MPI_Win_free(win);
}

int MPI_Win_get_group(MPI_Win win, MPI_Group *group)
{
    return fn.MPI_Win_get_group(win, group);
}

int MPI_Win_get_name(MPI_Win win, char *win_name, int *resultlen)
{
    return fn.MPI_Win_get_name(win, win_name, resultlen);
}

int MPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win)
{
    return fn.MPI_Win_lock(lock_type, rank, assert, win);
}

int MPI_Win_post(MPI_Group group, int assert, MPI_Win win)
{
    return fn.MPI_Win_post(group, assert, win);
}

int MPI_Win_set_name(MPI_Win win, char *win_name)
{
    return fn.MPI_Win_set_name(win, win_name);
}

int MPI_Win_start(MPI_Group group, int assert, MPI_Win win)
{
    return fn.MPI_Win_start(group, assert, win);
}

int MPI_Win_test(MPI_Win win, int *flag)
{
    return fn.MPI_Win_test(win, flag);
}

int MPI_Win_unlock(int rank, MPI_Win win)
{
    return fn.MPI_Win_unlock(rank, win);
}

int MPI_Win_wait(MPI_Win win)
{
    return fn.MPI_Win_wait(win);
}

int MPI_File_close(MPI_File *mpi_fh)
{
    return fn.MPI_File_close(mpi_fh);
}

int MPI_File_delete(char *filename, MPI_Info info)
{
    return fn.MPI_File_delete(filename, info);
}

MPI_Fint MPI_File_c2f(MPI_File mpi_fh)
{
    return fn.MPI_File_c2f(mpi_fh);
}

MPI_File MPI_File_f2c(MPI_Fint i)
{
    return fn.MPI_File_f2c(i);
}

int MPI_File_sync(MPI_File mpi_fh)
{
    return fn.MPI_File_sync(mpi_fh);
}

int MPI_File_get_amode(MPI_File mpi_fh, int *amode)
{
    return fn.MPI_File_get_amode(mpi_fh, amode);
}

int MPI_File_get_atomicity(MPI_File mpi_fh, int *flag)
{
    return fn.MPI_File_get_atomicity(mpi_fh, flag);
}

int MPI_File_get_byte_offset(MPI_File mpi_fh,
			     MPI_Offset offset,
			     MPI_Offset *disp)
{
    return fn.MPI_File_get_byte_offset(mpi_fh, offset, disp);
}

int MPI_File_get_type_extent(MPI_File mpi_fh, MPI_Datatype datatype, 
                             MPI_Aint *extent)
{
    return fn.MPI_File_get_type_extent(mpi_fh, datatype, extent);
}

int MPI_File_get_group(MPI_File mpi_fh, MPI_Group *group)
{
    return fn.MPI_File_get_group(mpi_fh, group);
}

int MPI_File_get_info(MPI_File mpi_fh, MPI_Info *info_used)
{
    return fn.MPI_File_get_info(mpi_fh, info_used);
}

int MPI_File_get_position(MPI_File mpi_fh, MPI_Offset *offset)
{
    return fn.MPI_File_get_position(mpi_fh, offset);
}

int MPI_File_get_position_shared(MPI_File mpi_fh, MPI_Offset *offset)
{
    return fn.MPI_File_get_position_shared(mpi_fh, offset);
}

int MPI_File_get_size(MPI_File mpi_fh, MPI_Offset *size)
{
    return fn.MPI_File_get_size(mpi_fh, size);
}

int MPI_File_get_view(MPI_File mpi_fh,
		      MPI_Offset *disp,
		      MPI_Datatype *etype,
		      MPI_Datatype *filetype,
		      char *datarep)
{
    return fn.MPI_File_get_view(mpi_fh, disp, etype, filetype, datarep);
}

int MPI_File_iread(MPI_File mpi_fh, void *buf, int count, 
		   MPI_Datatype datatype, MPIO_Request *request)
{
    return fn.MPI_File_iread(mpi_fh, buf, count, datatype, request);
}

int MPI_File_iread_at(MPI_File mpi_fh, MPI_Offset offset, void *buf,
                      int count, MPI_Datatype datatype, 
                      MPIO_Request *request)
{
    return fn.MPI_File_iread_at(mpi_fh, offset, buf, count, datatype, request);
}

int MPI_File_iread_shared(MPI_File mpi_fh, void *buf, int count, 
			  MPI_Datatype datatype, MPIO_Request *request)
{
    return fn.MPI_File_iread_shared(mpi_fh, buf, count, datatype, request);
}

int MPI_File_iwrite(MPI_File mpi_fh, void *buf, int count, 
		    MPI_Datatype datatype, MPIO_Request *request)
{
    return fn.MPI_File_iwrite(mpi_fh, buf, count, datatype, request);
}

int MPI_File_iwrite_at(MPI_File mpi_fh, MPI_Offset offset, void *buf,
                       int count, MPI_Datatype datatype, 
                       MPIO_Request *request)
{
    return fn.MPI_File_iwrite_at(mpi_fh, offset, buf, count, datatype, request);
}

int MPI_File_iwrite_shared(MPI_File mpi_fh, void *buf, int count, 
			   MPI_Datatype datatype, MPIO_Request *request)
{
    return fn.MPI_File_iwrite_shared(mpi_fh, buf, count, datatype, request);
}

int MPI_File_open(MPI_Comm comm, char *filename, int amode, 
                  MPI_Info info, MPI_File *fh)
{
    return fn.MPI_File_open(comm, filename, amode, info, fh);
}

int MPI_File_preallocate(MPI_File mpi_fh, MPI_Offset size)
{
    return fn.MPI_File_preallocate(mpi_fh, size);
}

int MPI_File_read_at_all_begin(MPI_File mpi_fh, MPI_Offset offset, void *buf,
			       int count, MPI_Datatype datatype)
{
    return fn.MPI_File_read_at_all_begin(mpi_fh, offset, buf, count, datatype);
}

int MPI_File_read_at_all_end(MPI_File mpi_fh, void *buf, MPI_Status *status)
{
    return fn.MPI_File_read_at_all_end(mpi_fh, buf, status);
}

int MPI_File_read(MPI_File mpi_fh, void *buf, int count, 
                  MPI_Datatype datatype, MPI_Status *status)
{
    return fn.MPI_File_read(mpi_fh, buf, count, datatype, status);
}

int MPI_File_read_all(MPI_File mpi_fh, void *buf, int count, 
                      MPI_Datatype datatype, MPI_Status *status)
{
    return fn.MPI_File_read_all(mpi_fh, buf, count, datatype, status);
}

int MPI_File_read_all_begin(MPI_File mpi_fh, void *buf, int count, 
                            MPI_Datatype datatype)
{
    return fn.MPI_File_read_all_begin(mpi_fh, buf, count, datatype);
}

int MPI_File_read_all_end(MPI_File mpi_fh, void *buf, MPI_Status *status)
{
    return fn.MPI_File_read_all_end(mpi_fh, buf, status);
}

int MPI_File_read_at(MPI_File mpi_fh, MPI_Offset offset, void *buf,
		     int count, MPI_Datatype datatype, MPI_Status *status)
{
    return fn.MPI_File_read_at(mpi_fh, offset, buf, count, datatype, status);
}

int MPI_File_read_at_all(MPI_File mpi_fh, MPI_Offset offset, void *buf,
                         int count, MPI_Datatype datatype, 
                         MPI_Status *status)
{
    return fn.MPI_File_read_at_all(mpi_fh, offset, buf, count, datatype, status);
}

int MPI_File_read_ordered(MPI_File mpi_fh, void *buf, int count, 
                          MPI_Datatype datatype, MPI_Status *status)
{
    return fn.MPI_File_read_ordered(mpi_fh, buf, count, datatype, status);
}

int MPI_File_read_ordered_begin(MPI_File mpi_fh, void *buf, int count, 
				MPI_Datatype datatype)
{
    return fn.MPI_File_read_ordered_begin(mpi_fh, buf, count, datatype);
}

int MPI_File_read_ordered_end(MPI_File mpi_fh, void *buf, MPI_Status *status)
{
    return fn.MPI_File_read_ordered_end(mpi_fh, buf, status);
}

int MPI_File_read_shared(MPI_File mpi_fh, void *buf, int count, 
			 MPI_Datatype datatype, MPI_Status *status)
{
    return fn.MPI_File_read_shared(mpi_fh, buf, count, datatype, status);
}

int MPI_File_seek(MPI_File mpi_fh, MPI_Offset offset, int whence)
{
    return fn.MPI_File_seek(mpi_fh, offset, whence);
}

int MPI_File_seek_shared(MPI_File mpi_fh, MPI_Offset offset, int whence)
{
    return fn.MPI_File_seek_shared(mpi_fh, offset, whence);
}

int MPI_File_set_atomicity(MPI_File mpi_fh, int flag)
{
    return fn.MPI_File_set_atomicity(mpi_fh, flag);
}

int MPI_File_set_info(MPI_File mpi_fh, MPI_Info info)
{
    return fn.MPI_File_set_info(mpi_fh, info);
}

int MPI_File_set_size(MPI_File mpi_fh, MPI_Offset size)
{
    return fn.MPI_File_set_size(mpi_fh, size);
}

int MPI_File_set_view(MPI_File mpi_fh, MPI_Offset disp, MPI_Datatype etype,
		      MPI_Datatype filetype, char *datarep, MPI_Info info)
{
    return fn.MPI_File_set_view(mpi_fh, disp, etype, filetype, datarep, info);
}

int MPI_File_write(MPI_File mpi_fh, void *buf, int count, 
                   MPI_Datatype datatype, MPI_Status *status)
{
    return fn.MPI_File_write(mpi_fh, buf, count, datatype, status);
}

int MPI_File_write_all(MPI_File mpi_fh, void *buf, int count, 
                       MPI_Datatype datatype, MPI_Status *status)
{
    return fn.MPI_File_write_all(mpi_fh, buf, count, datatype, status);
}

int MPI_File_write_all_begin(MPI_File mpi_fh, void *buf, int count, 
			     MPI_Datatype datatype)
{
    return fn.MPI_File_write_all_begin(mpi_fh, buf, count, datatype);
}

int MPI_File_write_all_end(MPI_File mpi_fh, void *buf, MPI_Status *status)
{
    return fn.MPI_File_write_all_end(mpi_fh, buf, status);
}

int MPI_File_write_at(MPI_File mpi_fh, MPI_Offset offset, void *buf,
                      int count, MPI_Datatype datatype, 
                      MPI_Status *status)
{
    return fn.MPI_File_write_at(mpi_fh, offset, buf, count, datatype, status);
}

int MPI_File_write_at_all(MPI_File mpi_fh, MPI_Offset offset, void *buf,
                          int count, MPI_Datatype datatype, 
                          MPI_Status *status)
{
    return fn.MPI_File_write_at_all(mpi_fh, offset, buf, count, datatype, status);
}

int MPI_File_write_ordered(MPI_File mpi_fh, void *buf, int count, 
			   MPI_Datatype datatype, MPI_Status *status)
{
    return fn.MPI_File_write_ordered(mpi_fh, buf, count, datatype, status);
}

int MPI_File_write_ordered_begin(MPI_File mpi_fh, void *buf, int count, 
				 MPI_Datatype datatype)
{
    return fn.MPI_File_write_ordered_begin(mpi_fh, buf, count, datatype);
}

int MPI_File_write_ordered_end(MPI_File mpi_fh, void *buf, MPI_Status *status)
{
    return fn.MPI_File_write_ordered_end(mpi_fh, buf, status);
}

int MPI_File_write_shared(MPI_File mpi_fh, void *buf, int count, 
                          MPI_Datatype datatype, MPI_Status *status)
{
    return fn.MPI_File_write_shared(mpi_fh, buf, count, datatype, status);
}

int MPI_File_write_at_all_begin(MPI_File mpi_fh, MPI_Offset offset, void *buf,
				int count, MPI_Datatype datatype)
{
    return fn.MPI_File_write_at_all_begin(mpi_fh, offset, buf, count, datatype);
}

int MPI_File_write_at_all_end(MPI_File mpi_fh, void *buf, MPI_Status *status)
{
    return fn.MPI_File_write_at_all_end(mpi_fh, buf, status);
}

int MPI_Info_create(MPI_Info *info)
{
    return fn.MPI_Info_create(info);
}

int MPI_Info_delete(MPI_Info info, char *key)
{
    return fn.MPI_Info_delete(info, key);
}

int MPI_Info_dup(MPI_Info info, MPI_Info *newinfo)
{
    return fn.MPI_Info_dup(info, newinfo);
}

int MPI_Info_free(MPI_Info *info)
{
    return fn.MPI_Info_free(info);
}

int MPI_Info_get(MPI_Info info, char *key, int valuelen, char *value, int *flag)
{
    return fn.MPI_Info_get(info, key, valuelen, value, flag);
}

int MPI_Info_get_nkeys(MPI_Info info, int *nkeys)
{
    return fn.MPI_Info_get_nkeys(info, nkeys);
}

int MPI_Info_get_nthkey(MPI_Info info, int n, char *key)
{
    return fn.MPI_Info_get_nthkey(info, n, key);
}

int MPI_Info_get_valuelen(MPI_Info info, char *key, int *valuelen, int *flag)
{
    return fn.MPI_Info_get_valuelen(info, key, valuelen, flag);
}

int MPI_Info_set(MPI_Info info, char *key, char *value)
{
    return fn.MPI_Info_set(info, key, value);
}

int MPI_Close_port(char *port_name)
{
    return fn.MPI_Close_port(port_name);
}

int MPI_Comm_accept(char *port_name, MPI_Info info, int root, MPI_Comm comm, 
                    MPI_Comm *newcomm)
{
    return fn.MPI_Comm_accept(port_name, info, root, comm, newcomm);
}

int MPI_Comm_connect(char *port_name, MPI_Info info, int root, MPI_Comm comm, 
                     MPI_Comm *newcomm)
{
    return fn.MPI_Comm_connect(port_name, info, root, comm, newcomm);
}

int MPI_Comm_disconnect(MPI_Comm * comm)
{
    return fn.MPI_Comm_disconnect(comm);
}

int MPI_Comm_get_parent(MPI_Comm *parent)
{
    return fn.MPI_Comm_get_parent(parent);
}

int MPI_Comm_join(int fd, MPI_Comm *intercomm)
{
    return fn.MPI_Comm_join(fd, intercomm);
}

int MPI_Comm_spawn(char *command, char *argv[], int maxprocs, MPI_Info info, 
		   int root, MPI_Comm comm, MPI_Comm *intercomm,
		   int array_of_errcodes[])
{
    return fn.MPI_Comm_spawn(command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes);
}

int MPI_Comm_spawn_multiple(int count, char *array_of_commands[], char* *array_of_argv[], int array_of_maxprocs[], MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[])
{
    return fn.MPI_Comm_spawn_multiple(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, root, comm, intercomm, array_of_errcodes);
}

int MPI_Lookup_name(char *service_name, MPI_Info info, char *port_name)
{
    return fn.MPI_Lookup_name(service_name, info, port_name);
}

int MPI_Open_port(MPI_Info info, char *port_name)
{
    return fn.MPI_Open_port(info, port_name);
}

int MPI_Publish_name(char *service_name, MPI_Info info, char *port_name)
{
    return fn.MPI_Publish_name(service_name, info, port_name);
}

int MPI_Unpublish_name(char *service_name, MPI_Info info, char *port_name)
{
    return fn.MPI_Unpublish_name(service_name, info, port_name);
}

int MPI_Cartdim_get(MPI_Comm comm, int *ndims)
{
    return fn.MPI_Cartdim_get(comm, ndims);
}

int MPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int *coords)
{
    return fn.MPI_Cart_coords(comm, rank, maxdims, coords);
}

int MPI_Cart_create(MPI_Comm comm_old, int ndims, int *dims, int *periods, 
		    int reorder, MPI_Comm *comm_cart)
{
    return fn.MPI_Cart_create(comm_old, ndims, dims, periods, reorder, comm_cart);
}

int MPI_Cart_get(MPI_Comm comm, int maxdims, int *dims, int *periods, 
                 int *coords)
{
    return fn.MPI_Cart_get(comm, maxdims, dims, periods, coords);
}

int MPI_Cart_map(MPI_Comm comm_old, int ndims, int *dims, int *periods, 
		 int *newrank)
{
    return fn.MPI_Cart_map(comm_old, ndims, dims, periods, newrank);
}

int MPI_Cart_rank(MPI_Comm comm, int *coords, int *rank)
{
    return fn.MPI_Cart_rank(comm, coords, rank);
}

int MPI_Cart_shift(MPI_Comm comm, int direction, int displ, int *source, 
		   int *dest)
{
    return fn.MPI_Cart_shift(comm, direction, displ, source, dest);
}

int MPI_Cart_sub(MPI_Comm comm, int *remain_dims, MPI_Comm *comm_new)
{
    return fn.MPI_Cart_sub(comm, remain_dims, comm_new);
}

int MPI_Dims_create(int nnodes, int ndims, int *dims)
{
    return fn.MPI_Dims_create(nnodes, ndims, dims);
}

int MPI_Graph_create(MPI_Comm comm_old, int nnodes, int *index, int *edges, 
		     int reorder, MPI_Comm *comm_graph)
{
    return fn.MPI_Graph_create(comm_old, nnodes, index, edges, reorder, comm_graph);
}

int MPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges)
{
    return fn.MPI_Graphdims_get(comm, nnodes, nedges);
}

int MPI_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors)
{
    return fn.MPI_Graph_neighbors_count(comm, rank, nneighbors);
}

int MPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, 
                  int *index, int *edges)
{
    return fn.MPI_Graph_get(comm, maxindex, maxedges, index, edges);
}

int MPI_Graph_map(MPI_Comm comm_old, int nnodes, int *index, int *edges,
                  int *newrank)
{
    return fn.MPI_Graph_map(comm_old, nnodes, index, edges, newrank);
}

int MPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, 
			int *neighbors)
{
    return fn.MPI_Graph_neighbors(comm, rank, maxneighbors, neighbors);
}

int MPI_Topo_test(MPI_Comm comm, int *topo_type)
{
    return fn.MPI_Topo_test(comm, topo_type);
}

double MPI_Wtime()
{
    return fn.MPI_Wtime();
}

double MPI_Wtick()
{
    return fn.MPI_Wtick();
}

/* PMPI versions */
int PMPI_Init( int *argc, char ***argv )
{
    int result;
    result = fn.PMPI_Init(argc, argv);
    MPI_F_STATUS_IGNORE = *fn.MPI_F_STATUS_IGNORE;
    MPI_F_STATUSES_IGNORE = *fn.MPI_F_STATUSES_IGNORE;
    return result;
}

int PMPI_Init_thread( int *argc, char ***argv, int required, int *provided )
{
    int result;
    result = fn.PMPI_Init_thread(argc, argv, required, provided);
    MPI_F_STATUS_IGNORE = *fn.MPI_F_STATUS_IGNORE;
    MPI_F_STATUSES_IGNORE = *fn.MPI_F_STATUSES_IGNORE;
    return result;
}

int PMPI_Status_c2f( MPI_Status *c_status, MPI_Fint *f_status )
{
    return fn.PMPI_Status_c2f(c_status, f_status);
}

int PMPI_Status_f2c( MPI_Fint *f_status, MPI_Status *c_status )
{
    return fn.PMPI_Status_f2c(f_status, c_status);
}

int PMPI_Attr_delete(MPI_Comm comm, int keyval)
{
    return fn.PMPI_Attr_delete(comm, keyval);
}

int PMPI_Attr_get(MPI_Comm comm, int keyval, void *attr_value, int *flag)
{
    return fn.PMPI_Attr_get(comm, keyval, attr_value, flag);
}

int PMPI_Attr_put(MPI_Comm comm, int keyval, void *attr_value)
{
    return fn.PMPI_Attr_put(comm, keyval, attr_value);
}

int PMPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn, 
			   MPI_Comm_delete_attr_function *comm_delete_attr_fn, 
			   int *comm_keyval, void *extra_state)
{
    return fn.PMPI_Comm_create_keyval(comm_copy_attr_fn, comm_delete_attr_fn, comm_keyval, extra_state);
}

int PMPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval)
{
    return fn.PMPI_Comm_delete_attr(comm, comm_keyval);
}

int PMPI_Comm_free_keyval(int *comm_keyval)
{
    return fn.PMPI_Comm_free_keyval(comm_keyval);
}

int PMPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag)
{
    return fn.PMPI_Comm_get_attr(comm, comm_keyval, attribute_val, flag);
}

int PMPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val)
{
    return fn.PMPI_Comm_set_attr(comm, comm_keyval, attribute_val);
}

int PMPI_Keyval_create(MPI_Copy_function *copy_fn, 
		      MPI_Delete_function *delete_fn, 
		      int *keyval, void *extra_state)
{
    return fn.PMPI_Keyval_create(copy_fn, delete_fn, keyval, extra_state);
}

int PMPI_Keyval_free(int *keyval)
{
    return fn.PMPI_Keyval_free(keyval);
}

int PMPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn, 
			   MPI_Type_delete_attr_function *type_delete_attr_fn,
			   int *type_keyval, void *extra_state)
{
    return fn.PMPI_Type_create_keyval(type_copy_attr_fn, type_delete_attr_fn, type_keyval, extra_state);
}

int PMPI_Type_delete_attr(MPI_Datatype type, int type_keyval)
{
    return fn.PMPI_Type_delete_attr(type, type_keyval);
}

int PMPI_Type_free_keyval(int *type_keyval)
{
    return fn.PMPI_Type_free_keyval(type_keyval);
}

int PMPI_Type_get_attr(MPI_Datatype type, int type_keyval, void *attribute_val, int *flag)
{
    return fn.PMPI_Type_get_attr(type, type_keyval, attribute_val, flag);
}

int PMPI_Type_set_attr(MPI_Datatype type, int type_keyval, void *attribute_val)
{
    return fn.PMPI_Type_set_attr(type, type_keyval, attribute_val);
}

int PMPI_Win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn, 
			  MPI_Win_delete_attr_function *win_delete_attr_fn, 
			  int *win_keyval, void *extra_state)
{
    return fn.PMPI_Win_create_keyval(win_copy_attr_fn, win_delete_attr_fn, win_keyval, extra_state);
}

int PMPI_Win_delete_attr(MPI_Win win, int win_keyval)
{
    return fn.PMPI_Win_delete_attr(win, win_keyval);
}

int PMPI_Win_free_keyval(int *win_keyval)
{
    return fn.PMPI_Win_free_keyval(win_keyval);
}

int PMPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, 
		     int *flag)
{
    return fn.PMPI_Win_get_attr(win, win_keyval, attribute_val, flag);
}

int PMPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val)
{
    return fn.PMPI_Win_set_attr(win, win_keyval, attribute_val);
}

int PMPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                  void *recvbuf, int recvcount, MPI_Datatype recvtype, 
                  MPI_Comm comm)
{
    return fn.PMPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

int PMPI_Allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                   void *recvbuf, int *recvcounts, int *displs, 
                   MPI_Datatype recvtype, MPI_Comm comm)
{
    return fn.PMPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
}

int PMPI_Allreduce ( void *sendbuf, void *recvbuf, int count, 
		    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm )
{
    return fn.PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
}

int PMPI_Alltoall(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, 
                 MPI_Comm comm)
{
    return fn.PMPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

int PMPI_Alltoallv(void *sendbuf, int *sendcnts, int *sdispls, 
                  MPI_Datatype sendtype, void *recvbuf, int *recvcnts, 
                  int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
    return fn.PMPI_Alltoallv(sendbuf, sendcnts, sdispls, sendtype, recvbuf, recvcnts, rdispls, recvtype, comm);
}

int PMPI_Alltoallw(void *sendbuf, int *sendcnts, int *sdispls, 
                  MPI_Datatype *sendtypes, void *recvbuf, int *recvcnts, 
                  int *rdispls, MPI_Datatype *recvtypes, MPI_Comm comm)
{
    return fn.PMPI_Alltoallw(sendbuf, sendcnts, sdispls, sendtypes, recvbuf, recvcnts, rdispls, recvtypes, comm);
}

int PMPI_Barrier( MPI_Comm comm )
{
    return fn.PMPI_Barrier(comm);
}

int PMPI_Bcast( void *buffer, int count, MPI_Datatype datatype, int root, 
               MPI_Comm comm )
{
    return fn.PMPI_Bcast(buffer, count, datatype, root, comm);
}

int PMPI_Exscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, 
               MPI_Op op, MPI_Comm comm)
{
    return fn.PMPI_Exscan(sendbuf, recvbuf, count, datatype, op, comm);
}

int PMPI_Gather(void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
               void *recvbuf, int recvcnt, MPI_Datatype recvtype, 
               int root, MPI_Comm comm)
{
    return fn.PMPI_Gather(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
}

int PMPI_Gatherv(void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
                void *recvbuf, int *recvcnts, int *displs, 
                MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    return fn.PMPI_Gatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcnts, displs, recvtype, root, comm);
}

int PMPI_Op_create(MPI_User_function *function, int commute, MPI_Op *op)
{
    return fn.PMPI_Op_create(function, commute, op);
}

int PMPI_Op_free(MPI_Op *op)
{
    return fn.PMPI_Op_free(op);
}

int PMPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, 
	       MPI_Op op, int root, MPI_Comm comm)
{
    return fn.PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
}

int PMPI_Reduce_scatter(void *sendbuf, void *recvbuf, int *recvcnts, 
		       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    return fn.PMPI_Reduce_scatter(sendbuf, recvbuf, recvcnts, datatype, op, comm);
}

int PMPI_Scan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, 
	     MPI_Op op, MPI_Comm comm)
{
    return fn.PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
}

int PMPI_Scatter(void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
		void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root, 
		MPI_Comm comm)
{
    return fn.PMPI_Scatter(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
}

int PMPI_Scatterv( void *sendbuf, int *sendcnts, int *displs, 
		  MPI_Datatype sendtype, void *recvbuf, int recvcnt,
		  MPI_Datatype recvtype,
		  int root, MPI_Comm comm)
{
    return fn.PMPI_Scatterv(sendbuf, sendcnts, displs, sendtype, recvbuf, recvcnt, recvtype, root, comm);
}

int PMPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
    return fn.PMPI_Comm_compare(comm1, comm2, result);
}

int PMPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
    return fn.PMPI_Comm_create(comm, group, newcomm);
}

int PMPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
    return fn.PMPI_Comm_dup(comm, newcomm);
}

int PMPI_Comm_free(MPI_Comm *comm)
{
    return fn.PMPI_Comm_free(comm);
}

int PMPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen)
{
    return fn.PMPI_Comm_get_name(comm, comm_name, resultlen);
}

int PMPI_Comm_group(MPI_Comm comm, MPI_Group *group)
{
    return fn.PMPI_Comm_group(comm, group);
}

int PMPI_Comm_rank( MPI_Comm comm, int *rank )
{
    return fn.PMPI_Comm_rank(comm, rank);
}

int PMPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group)
{
    return fn.PMPI_Comm_remote_group(comm, group);
}

int PMPI_Comm_remote_size(MPI_Comm comm, int *size)
{
    return fn.PMPI_Comm_remote_size(comm, size);
}

int PMPI_Comm_set_name(MPI_Comm comm, char *comm_name)
{
    return fn.PMPI_Comm_set_name(comm, comm_name);
}

int PMPI_Comm_size( MPI_Comm comm, int *size )
{
    return fn.PMPI_Comm_size(comm, size);
}

int PMPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
    return fn.PMPI_Comm_split(comm, color, key, newcomm);
}

int PMPI_Comm_test_inter(MPI_Comm comm, int *flag)
{
    return fn.PMPI_Comm_test_inter(comm, flag);
}

int PMPI_Intercomm_create(MPI_Comm local_comm, int local_leader, 
			 MPI_Comm peer_comm, int remote_leader, int tag, 
			 MPI_Comm *newintercomm)
{
    return fn.PMPI_Intercomm_create(local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm);
}

int PMPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm)
{
    return fn.PMPI_Intercomm_merge(intercomm, high, newintracomm);
}

int PMPI_Address( void *location, MPI_Aint *address )
{
    return fn.PMPI_Address(location, address);
}

int PMPI_Get_address(void *location, MPI_Aint *address)
{
    return fn.PMPI_Get_address(location, address);
}

int PMPI_Get_count( MPI_Status *status, 	MPI_Datatype datatype, int *count )
{
    return fn.PMPI_Get_count(status, datatype, count);
}

int PMPI_Get_elements(MPI_Status *status, MPI_Datatype datatype, int *elements)
{
    return fn.PMPI_Get_elements(status, datatype, elements);
}

int PMPI_Pack(void *inbuf,
	     int incount,
	     MPI_Datatype datatype,
	     void *outbuf, 
	     int outcount,
	     int *position,
	     MPI_Comm comm)
{
    return fn.PMPI_Pack(inbuf, incount, datatype, outbuf, outcount, position, comm);
}

int PMPI_Pack_external(char *datarep,
		      void *inbuf,
		      int incount,
		      MPI_Datatype datatype,
		      void *outbuf,
		      MPI_Aint outcount,
		      MPI_Aint *position)
{
    return fn.PMPI_Pack_external(datarep, inbuf, incount, datatype, outbuf, outcount, position);
}

int PMPI_Pack_external_size(char *datarep,
			   int incount,
			   MPI_Datatype datatype,
			   MPI_Aint *size)
{
    return fn.PMPI_Pack_external_size(datarep, incount, datatype, size);
}

int PMPI_Pack_size(int incount,
		  MPI_Datatype datatype,
		  MPI_Comm comm,
		  int *size)
{
    return fn.PMPI_Pack_size(incount, datatype, comm, size);
}

int PMPI_Register_datarep(char *datarep, 
			 MPI_Datarep_conversion_function *read_conversion_fn, 
			 MPI_Datarep_conversion_function *write_conversion_fn, 
			 MPI_Datarep_extent_function *dtype_file_extent_fn, 
			 void *extra_state)
{
    return fn.PMPI_Register_datarep(datarep, read_conversion_fn, write_conversion_fn, dtype_file_extent_fn, extra_state);
}

int PMPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype, 
			    int count)
{
    return fn.PMPI_Status_set_elements(status, datatype, count);
}

int PMPI_Type_commit(MPI_Datatype *datatype)
{
    return fn.PMPI_Type_commit(datatype);
}

int PMPI_Type_contiguous(int count,
			MPI_Datatype old_type,
			MPI_Datatype *new_type_p)
{
    return fn.PMPI_Type_contiguous(count, old_type, new_type_p);
}

int PMPI_Type_create_darray(int size,
			   int rank,
			   int ndims,
			   int array_of_gsizes[],
			   int array_of_distribs[],
			   int array_of_dargs[],
			   int array_of_psizes[],
			   int order,
			   MPI_Datatype oldtype,
			   MPI_Datatype *newtype)
{
    return fn.PMPI_Type_create_darray(size, rank, ndims, array_of_gsizes, array_of_distribs, array_of_dargs, array_of_psizes, order, oldtype, newtype);
}

int PMPI_Type_create_hindexed(int count,
			     int blocklengths[],
			     MPI_Aint displacements[],
			     MPI_Datatype oldtype,
			     MPI_Datatype *newtype)
{
    return fn.PMPI_Type_create_hindexed(count, blocklengths, displacements, oldtype, newtype);
}

int PMPI_Type_create_hvector(int count,
			    int blocklength,
			    MPI_Aint stride,
			    MPI_Datatype oldtype,
			    MPI_Datatype *newtype)
{
    return fn.PMPI_Type_create_hvector(count, blocklength, stride, oldtype, newtype);
}

int PMPI_Type_create_indexed_block(int count,
				  int blocklength,
				  int array_of_displacements[],
				  MPI_Datatype oldtype,
				  MPI_Datatype *newtype)
{
    return fn.PMPI_Type_create_indexed_block(count, blocklength, array_of_displacements, oldtype, newtype);
}

int PMPI_Type_create_resized(MPI_Datatype oldtype,
			    MPI_Aint lb,
			    MPI_Aint extent,
			    MPI_Datatype *newtype)
{
    return fn.PMPI_Type_create_resized(oldtype, lb, extent, newtype);
}

int PMPI_Type_create_struct(int count,
			   int array_of_blocklengths[],
			   MPI_Aint array_of_displacements[],
			   MPI_Datatype array_of_types[],
			   MPI_Datatype *newtype)
{
    return fn.PMPI_Type_create_struct(count, array_of_blocklengths, array_of_displacements, array_of_types, newtype);
}

int PMPI_Type_create_subarray(int ndims,
			     int array_of_sizes[],
			     int array_of_subsizes[],
			     int array_of_starts[],
			     int order,
			     MPI_Datatype oldtype,
			     MPI_Datatype *newtype)
{
    return fn.PMPI_Type_create_subarray(ndims, array_of_sizes, array_of_subsizes, array_of_starts, order, oldtype, newtype);
}

int PMPI_Type_dup(MPI_Datatype datatype, MPI_Datatype *newtype)
{
    return fn.PMPI_Type_dup(datatype, newtype);
}

int PMPI_Type_extent(MPI_Datatype datatype, MPI_Aint *extent)
{
    return fn.PMPI_Type_extent(datatype, extent);
}

int PMPI_Type_free(MPI_Datatype *datatype)
{
    return fn.PMPI_Type_free(datatype);
}

int PMPI_Type_get_contents(MPI_Datatype datatype,
			  int max_integers,
			  int max_addresses,
			  int max_datatypes,
			  int array_of_integers[],
			  MPI_Aint array_of_addresses[],
			  MPI_Datatype array_of_datatypes[])
{
    return fn.PMPI_Type_get_contents(datatype, max_integers, max_addresses, max_datatypes, array_of_integers, array_of_addresses, array_of_datatypes);
}

int PMPI_Type_get_envelope(MPI_Datatype datatype,
			  int *num_integers,
			  int *num_addresses,
			  int *num_datatypes,
			  int *combiner)
{
    return fn.PMPI_Type_get_envelope(datatype, num_integers, num_addresses, num_datatypes, combiner);
}

int PMPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent)
{
    return fn.PMPI_Type_get_extent(datatype, lb, extent);
}

int PMPI_Type_get_name(MPI_Datatype datatype, char *type_name, int *resultlen)
{
    return fn.PMPI_Type_get_name(datatype, type_name, resultlen);
}

int PMPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb, 
			     MPI_Aint *true_extent)
{
    return fn.PMPI_Type_get_true_extent(datatype, true_lb, true_extent);
}

int PMPI_Type_hindexed(int count,
		      int blocklens[],
		      MPI_Aint indices[],
		      MPI_Datatype old_type,
		      MPI_Datatype *newtype)
{
    return fn.PMPI_Type_hindexed(count, blocklens, indices, old_type, newtype);
}

int PMPI_Type_hvector(int count,
		     int blocklen,
		     MPI_Aint stride,
		     MPI_Datatype old_type,
		     MPI_Datatype *newtype_p)
{
    return fn.PMPI_Type_hvector(count, blocklen, stride, old_type, newtype_p);
}

int PMPI_Type_indexed(int count,
		     int blocklens[],
		     int indices[],
		     MPI_Datatype old_type,
		     MPI_Datatype *newtype)
{
    return fn.PMPI_Type_indexed(count, blocklens, indices, old_type, newtype);
}

int PMPI_Type_lb(MPI_Datatype datatype, MPI_Aint *displacement)
{
    return fn.PMPI_Type_lb(datatype, displacement);
}

int PMPI_Type_match_size(int typeclass, int size, MPI_Datatype *datatype)
{
    return fn.PMPI_Type_match_size(typeclass, size, datatype);
}

int PMPI_Type_set_name(MPI_Datatype type, char *type_name)
{
    return fn.PMPI_Type_set_name(type, type_name);
}

int PMPI_Type_size(MPI_Datatype datatype, int *size)
{
    return fn.PMPI_Type_size(datatype, size);
}

int PMPI_Type_struct(int count,
		    int blocklens[],
		    MPI_Aint indices[],
		    MPI_Datatype old_types[],
		    MPI_Datatype *newtype)
{
    return fn.PMPI_Type_struct(count, blocklens, indices, old_types, newtype);
}

int PMPI_Type_ub(MPI_Datatype datatype, MPI_Aint *displacement)
{
    return fn.PMPI_Type_ub(datatype, displacement);
}

int PMPI_Type_vector(int count,
		    int blocklength,
		    int stride, 
		    MPI_Datatype old_type,
		    MPI_Datatype *newtype_p)
{
    return fn.PMPI_Type_vector(count, blocklength, stride, old_type, newtype_p);
}

int PMPI_Unpack(void *inbuf,
	       int insize,
	       int *position,
	       void *outbuf,
	       int outcount,
	       MPI_Datatype datatype,
	       MPI_Comm comm)
{
    return fn.PMPI_Unpack(inbuf, insize, position, outbuf, outcount, datatype, comm);
}

int PMPI_Unpack_external(char *datarep,
			void *inbuf,
			MPI_Aint insize,
			MPI_Aint *position,
			void *outbuf,
			int outcount,
			MPI_Datatype datatype)
{
    return fn.PMPI_Unpack_external(datarep, inbuf, insize, position, outbuf, outcount, datatype);
}

int PMPI_Add_error_class(int *errorclass)
{
    return fn.PMPI_Add_error_class(errorclass);
}

int PMPI_Add_error_code(int errorclass, int *errorcode)
{
    return fn.PMPI_Add_error_code(errorclass, errorcode);
}

int PMPI_Add_error_string(int errorcode, char *string)
{
    return fn.PMPI_Add_error_string(errorcode, string);
}

int PMPI_Comm_call_errhandler(MPI_Comm comm, int errorcode)
{
    return fn.PMPI_Comm_call_errhandler(comm, errorcode);
}

int PMPI_Comm_create_errhandler(MPI_Comm_errhandler_fn *function, 
                               MPI_Errhandler *errhandler)
{
    return fn.PMPI_Comm_create_errhandler(function, errhandler);
}

int PMPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler)
{
    return fn.PMPI_Comm_get_errhandler(comm, errhandler);
}

int PMPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler)
{
    return fn.PMPI_Comm_set_errhandler(comm, errhandler);
}

int PMPI_Errhandler_create(MPI_Handler_function *function, 
                          MPI_Errhandler *errhandler)
{
    return fn.PMPI_Errhandler_create(function, errhandler);
}

int PMPI_Errhandler_free(MPI_Errhandler *errhandler)
{
    return fn.PMPI_Errhandler_free(errhandler);
}

int PMPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler *errhandler)
{
    return fn.PMPI_Errhandler_get(comm, errhandler);
}

int PMPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler)
{
    return fn.PMPI_Errhandler_set(comm, errhandler);
}

int PMPI_Error_class(int errorcode, int *errorclass)
{
    return fn.PMPI_Error_class(errorcode, errorclass);
}

int PMPI_Error_string(int errorcode, char *string, int *resultlen)
{
    return fn.PMPI_Error_string(errorcode, string, resultlen);
}

int PMPI_File_call_errhandler(MPI_File fh, int errorcode)
{
    return fn.PMPI_File_call_errhandler(fh, errorcode);
}

int PMPI_File_create_errhandler(MPI_File_errhandler_fn *function, 
                               MPI_Errhandler *errhandler)
{
    return fn.PMPI_File_create_errhandler(function, errhandler);
}

int PMPI_File_get_errhandler(MPI_File file, MPI_Errhandler *errhandler)
{
    return fn.PMPI_File_get_errhandler(file, errhandler);
}

int PMPI_File_set_errhandler(MPI_File file, MPI_Errhandler errhandler)
{
    return fn.PMPI_File_set_errhandler(file, errhandler);
}

int PMPI_Win_call_errhandler(MPI_Win win, int errorcode)
{
    return fn.PMPI_Win_call_errhandler(win, errorcode);
}

int PMPI_Win_create_errhandler(MPI_Win_errhandler_fn *function, 
			      MPI_Errhandler *errhandler)
{
    return fn.PMPI_Win_create_errhandler(function, errhandler);
}

int PMPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler *errhandler)
{
    return fn.PMPI_Win_get_errhandler(win, errhandler);
}

int PMPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler)
{
    return fn.PMPI_Win_set_errhandler(win, errhandler);
}

int PMPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result)
{
    return fn.PMPI_Group_compare(group1, group2, result);
}

int PMPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup)
{
    return fn.PMPI_Group_difference(group1, group2, newgroup);
}

int PMPI_Group_excl(MPI_Group group, int n, int *ranks, MPI_Group *newgroup)
{
    return fn.PMPI_Group_excl(group, n, ranks, newgroup);
}

int PMPI_Group_free(MPI_Group *group)
{
    return fn.PMPI_Group_free(group);
}

int PMPI_Group_incl(MPI_Group group, int n, int *ranks, MPI_Group *newgroup)
{
    return fn.PMPI_Group_incl(group, n, ranks, newgroup);
}

int PMPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup)
{
    return fn.PMPI_Group_intersection(group1, group2, newgroup);
}

int PMPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], 
                         MPI_Group *newgroup)
{
    return fn.PMPI_Group_range_excl(group, n, ranges, newgroup);
}

int PMPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], 
                         MPI_Group *newgroup)
{
    return fn.PMPI_Group_range_incl(group, n, ranges, newgroup);
}

int PMPI_Group_rank(MPI_Group group, int *rank)
{
    return fn.PMPI_Group_rank(group, rank);
}

int PMPI_Group_size(MPI_Group group, int *size)
{
    return fn.PMPI_Group_size(group, size);
}

int PMPI_Group_translate_ranks(MPI_Group group1, int n, int *ranks1, MPI_Group group2, int *ranks2)
{
    return fn.PMPI_Group_translate_ranks(group1, n, ranks1, group2, ranks2);
}

int PMPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup)
{
    return fn.PMPI_Group_union(group1, group2, newgroup);
}

int PMPI_Abort(MPI_Comm comm, int errorcode)
{
    return fn.PMPI_Abort(comm, errorcode);
}

int PMPI_Finalize( void )
{
    return fn.PMPI_Finalize();
}

int PMPI_Finalized( int *flag )
{
    return fn.PMPI_Finalized(flag);
}

int PMPI_Initialized( int *flag )
{
    return fn.PMPI_Initialized(flag);
}

int PMPI_Is_thread_main( int *flag )
{
    return fn.PMPI_Is_thread_main(flag);
}

int PMPI_Query_thread( int *provided )
{
    return fn.PMPI_Query_thread(provided);
}

int PMPI_Get_processor_name( char *name, int *resultlen )
{
    return fn.PMPI_Get_processor_name(name, resultlen);
}

int PMPI_Pcontrol(const int level, ...)
{
    int ret_val;
    va_list list;

    va_start(list, level);
    ret_val = fn.PMPI_Pcontrol(level, list);
    va_end(list);
    return ret_val;
}

int PMPI_Get_version( int *version, int *subversion )
{
    return fn.PMPI_Get_version(version, subversion);
}

int PMPI_Bsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, 
	      MPI_Comm comm)
{
    return fn.PMPI_Bsend(buf, count, datatype, dest, tag, comm);
}

int PMPI_Bsend_init(void *buf, int count, MPI_Datatype datatype, 
                   int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    return fn.PMPI_Bsend_init(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Buffer_attach(void *buffer, int size)
{
    return fn.PMPI_Buffer_attach(buffer, size);
}

int PMPI_Buffer_detach(void *buffer, int *size)
{
    return fn.PMPI_Buffer_detach(buffer, size);
}

int PMPI_Cancel(MPI_Request *request)
{
    return fn.PMPI_Cancel(request);
}

int PMPI_Grequest_complete( MPI_Request request )
{
    return fn.PMPI_Grequest_complete(request);
}

int PMPI_Grequest_start( MPI_Grequest_query_function *query_fn, 
			MPI_Grequest_free_function *free_fn, 
			MPI_Grequest_cancel_function *cancel_fn, 
			void *extra_state, MPI_Request *request )
{
    return fn.PMPI_Grequest_start(query_fn, free_fn, cancel_fn, extra_state, request);
}

int PMPI_Ibsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, 
	       MPI_Comm comm, MPI_Request *request)
{
    return fn.PMPI_Ibsend(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, 
	       MPI_Status *status)
{
    return fn.PMPI_Iprobe(source, tag, comm, flag, status);
}

int PMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source,
	      int tag, MPI_Comm comm, MPI_Request *request)
{
    return fn.PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
}

int PMPI_Irsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	       MPI_Comm comm, MPI_Request *request)
{
    return fn.PMPI_Irsend(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	      MPI_Comm comm, MPI_Request *request)
{
    return fn.PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Issend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	       MPI_Comm comm, MPI_Request *request)
{
    return fn.PMPI_Issend(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status)
{
    return fn.PMPI_Probe(source, tag, comm, status);
}

int PMPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
	     MPI_Comm comm, MPI_Status *status)
{
    return fn.PMPI_Recv(buf, count, datatype, source, tag, comm, status);
}

int PMPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source, 
		  int tag, MPI_Comm comm, MPI_Request *request)
{
    return fn.PMPI_Recv_init(buf, count, datatype, source, tag, comm, request);
}

int PMPI_Request_free(MPI_Request *request)
{
    return fn.PMPI_Request_free(request);
}

int PMPI_Request_get_status(MPI_Request request, int *flag, MPI_Status *status)
{
    return fn.PMPI_Request_get_status(request, flag, status);
}

int PMPI_Rsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	      MPI_Comm comm)
{
    return fn.PMPI_Rsend(buf, count, datatype, dest, tag, comm);
}

int PMPI_Rsend_init(void *buf, int count, MPI_Datatype datatype, int dest,
		   int tag, MPI_Comm comm, MPI_Request *request)
{
    return fn.PMPI_Rsend_init(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm)
{
    return fn.PMPI_Send(buf, count, datatype, dest, tag, comm);
}

int PMPI_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
		 int dest, int sendtag,
		 void *recvbuf, int recvcount, MPI_Datatype recvtype, 
		 int source, int recvtag,
		 MPI_Comm comm, MPI_Status *status)
{
    return fn.PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status);
}

int PMPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, 
			 int dest, int sendtag, int source, int recvtag,
			 MPI_Comm comm, MPI_Status *status)
{
    return fn.PMPI_Sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, comm, status);
}

int PMPI_Send_init(void *buf, int count, MPI_Datatype datatype, int dest,
		  int tag, MPI_Comm comm, MPI_Request *request)
{
    return fn.PMPI_Send_init(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Ssend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	      MPI_Comm comm)
{
    return fn.PMPI_Ssend(buf, count, datatype, dest, tag, comm);
}

int PMPI_Ssend_init(void *buf, int count, MPI_Datatype datatype, int dest,
		   int tag, MPI_Comm comm, MPI_Request *request)
{
    return fn.PMPI_Ssend_init(buf, count, datatype, dest, tag, comm, request);
}

int PMPI_Start(MPI_Request *request)
{
    return fn.PMPI_Start(request);
}

int PMPI_Startall(int count, MPI_Request array_of_requests[])
{
    return fn.PMPI_Startall(count, array_of_requests);
}

int PMPI_Status_set_cancelled(MPI_Status *status, int flag)
{
    return fn.PMPI_Status_set_cancelled(status, flag);
}

int PMPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
{
    return fn.PMPI_Test(request, flag, status);
}

int PMPI_Testall(int count, MPI_Request array_of_requests[], int *flag, 
		MPI_Status array_of_statuses[])
{
    return fn.PMPI_Testall(count, array_of_requests, flag, array_of_statuses);
}

int PMPI_Testany(int count, MPI_Request array_of_requests[], int *index, 
		int *flag, MPI_Status *status)
{
    return fn.PMPI_Testany(count, array_of_requests, index, flag, status);
}

int PMPI_Testsome(int incount, MPI_Request array_of_requests[], int *outcount, 
		 int array_of_indices[], MPI_Status array_of_statuses[])
{
    return fn.PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
}

int PMPI_Test_cancelled(MPI_Status *status, int *flag)
{
    return fn.PMPI_Test_cancelled(status, flag);
}

int PMPI_Wait(MPI_Request *request, MPI_Status *status)
{
    return fn.PMPI_Wait(request, status);
}

int PMPI_Waitall(int count, MPI_Request array_of_requests[], 
		MPI_Status array_of_statuses[])
{
    return fn.PMPI_Waitall(count, array_of_requests, array_of_statuses);
}

int PMPI_Waitany(int count, MPI_Request array_of_requests[], int *index, 
		MPI_Status *status)
{
    return fn.PMPI_Waitany(count, array_of_requests, index, status);
}

int PMPI_Waitsome(int incount, MPI_Request array_of_requests[], 
		 int *outcount, int array_of_indices[],
		 MPI_Status array_of_statuses[])
{
    return fn.PMPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
}

int PMPI_Accumulate(void *origin_addr, int origin_count, MPI_Datatype
                   origin_datatype, int target_rank, MPI_Aint
                   target_disp, int target_count, MPI_Datatype
                   target_datatype, MPI_Op op, MPI_Win win)
{
    return fn.PMPI_Accumulate(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, op, win);
}

int PMPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr)
{
    return fn.PMPI_Alloc_mem(size, info, baseptr);
}

int PMPI_Free_mem(void *base)
{
    return fn.PMPI_Free_mem(base);
}

int PMPI_Get(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPI_Win
            win)
{
    return fn.PMPI_Get(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win);
}

int PMPI_Put(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPI_Win
            win)
{
    return fn.PMPI_Put(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win);
}

int PMPI_Win_complete(MPI_Win win)
{
    return fn.PMPI_Win_complete(win);
}

int PMPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, 
		   MPI_Comm comm, MPI_Win *win)
{
    return fn.PMPI_Win_create(base, size, disp_unit, info, comm, win);
}

int PMPI_Win_fence(int assert, MPI_Win win)
{
    return fn.PMPI_Win_fence(assert, win);
}

int PMPI_Win_free(MPI_Win *win)
{
    return fn.PMPI_Win_free(win);
}

int PMPI_Win_get_group(MPI_Win win, MPI_Group *group)
{
    return fn.PMPI_Win_get_group(win, group);
}

int PMPI_Win_get_name(MPI_Win win, char *win_name, int *resultlen)
{
    return fn.PMPI_Win_get_name(win, win_name, resultlen);
}

int PMPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win)
{
    return fn.PMPI_Win_lock(lock_type, rank, assert, win);
}

int PMPI_Win_post(MPI_Group group, int assert, MPI_Win win)
{
    return fn.PMPI_Win_post(group, assert, win);
}

int PMPI_Win_set_name(MPI_Win win, char *win_name)
{
    return fn.PMPI_Win_set_name(win, win_name);
}

int PMPI_Win_start(MPI_Group group, int assert, MPI_Win win)
{
    return fn.PMPI_Win_start(group, assert, win);
}

int PMPI_Win_test(MPI_Win win, int *flag)
{
    return fn.PMPI_Win_test(win, flag);
}

int PMPI_Win_unlock(int rank, MPI_Win win)
{
    return fn.PMPI_Win_unlock(rank, win);
}

int PMPI_Win_wait(MPI_Win win)
{
    return fn.PMPI_Win_wait(win);
}

int PMPI_File_close(MPI_File *mpi_fh)
{
    return fn.PMPI_File_close(mpi_fh);
}

int PMPI_File_delete(char *filename, MPI_Info info)
{
    return fn.PMPI_File_delete(filename, info);
}

MPI_Fint PMPI_File_c2f(MPI_File mpi_fh)
{
    return fn.PMPI_File_c2f(mpi_fh);
}

MPI_File PMPI_File_f2c(MPI_Fint i)
{
    return fn.PMPI_File_f2c(i);
}

int PMPI_File_sync(MPI_File mpi_fh)
{
    return fn.PMPI_File_sync(mpi_fh);
}

int PMPI_File_get_amode(MPI_File mpi_fh, int *amode)
{
    return fn.PMPI_File_get_amode(mpi_fh, amode);
}

int PMPI_File_get_atomicity(MPI_File mpi_fh, int *flag)
{
    return fn.PMPI_File_get_atomicity(mpi_fh, flag);
}

int PMPI_File_get_byte_offset(MPI_File mpi_fh,
			     MPI_Offset offset,
			     MPI_Offset *disp)
{
    return fn.PMPI_File_get_byte_offset(mpi_fh, offset, disp);
}

int PMPI_File_get_type_extent(MPI_File mpi_fh, MPI_Datatype datatype, 
                             MPI_Aint *extent)
{
    return fn.PMPI_File_get_type_extent(mpi_fh, datatype, extent);
}

int PMPI_File_get_group(MPI_File mpi_fh, MPI_Group *group)
{
    return fn.PMPI_File_get_group(mpi_fh, group);
}

int PMPI_File_get_info(MPI_File mpi_fh, MPI_Info *info_used)
{
    return fn.PMPI_File_get_info(mpi_fh, info_used);
}

int PMPI_File_get_position(MPI_File mpi_fh, MPI_Offset *offset)
{
    return fn.PMPI_File_get_position(mpi_fh, offset);
}

int PMPI_File_get_position_shared(MPI_File mpi_fh, MPI_Offset *offset)
{
    return fn.PMPI_File_get_position_shared(mpi_fh, offset);
}

int PMPI_File_get_size(MPI_File mpi_fh, MPI_Offset *size)
{
    return fn.PMPI_File_get_size(mpi_fh, size);
}

int PMPI_File_get_view(MPI_File mpi_fh,
		      MPI_Offset *disp,
		      MPI_Datatype *etype,
		      MPI_Datatype *filetype,
		      char *datarep)
{
    return fn.PMPI_File_get_view(mpi_fh, disp, etype, filetype, datarep);
}

int PMPI_File_iread(MPI_File mpi_fh, void *buf, int count, 
		   MPI_Datatype datatype, MPIO_Request *request)
{
    return fn.PMPI_File_iread(mpi_fh, buf, count, datatype, request);
}

int PMPI_File_iread_at(MPI_File mpi_fh, MPI_Offset offset, void *buf,
                      int count, MPI_Datatype datatype, 
                      MPIO_Request *request)
{
    return fn.PMPI_File_iread_at(mpi_fh, offset, buf, count, datatype, request);
}

int PMPI_File_iread_shared(MPI_File mpi_fh, void *buf, int count, 
			  MPI_Datatype datatype, MPIO_Request *request)
{
    return fn.PMPI_File_iread_shared(mpi_fh, buf, count, datatype, request);
}

int PMPI_File_iwrite(MPI_File mpi_fh, void *buf, int count, 
		    MPI_Datatype datatype, MPIO_Request *request)
{
    return fn.PMPI_File_iwrite(mpi_fh, buf, count, datatype, request);
}

int PMPI_File_iwrite_at(MPI_File mpi_fh, MPI_Offset offset, void *buf,
                       int count, MPI_Datatype datatype, 
                       MPIO_Request *request)
{
    return fn.PMPI_File_iwrite_at(mpi_fh, offset, buf, count, datatype, request);
}

int PMPI_File_iwrite_shared(MPI_File mpi_fh, void *buf, int count, 
			   MPI_Datatype datatype, MPIO_Request *request)
{
    return fn.PMPI_File_iwrite_shared(mpi_fh, buf, count, datatype, request);
}

int PMPI_File_open(MPI_Comm comm, char *filename, int amode, 
                  MPI_Info info, MPI_File *fh)
{
    return fn.PMPI_File_open(comm, filename, amode, info, fh);
}

int PMPI_File_preallocate(MPI_File mpi_fh, MPI_Offset size)
{
    return fn.PMPI_File_preallocate(mpi_fh, size);
}

int PMPI_File_read_at_all_begin(MPI_File mpi_fh, MPI_Offset offset, void *buf,
			       int count, MPI_Datatype datatype)
{
    return fn.PMPI_File_read_at_all_begin(mpi_fh, offset, buf, count, datatype);
}

int PMPI_File_read_at_all_end(MPI_File mpi_fh, void *buf, MPI_Status *status)
{
    return fn.PMPI_File_read_at_all_end(mpi_fh, buf, status);
}

int PMPI_File_read(MPI_File mpi_fh, void *buf, int count, 
                  MPI_Datatype datatype, MPI_Status *status)
{
    return fn.PMPI_File_read(mpi_fh, buf, count, datatype, status);
}

int PMPI_File_read_all(MPI_File mpi_fh, void *buf, int count, 
                      MPI_Datatype datatype, MPI_Status *status)
{
    return fn.PMPI_File_read_all(mpi_fh, buf, count, datatype, status);
}

int PMPI_File_read_all_begin(MPI_File mpi_fh, void *buf, int count, 
                            MPI_Datatype datatype)
{
    return fn.PMPI_File_read_all_begin(mpi_fh, buf, count, datatype);
}

int PMPI_File_read_all_end(MPI_File mpi_fh, void *buf, MPI_Status *status)
{
    return fn.PMPI_File_read_all_end(mpi_fh, buf, status);
}

int PMPI_File_read_at(MPI_File mpi_fh, MPI_Offset offset, void *buf,
		     int count, MPI_Datatype datatype, MPI_Status *status)
{
    return fn.PMPI_File_read_at(mpi_fh, offset, buf, count, datatype, status);
}

int PMPI_File_read_at_all(MPI_File mpi_fh, MPI_Offset offset, void *buf,
                         int count, MPI_Datatype datatype, 
                         MPI_Status *status)
{
    return fn.PMPI_File_read_at_all(mpi_fh, offset, buf, count, datatype, status);
}

int PMPI_File_read_ordered(MPI_File mpi_fh, void *buf, int count, 
                          MPI_Datatype datatype, MPI_Status *status)
{
    return fn.PMPI_File_read_ordered(mpi_fh, buf, count, datatype, status);
}

int PMPI_File_read_ordered_begin(MPI_File mpi_fh, void *buf, int count, 
				MPI_Datatype datatype)
{
    return fn.PMPI_File_read_ordered_begin(mpi_fh, buf, count, datatype);
}

int PMPI_File_read_ordered_end(MPI_File mpi_fh, void *buf, MPI_Status *status)
{
    return fn.PMPI_File_read_ordered_end(mpi_fh, buf, status);
}

int PMPI_File_read_shared(MPI_File mpi_fh, void *buf, int count, 
			 MPI_Datatype datatype, MPI_Status *status)
{
    return fn.PMPI_File_read_shared(mpi_fh, buf, count, datatype, status);
}

int PMPI_File_seek(MPI_File mpi_fh, MPI_Offset offset, int whence)
{
    return fn.PMPI_File_seek(mpi_fh, offset, whence);
}

int PMPI_File_seek_shared(MPI_File mpi_fh, MPI_Offset offset, int whence)
{
    return fn.PMPI_File_seek_shared(mpi_fh, offset, whence);
}

int PMPI_File_set_atomicity(MPI_File mpi_fh, int flag)
{
    return fn.PMPI_File_set_atomicity(mpi_fh, flag);
}

int PMPI_File_set_info(MPI_File mpi_fh, MPI_Info info)
{
    return fn.PMPI_File_set_info(mpi_fh, info);
}

int PMPI_File_set_size(MPI_File mpi_fh, MPI_Offset size)
{
    return fn.PMPI_File_set_size(mpi_fh, size);
}

int PMPI_File_set_view(MPI_File mpi_fh, MPI_Offset disp, MPI_Datatype etype,
		      MPI_Datatype filetype, char *datarep, MPI_Info info)
{
    return fn.PMPI_File_set_view(mpi_fh, disp, etype, filetype, datarep, info);
}

int PMPI_File_write(MPI_File mpi_fh, void *buf, int count, 
                   MPI_Datatype datatype, MPI_Status *status)
{
    return fn.PMPI_File_write(mpi_fh, buf, count, datatype, status);
}

int PMPI_File_write_all(MPI_File mpi_fh, void *buf, int count, 
                       MPI_Datatype datatype, MPI_Status *status)
{
    return fn.PMPI_File_write_all(mpi_fh, buf, count, datatype, status);
}

int PMPI_File_write_all_begin(MPI_File mpi_fh, void *buf, int count, 
			     MPI_Datatype datatype)
{
    return fn.PMPI_File_write_all_begin(mpi_fh, buf, count, datatype);
}

int PMPI_File_write_all_end(MPI_File mpi_fh, void *buf, MPI_Status *status)
{
    return fn.PMPI_File_write_all_end(mpi_fh, buf, status);
}

int PMPI_File_write_at(MPI_File mpi_fh, MPI_Offset offset, void *buf,
                      int count, MPI_Datatype datatype, 
                      MPI_Status *status)
{
    return fn.PMPI_File_write_at(mpi_fh, offset, buf, count, datatype, status);
}

int PMPI_File_write_at_all(MPI_File mpi_fh, MPI_Offset offset, void *buf,
                          int count, MPI_Datatype datatype, 
                          MPI_Status *status)
{
    return fn.PMPI_File_write_at_all(mpi_fh, offset, buf, count, datatype, status);
}

int PMPI_File_write_ordered(MPI_File mpi_fh, void *buf, int count, 
			   MPI_Datatype datatype, MPI_Status *status)
{
    return fn.PMPI_File_write_ordered(mpi_fh, buf, count, datatype, status);
}

int PMPI_File_write_ordered_begin(MPI_File mpi_fh, void *buf, int count, 
				 MPI_Datatype datatype)
{
    return fn.PMPI_File_write_ordered_begin(mpi_fh, buf, count, datatype);
}

int PMPI_File_write_ordered_end(MPI_File mpi_fh, void *buf, MPI_Status *status)
{
    return fn.PMPI_File_write_ordered_end(mpi_fh, buf, status);
}

int PMPI_File_write_shared(MPI_File mpi_fh, void *buf, int count, 
                          MPI_Datatype datatype, MPI_Status *status)
{
    return fn.PMPI_File_write_shared(mpi_fh, buf, count, datatype, status);
}

int PMPI_File_write_at_all_begin(MPI_File mpi_fh, MPI_Offset offset, void *buf,
				int count, MPI_Datatype datatype)
{
    return fn.PMPI_File_write_at_all_begin(mpi_fh, offset, buf, count, datatype);
}

int PMPI_File_write_at_all_end(MPI_File mpi_fh, void *buf, MPI_Status *status)
{
    return fn.PMPI_File_write_at_all_end(mpi_fh, buf, status);
}

int PMPI_Info_create(MPI_Info *info)
{
    return fn.PMPI_Info_create(info);
}

int PMPI_Info_delete(MPI_Info info, char *key)
{
    return fn.PMPI_Info_delete(info, key);
}

int PMPI_Info_dup(MPI_Info info, MPI_Info *newinfo)
{
    return fn.PMPI_Info_dup(info, newinfo);
}

int PMPI_Info_free(MPI_Info *info)
{
    return fn.PMPI_Info_free(info);
}

int PMPI_Info_get(MPI_Info info, char *key, int valuelen, char *value, int *flag)
{
    return fn.PMPI_Info_get(info, key, valuelen, value, flag);
}

int PMPI_Info_get_nkeys(MPI_Info info, int *nkeys)
{
    return fn.PMPI_Info_get_nkeys(info, nkeys);
}

int PMPI_Info_get_nthkey(MPI_Info info, int n, char *key)
{
    return fn.PMPI_Info_get_nthkey(info, n, key);
}

int PMPI_Info_get_valuelen(MPI_Info info, char *key, int *valuelen, int *flag)
{
    return fn.PMPI_Info_get_valuelen(info, key, valuelen, flag);
}

int PMPI_Info_set(MPI_Info info, char *key, char *value)
{
    return fn.PMPI_Info_set(info, key, value);
}

int PMPI_Close_port(char *port_name)
{
    return fn.PMPI_Close_port(port_name);
}

int PMPI_Comm_accept(char *port_name, MPI_Info info, int root, MPI_Comm comm, 
                    MPI_Comm *newcomm)
{
    return fn.PMPI_Comm_accept(port_name, info, root, comm, newcomm);
}

int PMPI_Comm_connect(char *port_name, MPI_Info info, int root, MPI_Comm comm, 
                     MPI_Comm *newcomm)
{
    return fn.PMPI_Comm_connect(port_name, info, root, comm, newcomm);
}

int PMPI_Comm_disconnect(MPI_Comm * comm)
{
    return fn.PMPI_Comm_disconnect(comm);
}

int PMPI_Comm_get_parent(MPI_Comm *parent)
{
    return fn.PMPI_Comm_get_parent(parent);
}

int PMPI_Comm_join(int fd, MPI_Comm *intercomm)
{
    return fn.PMPI_Comm_join(fd, intercomm);
}

int PMPI_Comm_spawn(char *command, char *argv[], int maxprocs, MPI_Info info, 
		   int root, MPI_Comm comm, MPI_Comm *intercomm,
		   int array_of_errcodes[])
{
    return fn.PMPI_Comm_spawn(command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes);
}

int PMPI_Comm_spawn_multiple(int count, char *array_of_commands[], char* *array_of_argv[], int array_of_maxprocs[], MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[])
{
    return fn.PMPI_Comm_spawn_multiple(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, root, comm, intercomm, array_of_errcodes);
}

int PMPI_Lookup_name(char *service_name, MPI_Info info, char *port_name)
{
    return fn.PMPI_Lookup_name(service_name, info, port_name);
}

int PMPI_Open_port(MPI_Info info, char *port_name)
{
    return fn.PMPI_Open_port(info, port_name);
}

int PMPI_Publish_name(char *service_name, MPI_Info info, char *port_name)
{
    return fn.PMPI_Publish_name(service_name, info, port_name);
}

int PMPI_Unpublish_name(char *service_name, MPI_Info info, char *port_name)
{
    return fn.PMPI_Unpublish_name(service_name, info, port_name);
}

int PMPI_Cartdim_get(MPI_Comm comm, int *ndims)
{
    return fn.PMPI_Cartdim_get(comm, ndims);
}

int PMPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int *coords)
{
    return fn.PMPI_Cart_coords(comm, rank, maxdims, coords);
}

int PMPI_Cart_create(MPI_Comm comm_old, int ndims, int *dims, int *periods, 
		    int reorder, MPI_Comm *comm_cart)
{
    return fn.PMPI_Cart_create(comm_old, ndims, dims, periods, reorder, comm_cart);
}

int PMPI_Cart_get(MPI_Comm comm, int maxdims, int *dims, int *periods, 
                 int *coords)
{
    return fn.PMPI_Cart_get(comm, maxdims, dims, periods, coords);
}

int PMPI_Cart_map(MPI_Comm comm_old, int ndims, int *dims, int *periods, 
		 int *newrank)
{
    return fn.PMPI_Cart_map(comm_old, ndims, dims, periods, newrank);
}

int PMPI_Cart_rank(MPI_Comm comm, int *coords, int *rank)
{
    return fn.PMPI_Cart_rank(comm, coords, rank);
}

int PMPI_Cart_shift(MPI_Comm comm, int direction, int displ, int *source, 
		   int *dest)
{
    return fn.PMPI_Cart_shift(comm, direction, displ, source, dest);
}

int PMPI_Cart_sub(MPI_Comm comm, int *remain_dims, MPI_Comm *comm_new)
{
    return fn.PMPI_Cart_sub(comm, remain_dims, comm_new);
}

int PMPI_Dims_create(int nnodes, int ndims, int *dims)
{
    return fn.PMPI_Dims_create(nnodes, ndims, dims);
}

int PMPI_Graph_create(MPI_Comm comm_old, int nnodes, int *index, int *edges, 
		     int reorder, MPI_Comm *comm_graph)
{
    return fn.PMPI_Graph_create(comm_old, nnodes, index, edges, reorder, comm_graph);
}

int PMPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges)
{
    return fn.PMPI_Graphdims_get(comm, nnodes, nedges);
}

int PMPI_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors)
{
    return fn.PMPI_Graph_neighbors_count(comm, rank, nneighbors);
}

int PMPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, 
                  int *index, int *edges)
{
    return fn.PMPI_Graph_get(comm, maxindex, maxedges, index, edges);
}

int PMPI_Graph_map(MPI_Comm comm_old, int nnodes, int *index, int *edges,
                  int *newrank)
{
    return fn.PMPI_Graph_map(comm_old, nnodes, index, edges, newrank);
}

int PMPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, 
			int *neighbors)
{
    return fn.PMPI_Graph_neighbors(comm, rank, maxneighbors, neighbors);
}

int PMPI_Topo_test(MPI_Comm comm, int *topo_type)
{
    return fn.PMPI_Topo_test(comm, topo_type);
}

double PMPI_Wtime()
{
    return fn.PMPI_Wtime();
}

double PMPI_Wtick()
{
    return fn.PMPI_Wtick();
}
