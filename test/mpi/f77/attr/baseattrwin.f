      program main
      integer v, value;
      logical flag
      integer ierr, errs
      integer base(1024)
      integer disp
      integer win
      integer (kind=MPI_ADDRESS_KIND) baseadd
      errs = 0;
      
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
      else
         call MPI_Get_address( base, baseadd, ierr )
         if (value .ne. baseadd) then
	    errs = errs + 1
	    print *, "Got incorrect value for WIN_BASE (", value, 
     &             ", should be ", baseadd
         endif
      endif

      call MPI_Win_get_attr( win, MPI_WIN_SIZE, v, flag, ierr )
      if (.not. flag) then
         errs = errs + 1
         print *, "Could not get WIN_SIZE"
      else
	if (value .ne. n) then
	    errs = err + 1
	    print *, "Got wrong value for WIN_SIZE (", value, 
     &        ", should be ", n
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

      call MPI_Finalize( );

      end
