C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      include 'mpif.h'
      integer value
      logical flag
      integer ierr, errs
      integer base(1024)
      integer disp
      integer win
      integer commsize
      errs = 0
      
      call mpi_init( ierr )
      call mpi_comm_size( MPI_COMM_WORLD, commsize, ierr )

C Create a window; then extract the values 
      n    = 1024
      disp = 4
      call MPI_Win_create( base, n, disp, MPI_INFO_NULL, MPI_COMM_WORLD, 
     &  win, ierr )
C
C In order to check the base, we need an address-of function.
C We use MPI_Get_address, even though that isn't strictly correct
      call MPI_Win_get_attr( win, MPI_WIN_BASE, value, flag, ierr )
      if (.not. flag) then
         errs = errs + 1
         print *, "Could not get WIN_BASE"
C
C There is no easy way to get the actual value of base to compare 
C against.  MPI_Address gives a value relative to MPI_BOTTOM, which 
C is different from 0 in Fortran (unless you can define MPI_BOTTOM
C as something like %pointer(0)).
C      else
C
CC For this Fortran 77 version, we use the older MPI_Address function
C         call MPI_Address( base, baseadd, ierr )
C         if (value .ne. baseadd) then
C           errs = errs + 1
C           print *, "Got incorrect value for WIN_BASE (", value, 
C     &             ", should be ", baseadd, ")"
C         endif
      endif

      call MPI_Win_get_attr( win, MPI_WIN_SIZE, value, flag, ierr )
      if (.not. flag) then
         errs = errs + 1
         print *, "Could not get WIN_SIZE"
      else
        if (value .ne. n) then
            errs = errs + 1
            print *, "Got incorrect value for WIN_SIZE (", value, 
     &        ", should be ", n, ")"
         endif
      endif

      call MPI_Win_get_attr( win, MPI_WIN_DISP_UNIT, value, flag, ierr )
      if (.not. flag) then
         errs = errs + 1
         print *, "Could not get WIN_DISP_UNIT"
      else
         if (value .ne. disp) then
            errs = errs + 1
            print *, "Got wrong value for WIN_DISP_UNIT (", value, 
     &               ", should be ", disp, ")"
         endif
      endif

      ! Check for errors
      if (errs .eq. 0) then
         print *, " No Errors"
      else
         print *, " Found ", errs, " errors"
      endif

      call MPI_Win_free( win, ierr )

      call MPI_Finalize( ierr )

      end
