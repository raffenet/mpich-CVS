C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
C Simple info test 
       program main
       include 'mpif.h'
       integer i1, i2
       integer errs, ierr
       character*(64) value
       logical flag
C
       errs = 0

       call MTest_Init( ierr )

       call mpi_info_create( i1, ierr )
       call mpi_info_create( i2, ierr )

       call mpi_info_set( i1, "key1", "value1", ierr )
       call mpi_info_set( i2, "key2", "value2", ierr )

       call mpi_info_get( i1, "key2", 64, value, flag, ierr )
       if (flag) then
          print *, "Found key2 in info1"
          errs = errs + 1
       endif

       call MPI_Info_get( i1, "key1", 64, value, flag, ierr )
       if (.not. flag ) then
          print *, "Did not find key1 in info1"
          errs = errs + 1
       else if (value .ne. "value1") then
          print *, "Found wrong value (", value, "), expected value1"
          errs = errs + 1
       endif

       call mpi_info_free( i1, ierr )
       call mpi_info_free( i2, ierr )

       call MTest_Finalize( errs )
       call MPI_Finalize( ierr )
       end
