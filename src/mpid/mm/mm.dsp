# Microsoft Developer Studio Project File - Name="mm" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=mm - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mm.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mm.mak" CFG="mm - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mm - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "mm - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
F90=df.exe
RSC=rc.exe

!IF  "$(CFG)" == "mm - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE F90 /compile_only /include:"Release/" /nologo /warn:nofileopt
# ADD F90 /browser /compile_only /include:"Release/" /nologo /warn:nofileopt
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "include" /I "..\..\include" /I "..\common\datatype" /I "src\ib\paceline" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\lib\mpid_mm.lib"

!ELSEIF  "$(CFG)" == "mm - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE F90 /check:bounds /compile_only /debug:full /include:"Debug/" /nologo /traceback /warn:argument_checking /warn:nofileopt
# ADD F90 /browser /check:bounds /compile_only /debug:full /include:"Debug/" /nologo /traceback /warn:argument_checking /warn:nofileopt
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "include" /I "..\..\include" /I "..\common\datatype" /I "src\ib\paceline" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\lib\mpid_mmd.lib"

!ENDIF 

# Begin Target

# Name "mm - Win32 Release"
# Name "mm - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;f90;for;f;fpp"
# Begin Source File

SOURCE=.\src\util\block_allocator.c
# End Source File
# Begin Source File

SOURCE=.\src\bsocket\bsockets.c
# End Source File
# Begin Source File

SOURCE=.\src\util\carutil.c
# End Source File
# Begin Source File

SOURCE=..\..\util\dbg\dbg_printf.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ib_can_connect.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ib_get_business_card.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ib_init.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ib_make_progress.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ib_merge_unexpected_data.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ib_merge_with_posted.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ib_merge_with_unexpected.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ib_over_via.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ib_post_connect.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ib_post_read.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ib_post_write.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ib_queue.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ib_read.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ib_reset_car.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ib_setup_connections.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ib_write_aggressive.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ibu.c
# End Source File
# Begin Source File

SOURCE=.\src\ib\ibutil.c
# End Source File
# Begin Source File

SOURCE=.\src\mm\mm_accept.c
# End Source File
# Begin Source File

SOURCE=.\src\common\mm_buffer_util.c
# End Source File
# Begin Source File

SOURCE=.\src\common\mm_choose_buffer.c
# End Source File
# Begin Source File

SOURCE=.\src\mm\mm_close.c
# End Source File
# Begin Source File

SOURCE=.\src\mm\mm_close_port.c
# End Source File
# Begin Source File

SOURCE=.\src\mm\mm_connect.c
# End Source File
# Begin Source File

SOURCE=.\src\common\mm_cq_enqueue.c
# End Source File
# Begin Source File

SOURCE=.\src\common\mm_cq_test.c
# End Source File
# Begin Source File

SOURCE=.\src\common\mm_create_post_unex.c
# End Source File
# Begin Source File

SOURCE=.\src\util\mm_describe_states.c
# End Source File
# Begin Source File

SOURCE=.\src\common\mm_enqueue_request_to_send.c
# End Source File
# Begin Source File

SOURCE=.\src\util\mm_events.c
# End Source File
# Begin Source File

SOURCE=.\src\common\mm_get_vc.c
# End Source File
# Begin Source File

SOURCE=.\src\common\mm_make_progress.c
# End Source File
# Begin Source File

SOURCE=.\src\util\mm_mpe_prof.c
# End Source File
# Begin Source File

SOURCE=.\src\mm\mm_open_port.c
# End Source File
# Begin Source File

SOURCE=.\src\common\mm_post_recv.c
# End Source File
# Begin Source File

SOURCE=.\src\common\mm_post_rndv_clear_to_send.c
# End Source File
# Begin Source File

SOURCE=.\src\common\mm_post_rndv_data_send.c
# End Source File
# Begin Source File

SOURCE=.\src\common\mm_post_send.c
# End Source File
# Begin Source File

SOURCE=.\src\mm\mm_recv.c
# End Source File
# Begin Source File

SOURCE=.\src\common\mm_reset_cars.c
# End Source File
# Begin Source File

SOURCE=.\src\mm\mm_send.c
# End Source File
# Begin Source File

SOURCE=.\src\common\mm_vcutil.c
# End Source File
# Begin Source File

