C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
       program main
       implicit none
       include 'mpif.h'
       integer esize
       integer ierr, errs
       include 'asize.h'

       errs = 0
       call mtest_init( ierr )

       call mpi_pack_external_size( 'external32', 10, MPI_INTEGER, 
     &                              esize, ierr ) 
       if (esize .ne. 10 * 4) then
          errs = errs + 1
          print *, 'Expected 40 for size of 10 integers'
       endif
C
       call mpi_pack_external( 'external32', inbuf, insize, MPI_INTEGER,
     &               packbuf, pbufsize, asize1, asize2, ierr )
       call mpi_pack_external( 'external32', rbuf, rsize, 
     &               MPI_DOUBLE_PRECISION, packbuf, pbufsize, asize1, 
     &               asize2, ierr )
       call mpi_pack_external( 'external32', cbuf, csize, 
     &               MPI_CHARACTER, packbuf, pbufsize, asize1, 
     &               asize2, ierr )
       call mpi_pack_external( 'external32', inbuf2, insize2, MPI_INTEGER,
     &               packbuf, pbufsize, asize1, asize2, ierr )
C
C We could try sending this with MPI_BYTE...
       asize2 = 0
       call mpi_unpack_external( 'external32', packbuf, asize1,
     &  asize2, ioutbuf, insize, MPI_INTEGER, ierr )
       call mpi_unpack_external( 'external32', packbuf, asize1,
     &  asize2, routbuf, rsize, MPI_DOUBLE_PRECISION, ierr )
       call mpi_unpack_external( 'external32', packbuf, asize1,
     &  asize2, coutbuf, csize, MPI_CHARACTER, ierr )
       call mpi_unpack_external( 'external32', packbuf, asize1,
     &  asize2, ioutbuf2, insize2, MPI_INTEGER, ierr )
C
C Now, test the values
C
todo ......       
       call mtest_finalize( errs )
       end
