        subroutine MTest_Init( ierr )
C       Place the include first so that we can automatically create a
C       Fortran 90 version that uses the mpi module instead.  If
C       the module is in a different place, the compiler can complain
C       about out-of-order statements
        include 'mpif.h'
        integer ierr
        logical flag
        logical dbgflag
        integer wrank
        common /mtest/ dbgflag, wrank

        call MPI_Initialized( flag, ierr );
        if (.not. flag) then
           call MPI_Init( ierr )
        endif

        dbgflag = .false.
        call MPI_Comm_rank( MPI_COMM_WORLD, wrank, ierr )
        end

        subroutine MTest_Finalize( errs )
        include 'mpif.h'
        integer errs
        integer rank, toterrs, ierr
        
        call MPI_Comm_rank( MPI_COMM_WORLD, rank, ierr )

        call MPI_Allreduce( errs, toterrs, 1, MPI_INTEGER, MPI_SUM, 
     *        MPI_COMM_WORLD, ierrs ) 
        
        if (rank .eq. 0) then
           if (toterrs .gt. 0) then 
                print *, " Found ", toterrs, " errors"
           else
                print *, " No Errors"
           endif
        endif
        end

        subroutine MTestPrintError( errcode )
        include 'mpif.h'
        integer errcode
        integer errclass, slen, ierr
        character*(MPI_MAX_ERROR_STRING) string

        call MPI_Error_class( errcode, errclass, ierr )
        call MPI_Error_string( errcode, string, slen, ierr )
        print *, "Error class ", errclass, "(", string(1:slen), ")"
        end