SOURCE=.\src\init\mpid_abort.c
# End Source File
# Begin Source File

SOURCE=.\src\rma\mpid_accumulate.c
# End Source File
# Begin Source File

SOURCE=.\src\pt2pt\mpid_bsend_init.c
# End Source File
# Begin Source File

SOURCE=.\src\spawn\mpid_close_port.c
# End Source File
# Begin Source File

SOURCE=.\src\spawn\mpid_comm_accept.c
# End Source File
# Begin Source File

SOURCE=.\src\spawn\mpid_comm_connect.c
# End Source File
# Begin Source File

SOURCE=.\src\spawn\mpid_comm_disconnect.c
# End Source File
# Begin Source File

SOURCE=.\src\spawn\mpid_comm_spawn_multiple.c
# End Source File
# Begin Source File

SOURCE=.\src\init\mpid_finalize.c
# End Source File
# Begin Source File

SOURCE=.\src\rma\mpid_get.c
# End Source File
# Begin Source File

SOURCE=.\src\init\mpid_init.c
# End Source File
# Begin Source File

SOURCE=.\src\pt2pt\mpid_irecv.c
# End Source File
# Begin Source File

SOURCE=.\src\pt2pt\mpid_isend.c
# End Source File
# Begin Source File

SOURCE=.\src\spawn\mpid_open_port.c
# End Source File
# Begin Source File

SOURCE=.\src\rma\mpid_put.c
# End Source File
# Begin Source File

SOURCE=.\src\pt2pt\mpid_recv.c
# End Source File
# Begin Source File

SOURCE=.\src\pt2pt\mpid_send.c
# End Source File
# Begin Source File

SOURCE=.\src\pt2pt\mpid_startall.c
# End Source File
# Begin Source File

SOURCE=.\src\rma\mpid_win_fence.c
# End Source File
# Begin Source File

SOURCE=.\src\packer\packer_car_queue.c
# End Source File
# Begin Source File

SOURCE=.\src\packer\packer_init.c
# End Source File
# Begin Source File

SOURCE=.\src\packer\packer_make_progress.c
# End Source File
# Begin Source File

SOURCE=.\src\packer\packer_post_read.c
# End Source File
# Begin Source File

SOURCE=.\src\packer\packer_post_write.c
# End Source File
# Begin Source File

SOURCE=.\src\packer\packer_reset_car.c
# End Source File
# Begin Source File

SOURCE=.\src\progress\progress_end.c
# End Source File
# Begin Source File

SOURCE=.\src\progress\progress_poke.c
# End Source File
# Begin Source File

SOURCE=.\src\progress\progress_start.c
# End Source File
# Begin Source File

SOURCE=.\src\progress\progress_test.c
# End Source File
# Begin Source File

SOURCE=.\src\progress\progress_wait.c
# End Source File
# Begin Source File

SOURCE=.\src\request\requestutil.c
# End Source File
# Begin Source File

SOURCE=.\src\shm\shm_can_connect.c
# End Source File
# Begin Source File

SOURCE=.\src\shm\shm_get_business_card.c
# End Source File
# Begin Source File

SOURCE=.\src\shm\shm_init.c
# End Source File
# Begin Source File

SOURCE=.\src\shm\shm_make_progress.c
# End Source File
# Begin Source File

SOURCE=.\src\shm\shm_memory.c
# End Source File
# Begin Source File

SOURCE=.\src\shm\shm_post_connect.c
# End Source File
# Begin Source File

SOURCE=.\src\shm\shm_post_read.c
# End Source File
# Begin Source File

SOURCE=.\src\shm\shm_post_write.c
# End Source File
# Begin Source File

SOURCE=.\src\sock\sock_iocp.c
# End Source File
# Begin Source File

SOURCE=.\src\sock\socket_can_connect.c
# End Source File
# Begin Source File

SOURCE=.\src\sock\socket_car_queue.c
# End Source File
# Begin Source File

SOURCE=.\src\sock\socket_get_business_card.c
# End Source File
# Begin Source File

SOURCE=.\src\sock\socket_handle_written.c
# End Source File
# Begin Source File

SOURCE=.\src\sock\socket_init.c
# End Source File
# Begin Source File

SOURCE=.\src\sock\socket_make_progress.c
# End Source File
# Begin Source File

