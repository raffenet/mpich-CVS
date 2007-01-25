C -*- Mode: Fortran; -*- 
C
C  (C) 2004 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
        program main
        include 'mpif.h'
C
C This program makes use of a common (but not universal; g77 doesn't 
C have it) extension: the "Cray" pointer.  This allows MPI_Alloc_mem
C to allocate memory and return it to Fortran, where it can be used.
C As this is not standard Fortran, this test is not run by default.
C To run it, build (with a suitable compiler) and run with
C   mpiexec -n 1 ./allocmemf
C
        real a
        pointer (p,a(100,100))
        include '../rma/addsize.h'
        integer ierr, sizeofreal, errs
        integer i,j
C
        errs = 0
        call mtest_init(ierr)
        call mpi_type_size( MPI_REAL, sizeofreal, ierr )
C Make sure we pass in an integer of the correct type
        aint = sizeofreal * 100 * 100
        call mpi_alloc_mem( aint,MPI_INFO_NULL,p,ierr )

        do i=1,100
            do j=1,100
                a(i,j) = -1
            enddo
        enddo
        a(3,5) = 10.0

        call mpi_free_mem( a, ierr )
        call mtest_finalize(errs)
        call mpi_finalize(ierr)

        end
