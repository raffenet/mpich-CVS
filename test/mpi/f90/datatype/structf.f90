C Thanks to 
C William R. Magro
C for this test
C
C It has been modifiedly slightly to work with the automated MPI
C tests.
C  WDG.
C
C It was further modified to use MPI_Get_address instead of MPI_Address
C for MPICH2, and to fit in the MPICH2 test harness - WDG
C
      program bustit
      implicit none

      use mpi

      integer ierr
      integer comm
      integer newtype
      integer me
      integer position
      integer type(5)
      integer length(5)
      integer (kind=MPI_ADDRESS_KIND) disp(5)
      integer bufsize
      integer errs, toterrs
      parameter (bufsize=100)
      character buf(bufsize)
      character name*(10)
      integer status(MPI_STATUS_SIZE)
      integer i, size
      double precision x
      integer src, dest

      errs = 0
!     Enroll in MPI
      call mpi_init(ierr)

!     get my rank
      call mpi_comm_rank(MPI_COMM_WORLD, me, ierr)
      call mpi_comm_size(MPI_COMM_WORLD, size, ierr )
      if (size .lt. 2) then
         print *, "Must have at least 2 processes"
         call MPI_Abort( 1, MPI_COMM_WORLD, ierr )
      endif

      comm = MPI_COMM_WORLD
      src = 0
      dest = 1

      if(me.eq.src) then
          i=5
          x=5.1234d0
          name="hello"

          type(1)=MPI_CHARACTER
          length(1)=5
          call mpi_get_address(name,disp(1),ierr)

          type(2)=MPI_DOUBLE_PRECISION
          length(2)=1
          call mpi_get_address(x,disp(2),ierr)

          call mpi_type_create_struct(2,length,disp,type,newtype,ierr)
          call mpi_type_commit(newtype,ierr)
          call mpi_barrier( MPI_COMM_WORLD, ierr )
          call mpi_send(MPI_BOTTOM,1,newtype,dest,1,comm,ierr)
          call mpi_type_free(newtype,ierr)
!         write(*,*) "Sent ",name(1:5),x
      else 
!         Everyone calls barrier incase size > 2
          call mpi_barrier( MPI_COMM_WORLD, ierr )
          if (me.eq.dest) then
             position=0

             name = " "
             x    = 0.0d0
             call mpi_recv(buf,bufsize,MPI_PACKED, src,
     .            1, comm, status, ierr)
             
             call mpi_unpack(buf,bufsize,position,
     .            name,5,MPI_CHARACTER, comm,ierr)
             call mpi_unpack(buf,bufsize,position,
     .            x,1,MPI_DOUBLE_PRECISION, comm,ierr)
!            Check the return values
             if (name .nes. "Hello") then
                errs = errs + 1
                print *, "Received ", name, " but expected Hello"
             endif
             if (abs(x-5.1234) .gt. 1.0e-6) then
                errs = errs + 1
                print *, "Received ", x, " but expected 5.1234"
             endif
             print 1, name, x
 1           format( " Received ", a, f7.4 )
          endif
      endif
!
!     Sum up errs and report the result
      call mpi_reduce( errs, toterrs, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr )
      if (me .eq. 0) then
         if (toterrs .eq. 0) then
            print *, " No Errors"
         else
            print *, " Found ", toterrs, " errors"
         endif
      endif

      call mpi_finalize(ierr)

      end