SOURCE=.\src\sock\socket_merge_unexpected_data.c
# End Source File
# Begin Source File

SOURCE=.\src\sock\socket_merge_with_posted.c
# End Source File
# Begin Source File

SOURCE=.\src\sock\socket_merge_with_unexpected.c
# End Source File
# Begin Source File

SOURCE=.\src\sock\socket_post_connect.c
# End Source File
# Begin Source File

SOURCE=.\src\sock\socket_post_read.c
# End Source File
# Begin Source File

SOURCE=.\src\sock\socket_post_write.c
# End Source File
# Begin Source File

SOURCE=.\src\sock\socket_read.c
# End Source File
# Begin Source File

SOURCE=.\src\sock\socket_reset_car.c
# End Source File
# Begin Source File

SOURCE=.\src\sock\socket_write_aggressive.c
# End Source File
# Begin Source File

SOURCE=.\src\tcp\tcp_can_connect.c
# End Source File
# Begin Source File

SOURCE=.\src\tcp\tcp_car_queue.c
# End Source File
# Begin Source File

SOURCE=.\src\tcp\tcp_get_business_card.c
# End Source File
# Begin Source File

SOURCE=.\src\tcp\tcp_init.c
# End Source File
# Begin Source File

SOURCE=.\src\tcp\tcp_make_progress.c
# End Source File
# Begin Source File

SOURCE=.\src\tcp\tcp_merge_unexpected_data.c
# End Source File
# Begin Source File

SOURCE=.\src\tcp\tcp_merge_with_posted.c
# End Source File
# Begin Source File

SOURCE=.\src\tcp\tcp_merge_with_unexpected.c
# End Source File
# Begin Source File

SOURCE=.\src\tcp\tcp_post_connect.c
# End Source File
# Begin Source File

SOURCE=.\src\tcp\tcp_post_read.c
# End Source File
# Begin Source File

SOURCE=.\src\tcp\tcp_post_write.c
# End Source File
# Begin Source File

SOURCE=.\src\tcp\tcp_read.c
# End Source File
# Begin Source File

SOURCE=.\src\tcp\tcp_reset_car.c
# End Source File
# Begin Source File

SOURCE=.\src\tcp\tcp_write.c
# End Source File
# Begin Source File

SOURCE=.\src\tcp\tcp_write_aggressive.c
# End Source File
# Begin Source File

SOURCE=..\..\util\timing\timer.c
# End Source File
# Begin Source File

SOURCE=.\src\unpacker\unpacker_car_queue.c
# End Source File
# Begin Source File

SOURCE=.\src\unpacker\unpacker_init.c
# End Source File
# Begin Source File

SOURCE=.\src\unpacker\unpacker_make_progress.c
# End Source File
# Begin Source File

SOURCE=.\src\unpacker\unpacker_post_read.c
# End Source File
# Begin Source File

SOURCE=.\src\unpacker\unpacker_post_write.c
# End Source File
# Begin Source File

SOURCE=.\src\unpacker\unpacker_reset_car.c
# End Source File
# Begin Source File

SOURCE=.\src\via\via_can_connect.c
# End Source File
# Begin Source File

SOURCE=.\src\via\via_get_business_card.c
# End Source File
# Begin Source File

SOURCE=.\src\via\via_init.c
# End Source File
# Begin Source File

SOURCE=.\src\via\via_make_progress.c
# End Source File
# Begin Source File

SOURCE=.\src\via\via_post_connect.c
# End Source File
# Begin Source File

SOURCE=.\src\via\via_post_read.c
# End Source File
# Begin Source File

SOURCE=.\src\via\via_post_write.c
# End Source File
# Begin Source File

SOURCE=.\src\viardma\via_rdma_can_connect.c
# End Source File
# Begin Source File

SOURCE=.\src\viardma\via_rdma_get_business_card.c
# End Source File
# Begin Source File

SOURCE=.\src\viardma\via_rdma_init.c
# End Source File
# Begin Source File

SOURCE=.\src\viardma\via_rdma_make_progress.c
# End Source File
# Begin Source File

SOURCE=.\src\viardma\via_rdma_post_connect.c
# End Source File
# Begin Source File

SOURCE=.\src\viardma\via_rdma_post_read.c
# End Source File
# Begin Source File

