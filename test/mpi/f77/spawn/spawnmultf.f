C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
C This is a special test that requires an getarg/iargc routine 
C This tests spawn_mult by using the same executable but different 
C command-line options.
C
	program main
	include 'mpif.h'
	integer errs, err
	integer rank, size, rsize, i
	integer np(2)
	integer infos(2)
	integer errcodes(2)
	integer parentcomm, intercomm
	integer status(MPI_STATUS_SIZE)
	character*(10) inargv(6,2), outargv(6,2)
	character*(30) cmds(2)
	character*(80)   argv(64)
	integer argc
        data inargv /"a", "b=c", "d e", "-pf", " Ss", " ",                  &
     &               "-p", "27", "-echo", " ", " ", " "/ 
        data outargv /"a", "b=c", "d e", "-pf", " Ss", " ",                 &
     &               "-p", "27", "-echo", " ", " ", " "/ 

	errs = 0

	call MTest_Init( ierr )

	call MPI_Comm_get_parent( parentcomm, ierr )

	if (parentcomm .eq. MPI_COMM_NULL) then
C	Create 2 more processes 
	   cmds(1) = "./spawnmultf"
	   cmds(2) = "./spawnmultf"
	   np(1)   = 1
           np(2)   = 1
	   infos(1)= MPI_INFO_NULL
           infos(2)= MPI_INFO_NULL
           call MPI_Comm_spawn_multiple( 2, cmds, inargv,                    &
     &             np, infos, 0,                                             &
     &             MPI_COMM_WORLD, intercomm, errcodes, ierr )  
	else 
	   intercomm = parentcomm
	endif

C       We now have a valid intercomm

	call MPI_Comm_remote_size( intercomm, rsize, ierr )
	call MPI_Comm_size( intercomm, size, ierr )
	call MPI_Comm_rank( intercomm, rank, ierr )

	if (parentcomm .eq. MPI_COMM_NULL) then
C	    Master 
	if (rsize .ne. np(1) + np(2)) then
	    errs = errs + 1
	    print *, "Did not create ", np(1)+np(2), " processes (got
	1	 ", rsize, ")" 
	 endif
	 do i=0, rsize-1
	    call MPI_Send( i, 1, MPI_INTEGER, i, 0, intercomm, ierr )
	 enddo
C       We could use intercomm reduce to get the errors from the 
C       children, but we'll use a simpler loop to make sure that
C       we get valid data 
	 do i=0, rsize-1
	    call MPI_Recv( err, 1, MPI_INTEGER, i, 1, intercomm,
	1	 MPI_STATUS_IGNORE, ierr )
	    errs = errs + err
	 enddo
	else 
C       Child 
C       FIXME: This assumes that stdout is handled for the children
C       (the error count will still be reported to the parent)
	   argc = iargc()
	   do i=1, argc
	      call getarg( i, argv(i) )
	   enddo
	if (size .ne. np(1) + np(2)) then
	    errs = errs + 1
	    print *, "(Child) Did not create ", np, " processes (got ",
	1	 size, ")"
	 endif

	 call MPI_Recv( i, 1, MPI_INTEGER, 0, 0, intercomm, status, ierr
	1     )
	if (i .ne. rank) then
	   errs = errs + 1
	   print *, "Unexpected rank on child ", rank, "(",i,")"
	endif
C       Check the command line 
	do i=1, argc
	   if (outargv(i,rank+1) .eq. " ") then
	      errs = errs + 1
	      print *, "Wrong number of arguments (", argc, ")"
	      goto 200
	   endif
	   if (argv(i) .ne. outargv(i,rank+1)) then
	      errs = errs + 1
	      print *, "Found arg ", argv(i), " but expected ",
	1	   outargv(i,rank+1) 
	   endif
	enddo
 200	continue
	if (outargv(i,rank+1) .ne. " ") then
C       We had too few args in the spawned command 
	    errs = errs + 1
	    print *, "Too few arguments to spawned command"
	 endif
C       Send the errs back to the master process 
	 call MPI_Ssend( errs, 1, MPI_INTEGER, 0, 1, intercomm, ierr )
	endif

C       It isn't necessary to free the intercomm, but it should not hurt
	call MPI_Comm_free( intercomm, ierr )

C       Note that the MTest_Finalize get errs only over COMM_WORLD 
	if (parentcomm .eq. MPI_COMM_NULL) then
	   call MTest_Finalize( errs )
	endif

	call MPI_Finalize( ierr )
	end
