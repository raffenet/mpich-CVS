# Microsoft Developer Studio Project File - Name="mpich2" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=MPICH2 - WIN32 RELEASE
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mpich2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mpich2.mak" CFG="MPICH2 - WIN32 RELEASE"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mpich2 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mpich2 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
F90=df.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mpich2 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE F90 /compile_only /include:"Release/" /dll /nologo /warn:nofileopt
# ADD F90 /browser /compile_only /include:"Release/" /dll /nologo /warn:nofileopt
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MPICH2_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "src\include" /I "src\mpid\mm\include" /I "src\mpid\common\datatype" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MPICH2_EXPORTS" /D FD_SETSIZE=256 /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 dlog.lib mpid_mm.lib ws2_32.lib pmi.lib mpdutil.lib mpichinfo.lib crypt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"lib/mpich2.dll" /libpath:"src\pm\winmpd\bsocket2\release" /libpath:"src\pm\winmpd\mpdutil\release" /libpath:"src\pm\winmpd\mpichinfo\release" /libpath:"src\pm\winmpd\crypt\release" /libpath:"lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=copying mpich2.dll to system32 directory
PostBuild_Cmds=copy lib\mpich2.dll %SystemRoot%\system32\mpich2.dll	copy Release\mpich2.lib lib\mpich2.lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "mpich2 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE F90 /check:bounds /compile_only /debug:full /include:"Debug/" /dll /nologo /traceback /warn:argument_checking /warn:nofileopt
# ADD F90 /browser /check:bounds /compile_only /debug:full /include:"Debug/" /dll /nologo /traceback /warn:argument_checking /warn:nofileopt
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MPICH2_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "src\include" /I "src\mpid\mm\include" /I "src\mpid\common\datatype" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MPICH2_EXPORTS" /D FD_SETSIZE=256 /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dlogd.lib mpid_mmd.lib ws2_32.lib pmid.lib mpdutil.lib mpichinfo.lib crypt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"lib/mpich2d.dll" /pdbtype:sept /libpath:"src\pm\winmpd\bsocket2\debug" /libpath:"src\pm\winmpd\mpdutil\debug" /libpath:"src\pm\winmpd\mpichinfo\debug" /libpath:"src\pm\winmpd\crypt\debug" /libpath:"lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=copying mpich2d.dll to system32 directory
PostBuild_Cmds=copy lib\mpich2d.dll %SystemRoot%\system32\mpich2d.dll	copy Debug\mpich2d.lib lib\mpich2d.lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "mpich2 - Win32 Release"
# Name "mpich2 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;f90;for;f;fpp"
# Begin Source File

SOURCE=.\src\mpi\init\abort.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\accumulate.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\add_error_class.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\add_error_code.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\add_error_string.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\address.c

!IF  "$(CFG)" == "mpich2 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mpich2 - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\allgather.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\allgatherv.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\alloc_mem.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\allreduce.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\alltoall.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\alltoallv.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\alltoallw.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\attr_delete.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\attr_get.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\attr_put.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\barrier.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\bcast.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\bsend.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\bsend_init.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\bufattach.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\buffree.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\cancel.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\topo\cart_coords.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\topo\cart_create.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\topo\cart_get.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\topo\cart_map.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\topo\cart_rank.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\topo\cart_shift.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\topo\cart_sub.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\topo\cartdim_get.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\spawn\close_port.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\spawn\comm_accept.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\comm_call_errhandler.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\comm\comm_compare.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\spawn\comm_connect.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\comm\comm_create.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\comm_create_errhandler.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\comm_create_keyval.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\comm_delete_attr.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\spawn\comm_disconnect.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\comm\comm_dup.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\comm\comm_free.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\comm_free_keyval.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\comm_get_attr.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\comm_get_errhandler.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\comm\comm_get_name.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\spawn\comm_get_parent.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\comm\comm_group.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\spawn\comm_join.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\comm\comm_rank.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\comm\comm_remote_group.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\comm\comm_remote_size.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\comm_set_attr.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\comm_set_errhandler.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\comm\comm_set_name.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\comm\comm_size.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\spawn\comm_spawn.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\spawn\comm_spawn_multiple.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\comm\comm_split.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\comm\comm_test_inter.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\commreq_free.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\comm\commutil.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\create_recv.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\create_send.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\dataloop.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\topo\dims_create.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\dynerrutil.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\errhandler_create.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\errhandler_free.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\errhandler_get.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\errhandler_set.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\error_class.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\error_string.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\errutil.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\exscan.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\file_call_errhandler.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\file_create_errhandler.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\file_get_errhandler.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\file_set_errhandler.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\io\fileutil.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\init\finalize.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\init\finalized.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\free_mem.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\gather.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\gatherv.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\get.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\get_address.c