SOURCE=.\src\viardma\via_rdma_post_write.c
# End Source File
# Begin Source File

SOURCE=.\src\xfer\xfer_forward_op.c
# End Source File
# Begin Source File

SOURCE=.\src\xfer\xfer_init.c
# End Source File
# Begin Source File

SOURCE=.\src\xfer\xfer_recv_forward_op.c
# End Source File
# Begin Source File

SOURCE=.\src\xfer\xfer_recv_mop_forward_op.c
# End Source File
# Begin Source File

SOURCE=.\src\xfer\xfer_recv_mop_op.c
# End Source File
# Begin Source File

SOURCE=.\src\xfer\xfer_recv_op.c
# End Source File
# Begin Source File

SOURCE=.\src\xfer\xfer_replicate_op.c
# End Source File
# Begin Source File

SOURCE=.\src\xfer\xfer_send_op.c
# End Source File
# Begin Source File

SOURCE=.\src\xfer\xfer_start.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\include\blockallocator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\bsocket.h
# End Source File
# Begin Source File

SOURCE=.\src\bsocket\bsocketimpl.h
# End Source File
# Begin Source File

SOURCE=..\common\datatype\gen_dataloop.h
# End Source File
# Begin Source File

SOURCE=.\src\ib\ibimpl.h
# End Source File
# Begin Source File

SOURCE=.\include\ibu.h
# End Source File
# Begin Source File

SOURCE=.\include\mm_events.h
# End Source File
# Begin Source File

SOURCE=.\include\mm_ib.h
# End Source File
# Begin Source File

SOURCE=.\include\mm_ib_pre.h
# End Source File
# Begin Source File

SOURCE=.\include\mm_mpe_prof.h
# End Source File
# Begin Source File

SOURCE=.\include\mm_packer.h
# End Source File
# Begin Source File

SOURCE=.\include\mm_shm.h
# End Source File
# Begin Source File

SOURCE=.\include\mm_shm_pre.h
# End Source File
# Begin Source File

SOURCE=.\include\mm_socket.h
# End Source File
# Begin Source File

SOURCE=.\include\mm_socket_pre.h
# End Source File
# Begin Source File

SOURCE=.\include\mm_tcp.h
# End Source File
# Begin Source File

SOURCE=.\include\mm_tcp_pre.h
# End Source File
# Begin Source File

SOURCE=.\include\mm_timer_states.h
# End Source File
# Begin Source File

SOURCE=.\include\mm_unpacker.h
# End Source File
# Begin Source File

SOURCE=.\include\mm_via.h
# End Source File
# Begin Source File

SOURCE=.\include\mm_via_pre.h
# End Source File
# Begin Source File

SOURCE=.\include\mm_via_rdma.h
# End Source File
# Begin Source File

SOURCE=.\include\mm_via_rdma_pre.h
# End Source File
# Begin Source File

SOURCE=..\..\include\mpi.h
# End Source File
# Begin Source File

SOURCE=..\..\include\mpichconf.h
# End Source File
# Begin Source File

SOURCE=..\..\include\mpichtimer.h
# End Source File
# Begin Source File

SOURCE=..\..\include\mpidconf.h
# End Source File
# Begin Source File

SOURCE=.\include\mpidimpl.h
# End Source File
# Begin Source File

SOURCE=.\include\mpidpost.h
# End Source File
# Begin Source File

SOURCE=.\include\mpidpre.h
# End Source File
# Begin Source File

SOURCE=..\..\include\mpihandle.h
# End Source File
# Begin Source File

SOURCE=..\..\include\mpiimpl.h
# End Source File
# Begin Source File

SOURCE=..\..\include\mpiimplthread.h
# End Source File
# Begin Source File

SOURCE=..\..\include\pmi.h
# End Source File
# Begin Source File

SOURCE=.\src\shm\shmimpl.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sock.h
# End Source File
# Begin Source File

SOURCE=.\src\sock\socketimpl.h
# End Source File
# Begin Source File

SOURCE=.\src\tcp\tcpimpl.h
# End Source File
# Begin Source File

SOURCE=.\src\viardma\via_rdmaimpl.h
# End Source File
# Begin Source File

SOURCE=.\src\via\viaimpl.h
# End Source File
# End Group
# End Target
# End Project