!IF  "$(CFG)" == "mpich2 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mpich2 - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\getcount.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\getelements.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\topo\graph_get.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\topo\graph_map.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\topo\graph_nbr.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\topo\graphcreate.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\topo\graphdimsget.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\topo\graphnbrcnt.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\group\group_compare.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\group\group_difference.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\group\group_excl.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\group\group_free.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\group\group_incl.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\group\group_intersection.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\group\group_range_excl.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\group\group_range_incl.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\group\group_rank.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\group\group_size.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\group\group_translate_ranks.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\group\group_union.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\group\grouputil.c
# End Source File
# Begin Source File

SOURCE=.\src\util\mem\handlemem.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\helper_fns.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\ibsend.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\util\info\info_create.c
# End Source File
# Begin Source File

SOURCE=.\src\util\info\info_delete.c
# End Source File
# Begin Source File

SOURCE=.\src\util\info\info_dup.c
# End Source File
# Begin Source File

SOURCE=.\src\util\info\info_free.c
# End Source File
# Begin Source File

SOURCE=.\src\util\info\info_get.c
# End Source File
# Begin Source File

SOURCE=.\src\util\info\info_getn.c
# End Source File
# Begin Source File

SOURCE=.\src\util\info\info_getnth.c
# End Source File
# Begin Source File

SOURCE=.\src\util\info\info_getvallen.c
# End Source File
# Begin Source File

SOURCE=.\src\util\info\info_set.c
# End Source File
# Begin Source File

SOURCE=.\src\util\info\infoutil.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\init\init.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\init\initialized.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\init\initthread.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\comm\intercomm_create.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\comm\intercomm_merge.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\iprobe.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\irecv.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\irsend.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\isend.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\init\ismain.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\issend.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\keyval_create.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\keyval_free.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\spawn\lookup_name.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\mpich2.def
# End Source File
# Begin Source File

SOURCE=.\src\mpi\timer\mpidtime.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\mpir_test.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\mpir_wait.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\op_create.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\op_free.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\opband.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\opbor.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\opbxor.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\spawn\open_port.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\opland.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\oplor.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\oplxor.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\opmax.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\opmin.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\opprod.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\opsum.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\pack.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\pack_external.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\pack_external_size.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\pack_size.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\misc\pcontrol.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\probe.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\spawn\publish_name.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\put.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\init\querythread.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\recv.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\red_scat.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\reduce.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\register_datarep.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\request_get_status.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\rsend.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\rsend_init.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\scan.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\scatter.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\coll\scatterv.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\send.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\sendrecv.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\sendrecv_rep.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\ssend.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\ssend_init.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\start.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\startall.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\status_set_cancelled.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\status_set_elements.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\test.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\test_cancelled.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\testall.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\testany.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\testsome.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\topo\topo_test.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\util\mem\trmem.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_commit.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_contig.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_create_darray.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_create_hindexed.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_create_hvector.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_create_indexed_block.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\type_create_keyval.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_create_resized.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_create_struct.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_create_subarray.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\type_delete_attr.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_dup.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_extent.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_free.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\type_free_keyval.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\type_get_attr.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_get_contents.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_get_envelope.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_get_extent.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_get_name.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_get_true_extent.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_hind.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_hvec.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_ind.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_lb.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_match_size.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\type_set_attr.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_set_name.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_size.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_struct.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_ub.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\type_vec.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\typeutil.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\unpack.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\datatype\unpack_external.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\spawn\unpublish_name.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\wait.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\waitall.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\waitany.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\waitsome.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\win_call_errhandler.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\win_complete.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\win_create.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\win_create_errhandler.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\win_create_keyval.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\win_delete_attr.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\win_fence.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\win_free.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\win_free_keyval.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\win_get_attr.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\win_get_errhandler.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\win_get_group.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\win_get_name.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\win_lock.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\win_post.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\attr\win_set_attr.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\errhan\win_set_errhandler.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\win_set_name.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\win_start.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\win_test.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\win_unlock.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\win_wait.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\mpi\rma\winutil.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\timer\wtick.c
# End Source File
# Begin Source File

SOURCE=.\src\mpi\timer\wtime.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\src\mpi\errhan\defmsg.h
# End Source File
# Begin Source File

SOURCE=.\src\include\mpi.h
# End Source File
# Begin Source File

SOURCE=.\src\mpi\init\mpi_init.h
# End Source File
# Begin Source File

SOURCE=.\src\include\mpichconf.h
# End Source File
# Begin Source File

SOURCE=.\src\include\mpichtimer.h
# End Source File
# Begin Source File

SOURCE=.\src\mpi\comm\mpicomm.h
# End Source File
# Begin Source File

SOURCE=.\src\include\mpiimpl.h
# End Source File
# Begin Source File

SOURCE=.\src\util\info\mpiinfo.h
# End Source File
# Begin Source File

SOURCE=.\src\mpi\pt2pt\mpir_pt2pt.h
# End Source File
# Begin Source File

SOURCE=.\src\include\mpitimerimpl.h
# End Source File
# Begin Source File

SOURCE=.\src\include\rlog_macros.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
