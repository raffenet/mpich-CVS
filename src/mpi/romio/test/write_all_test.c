/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
*      LOS ALAMOS NATIONAL LABORATORY
*      An Affirmative Action/Equal Opportunity Employer
*
* Copyright (c) 2005
* the Regents of the University of California.
*
* Unless otherwise indicated, this information has been authored by an
* employee or employees of the University of California, operator of the Los
* Alamos National Laboratory under Contract No. W-7405-ENG-36 with the U. S.
* Department of Energy. The U. S. Government has rights to use, reproduce, and
* distribute this information. The public may copy and use this information
* without charge, provided that this Notice and any statement of authorship
* are reproduced on all copies. Neither the Government nor the University
* makes any warranty, express or implied, or assumes any liability or
* responsibility for the use of this information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpio.h>
#include <mpi.h>
#include <time.h>

/* enable all patches, set ON MPI_Abort call after debug print */

int PATCH_1 = -1;
int PATCH_2 = -1;
int PATCH_3 = -1;
int ABORT   = -1;

/**********************************************************************
* Routine to test MPI_File_write_all, read_all with zero length buffers
* Date: February 1, 2005 [jnunez]
* Date: March 18,   2005 [swh]
*********************************************************************/

static int usage(void)
{
  fprintf(stderr,"-nzp # \tNumber of processors to write zero length buffers(np-nzp, write zero-len).\n");
  fprintf(stderr,"\t The last -nzp processors will write zero length buffers.\n");
  fprintf(stderr,"-naw # \tNumber of writes for each processor in ALL WRITE loop.\n");
  fprintf(stderr,"-nzw # \tNumber of writes for each processor in ZERO length WRITE loop.\n");
  fprintf(stderr,"-size # \tNumber of bytes to write for nonzero length buffers.\n");
  fprintf(stderr,"-offm # \tOffset multiplier for debugging.\n");
  fprintf(stderr,"-zplace # \tZero write placement flag (0=start,1=end,2>off-end,3=middle,4<start).\n");
  fprintf(stderr,"-no_patch_1 \tDisable PATCH_1 in function ADIOI_Calc_aggregator\n");
  fprintf(stderr,"-no_patch_2 \tDisable PATCH_2 in function ADIOI_Heap_merge\n");
  fprintf(stderr,"-no_patch_3 \tDisable PATCH_3 in function ADIOI_Exchange_and_write\n");
  fprintf(stderr,"-no_mpi_abort \tDisable MPI_Abort call after debug prints for PATCH_1&2\n");
  fprintf(stderr,"-fname \%%s \tPath and file name to write to.\n");
  fprintf(stderr,"-hints \%%s \%%s ... \%%s \%%s  \tString pairs in the form of hint name (key) and value. Hints are expected to be in pairs and can list as many pairs as you like. File will be open with these hints.\n");
  return 1;
}

int main(int argc,char *argv[])
{
  
  MPI_Info info = MPI_INFO_NULL;      
  MPI_File fh;       
  MPI_Status status; 
#ifdef DEBUG
  MPI_Info def_info = MPI_INFO_NULL;  /* Default MPI_info struct     */
  char key[200], value[200];
  int flag, nkeys = 0;
#endif
  char *fname = NULL;
  
  int numpes, my_rank = -1;        
  int read_OK, nr_errors=0, toterrs;
  int i, j;
  int num_zero_procs    = 0;     /* Number of processes that send zero length*/
  int num_zero_writes   = 0;     /* Number of writes in zero length loop */
  int num_all_writes    = 0;     /* Number of writes in non-zero length loop */
  int obj_size          = 1024;  /* Number of bytes per non-zero object */
  int off_mult          = 8;     /* offset unit size multiplier for debugging */
  int zero_write_place  = 0;     /* zero write placement flag 
				    (0=start,1=end,2=off-end */
  double *wbuf = NULL;
  double *zbuf = NULL;
  int *length_by_rank_all;
  int *length_by_rank_zero;
  MPI_Offset    offset = 0;
  MPI_Offset    start_offset_coll = 0;
  MPI_Offset    *offset_by_rank_all;
  MPI_Offset    *offset_by_rank_zero;
  MPI_Offset    length_of_one_write_all;
  MPI_Offset    length_of_one_write_zero;
    
  struct tm *ptr;
  time_t lt;
 
 
 /******************************************************************
 * MPI Initialization
 ******************************************************************/
  if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
    fprintf(stderr, "ERROR: Problem with MPI_Init.\n");
    return -1;
  }
  if (MPI_Comm_rank(MPI_COMM_WORLD, &my_rank) != MPI_SUCCESS) {
    fprintf(stderr, "[RANK %d] ERROR: Problem getting processor rank.\n", 
	    my_rank);
    return -1;
  }
  if (MPI_Comm_size( MPI_COMM_WORLD, &numpes) != MPI_SUCCESS){
    fprintf(stderr, "[RANK %d] ERROR: Problem getting number of processors.\n",
	    my_rank);
    return -1;
  }
  
 /******************************************************************
 * Print routine identifier and date 
 ******************************************************************/
  if(my_rank == 0){
    lt = time(NULL);
    ptr = localtime(&lt);
#ifdef DEBUG
    fprintf(stdout,"write_all_test V.2005-03-22.01: %s ######\n", asctime(ptr)); 
    fflush(stdout);
#endif
  }
 
 /******************************************************************
 * Check number of command line arguments 
 ******************************************************************/
  if(argc < 2){
    usage();
    return -1;
  }

/***************************************************
* Parse the command line
**************************************************/
  i = 1;
  while ( i < argc){
    if(!strcmp(argv[i], "-nzp")){
      num_zero_procs= atoi(argv[i+1]);
      i += 2;
    }
    if(!strcmp(argv[i], "-nzw")){
      num_zero_writes = atoi(argv[i+1]);
      i += 2;
    }
    else if(!strcmp(argv[i], "-naw")){
      num_all_writes = atoi(argv[i+1]);
      i += 2;
    }
    else if(!strcmp(argv[i], "-size")){
      obj_size = atoi(argv[i+1]);
      i += 2;
    }
    else if(!strcmp(argv[i], "-offm")){
      off_mult = atoi(argv[i+1]);
      i += 2;
    }
    else if(!strcmp(argv[i], "-zplace")){
      zero_write_place = atoi(argv[i+1]);
      i += 2;
    }
    else if(!strcmp(argv[i], "-no_patch_1")){
      PATCH_1 = 0;
      i += 1;
    }
    else if(!strcmp(argv[i], "-no_patch_2")){
      PATCH_2 = 0;
      i += 1;
    }
    else if(!strcmp(argv[i], "-no_patch_3")){
      PATCH_3 = 0;
      i += 1;
    }
    else if(!strcmp(argv[i], "-no_mpi_abort")){
      ABORT = 0;
      i += 1;
    }
    else if(!strcmp(argv[i], "-fname")){
      fname = argv[i+1];
/*    fprintf(stderr,"[%03d]: File name is %s\n", my_rank, fname); */
      i += 2;
    }
    else if(!strcmp(argv[i], "-hints")){
      MPI_Info_create(&info);
      i++;
      
      while(strncmp(argv[i], "-", 1)){
	MPI_Info_set(info, argv[i], argv[i+1]);
	i += 2;
      }
    }
    else{
      if(my_rank == 0)
	fprintf(stderr, "[RANK %d] Warning: Unrecognized input %s\n", my_rank, argv[i]);
      i++;
    }
  }
  
/***************************************************
* Check input parameters and set the size to zero for the first num_zero_procs
* processors
**************************************************/
  if( num_zero_procs> numpes){
    printf("RANK %d - ERROR: Too many processors (%d) to write zero bytes.\n",
	   my_rank, num_zero_procs);
    MPI_Finalize();
    return -1;
  }

  if( (length_by_rank_all = 
	      (int *)malloc(numpes * sizeof(int))) == NULL)
  {
    printf("RANK %d - ERROR: Unable to allocate memory for length_by_rank_all\n",my_rank);
    MPI_Finalize();
    return -1;
  }

  if( (length_by_rank_zero= 
	      (int *)malloc(numpes * sizeof(int))) == NULL)
  {
    printf("RANK %d - ERROR: Unable to allocate memory for length_by_rank_all\n",my_rank);
    MPI_Finalize();
    return -1;
  }
  if( (offset_by_rank_all = (MPI_Offset *)malloc(numpes * sizeof(MPI_Offset))) == NULL){
    printf("RANK %d - ERROR: Unable to allocate memory for offset_by_rank_all\n",my_rank);
    MPI_Finalize();
    return -1;
  }
  if( (offset_by_rank_zero= (MPI_Offset *)malloc(numpes * sizeof(MPI_Offset))) == NULL){
    printf("RANK %d - ERROR: Unable to allocate memory for offset_by_rank_all\n",my_rank);
    MPI_Finalize();
    return -1;
  }

/* ------------------------------------------------------------------------*/
/* Pre-determine lengths & offsets for all procs within a single collective
 * operation */
/* ------------------------------------------------------------------------*/

   for (i=0; i<numpes; i++)
   {
     length_by_rank_all [i] =         (obj_size+  i);
     length_by_rank_zero[i] =         (obj_size+  i);
     if (i >= numpes-num_zero_procs) length_by_rank_zero[i] = 0;
   }
   offset_by_rank_all [0]  = 0;
   offset_by_rank_zero[0]  = 0;
   for (i=1; i<numpes; i++)
   {
     offset_by_rank_all [i] = offset_by_rank_all [i-1] + 
	 length_by_rank_all [i-1];
     offset_by_rank_zero[i] = offset_by_rank_zero[i-1] + 
	 length_by_rank_zero[i-1];
   }
/* Domain Length of one collective write all */
   length_of_one_write_all   = (num_all_writes  == 0) ? 0 : 
       (offset_by_rank_all [numpes-1] + length_by_rank_all [numpes-1]);
/* Domain Length of one collective write zero */
    length_of_one_write_zero = (num_zero_writes == 0) ? 0 : 
	(offset_by_rank_zero[numpes-1] + length_by_rank_zero[numpes-1]);

   for (i=0; i<numpes; i++)
   {
     if (i >= numpes-num_zero_procs) 
     {
       if (zero_write_place == 0) offset_by_rank_zero[i] = 0;
       if (zero_write_place == 1) offset_by_rank_zero[i] = 
	   length_of_one_write_zero - 1;
       if (zero_write_place == 2) offset_by_rank_zero[i] = 
	   length_of_one_write_zero;
       if (zero_write_place == 3) offset_by_rank_zero[i] = 
	   length_of_one_write_zero/2;
     }
   }
  
#ifdef DEBUG
  if(!my_rank)
  {
   for (i=0; i<numpes; i++)
   {
    fprintf(stdout,"[%03d]: length_by_rank_all [%04d]:%011ld  ######\n", 
	    my_rank, i, length_by_rank_all [i]);
    fprintf(stdout,"[%03d]: length_by_rank_zero[%04d]:%011ld  ######\n", 
	    my_rank, i, length_by_rank_zero[i]);
    fprintf(stdout,"[%03d]: offset_by_rank_all [%04d]:%011Ld  ######\n", 
	    my_rank, i, offset_by_rank_all [i]);
    fprintf(stdout,"[%03d]: offset_by_rank_zero[%04d]:%011Ld  ######\n", 
	    my_rank, i, offset_by_rank_zero[i]);
   }
    fprintf(stdout,"[%03d]:#############################################################################\n", my_rank);
    fprintf(stdout,"[%03d]: length_of_one_write_all                  %011Ld  ######\n", my_rank, length_of_one_write_all);
    fprintf(stdout,"[%03d]: length_of_one_write_zero                 %011Ld  ######\n", my_rank, length_of_one_write_zero);
    fprintf(stdout,"[%03d]: zero_write_place                         %011ld  ######\n", my_rank, zero_write_place);
    fprintf(stdout,"[%03d]: Num procs total                          %011ld  ######\n", my_rank, numpes);
    fprintf(stdout,"[%03d]: Num procs zero-len write                 %011ld  ######\n", my_rank, num_zero_procs);
    fprintf(stdout,"[%03d]: Num writes in ALL  write loop(s)         %011ld  ######\n", my_rank, num_all_writes);
    fprintf(stdout,"[%03d]: Num writes in ZERO write loop(s)         %011ld  ######\n", my_rank, num_zero_writes);
    fprintf(stdout,"[%03d]: Num elements per non-zero-len proc write %011ld  ######\n", my_rank, length_by_rank_all[my_rank]); 
    fprintf(stdout,"[%03d]: Num bytes    per non-zero-len proc write %011ld  ######\n", my_rank, length_by_rank_all[my_rank]*sizeof(double)); 
    fprintf(stdout,"[%03d]: sizeof(MPI_Offset)                       %011ldB ######\n", my_rank, sizeof(MPI_Offset)); 
    fprintf(stdout,"[%03d]: PATCH_1   (-1=enabled, 0=disabled)       %011ld  ######\n", my_rank, PATCH_1); 
    fprintf(stdout,"[%03d]: PATCH_2   (-1=enabled, 0=disabled)       %011ld  ######\n", my_rank, PATCH_2); 
    fprintf(stdout,"[%03d]: PATCH_3   (-1=enabled, 0=disabled)       %011ld  ######\n", my_rank, PATCH_3); 
    fprintf(stdout,"[%03d]: MPI_Abort (-1=enabled, 0=disabled)       %011ld  ######\n", my_rank, ABORT); 
    fprintf(stdout,"[%03d]:#############################################################################\n", my_rank);
    fflush(stdout);
  }
#endif
/***************************************************
* Open file and print hints
**************************************************/
  if( MPI_File_open(MPI_COMM_WORLD, fname, 
	      MPI_MODE_CREATE | MPI_MODE_WRONLY,info, &fh) != MPI_SUCCESS) 
  { 
    printf("RANK %d - ERROR: Unable to open file %s.\n",my_rank, fname);
    MPI_Finalize();
    return -1;
  }

#ifdef DEBUG
  if(!my_rank)
  {
    MPI_File_get_info(fh, &def_info);
    MPI_Info_get_nkeys(def_info, &nkeys);
    fprintf(stdout,"MPI-IO Hints set for: %s ######\n", fname);
    for(i=0; i < nkeys; i++)
    {
      MPI_Info_get_nthkey(def_info, i, key);
      fprintf(stdout,"key = %s ", key);
      MPI_Info_get(def_info, key, 200, value, &flag);
      fprintf(stdout,"value = %s ######\n", value);
    }
    fflush(stdout);
  }
#endif

  MPI_Barrier(MPI_COMM_WORLD);

/**************************************
 * Allocate memory for and fill the write buffer adn the zero length buffer.
 ************************************/
  if( (wbuf = (double *)malloc(length_by_rank_all[my_rank] * 
		  sizeof(double))) == NULL)
  {
    printf("RANK %d - ERROR: Unable to allocate memory for the write buffer\n",
	   my_rank);
    MPI_Finalize();
    return -1;
  }

  if( (zbuf = (double *)malloc(length_by_rank_all[my_rank] * 
		  sizeof(double))) == NULL)
  {
    printf("RANK %d - ERROR: Unable to allocate memory for the write buffer\n",
	   my_rank);
    MPI_Finalize();
    return -1;
  }
  
  for(i=0; i < length_by_rank_all[my_rank]; i++){
    wbuf[i] = (double) my_rank;
    zbuf[i] = (double) my_rank;
  }
  
    if ( MPI_Barrier(MPI_COMM_WORLD) != MPI_SUCCESS)
    {
      fprintf(stderr,"RANK %d - ERROR: MPI_Barrier at Location 0010\n",my_rank);
      MPI_Finalize();
      return -1;
    }

/**************************************
 * Write the portion of the file that all processors write and then 
 * wrtie the portion of the file that not all processors write
 * ************************************/

  start_offset_coll = 0;

  for(i=0; i < num_all_writes; i++)
  {
    offset = start_offset_coll + length_of_one_write_all * i + 
	offset_by_rank_all[my_rank];
    offset = offset*sizeof(double);

    if ( MPI_File_seek(fh, offset, MPI_SEEK_SET) != MPI_SUCCESS )
    {
      printf("RANK %d - ERROR: before write %03d Unable to seek to offset %011Ld size %011u.\n",
             my_rank, i+1, offset, length_by_rank_all[my_rank]*sizeof(double));
      MPI_Finalize();
      return -1;
    };
    
    if( MPI_File_write_all(fh, wbuf, length_by_rank_all[my_rank], 
		MPI_DOUBLE, &status) != MPI_SUCCESS)
    {
      printf("RANK %d - ERROR: Unable to write object %d with size %u.\n",
	     my_rank, i+1, length_by_rank_all[my_rank]);
      MPI_Finalize();
      return -1;
    } 

#ifdef DEBUG
    fprintf(stdout,"[%03d]: Write_1All_# = %02d of %02d offset: %011Ld size: %08ld next_offset: %011Ld ...... ######\n", 
	    my_rank, i+1, num_all_writes, offset, 
	    length_by_rank_all[my_rank]*sizeof(double),
	    offset+ length_by_rank_all[my_rank]*sizeof(double));
    fflush(stdout);
#endif
  }

  start_offset_coll = start_offset_coll + 
      length_of_one_write_all*num_all_writes;

  for(i=0; i < num_zero_writes; i++)
  {
    offset = start_offset_coll + length_of_one_write_zero * i + 
	offset_by_rank_zero[my_rank];
    offset = offset*off_mult;

    if ( MPI_File_seek(fh, offset, MPI_SEEK_SET) != MPI_SUCCESS )
    {
      printf("RANK %d - ERROR: before write %03d Unable to seek to offset %011Ld size %011u.\n",
             my_rank, i+1, offset, length_by_rank_zero[my_rank]*sizeof(double));
      MPI_Finalize();
      return -1;
    };
    
    if( MPI_File_write_all(fh, zbuf, length_by_rank_zero[my_rank], 
		MPI_DOUBLE, &status) != MPI_SUCCESS)
    {
      printf("RANK %d - ERROR: Unable to write object %d with size %d.\n",
	     my_rank, i+1, length_by_rank_zero[my_rank]);
      MPI_Finalize();
      return -1;
    }
#ifdef DEBUG
     fprintf(stdout,"[%03d]: Write_2Zero# = %02d of %02d offset: %011Ld size: %08ld next_offset: %011Ld ...... ######\n", 
	     my_rank, i+1, num_zero_writes, offset, 
	     length_by_rank_zero[my_rank]*sizeof(double),
	     offset+ length_by_rank_zero[my_rank]*sizeof(double));
     fflush(stdout);
#endif
  }

  start_offset_coll = start_offset_coll + 
      length_of_one_write_zero*num_zero_writes;

  for(i=0; i < num_all_writes; i++)
  {
    offset = start_offset_coll + length_of_one_write_all * i + 
	offset_by_rank_all[my_rank];
    offset = offset*sizeof(double);

    if ( MPI_File_seek(fh, offset, MPI_SEEK_SET) != MPI_SUCCESS )
    {
      printf("RANK %d - ERROR: before write %03d Unable to seek to offset %011Ld size %011u.\n",
	      my_rank, i+1, offset, 
	      length_by_rank_all[my_rank]*sizeof(double));
      MPI_Finalize();
      return -1;
    };
    
    if( MPI_File_write_all(fh, wbuf, length_by_rank_all[my_rank], 
		MPI_DOUBLE, &status) != MPI_SUCCESS)
    {
      printf("RANK %d - ERROR: Unable to write object %d with size %d.\n",
	     my_rank, i+1, length_by_rank_all[my_rank]);
      MPI_Finalize();
      return -1;
    } 
#ifdef DEBUG
    fprintf(stdout,"[%03d]: Write_3All_# = %02d of %02d offset: %011Ld size: %08ld next_offset: %011Ld ...... ######\n", 
	    my_rank, i+1, num_all_writes, offset, 
	    length_by_rank_all[my_rank]*sizeof(double),
	    offset+ length_by_rank_all[my_rank]*sizeof(double));
    fflush(stdout);
#endif
  }

  MPI_File_close(&fh);

/***************************************************
* Open file for read check
**************************************************/
  if( MPI_File_open(MPI_COMM_WORLD, fname,MPI_MODE_RDONLY,info, &fh) 
	  != MPI_SUCCESS) 
  {
    printf("RANK %d - ERROR: Unable to open file %s.\n",my_rank, fname);
    MPI_Finalize();
    return -1;
  }

/**************************************
 * Do read checks with read all (2 test in 1)
 * ************************************/

  start_offset_coll = 0;

  for(i=0; i < num_all_writes; i++)
  {
    for (j=0; j< length_by_rank_all[my_rank]; j++) wbuf[j] = (double) numpes;
    offset = start_offset_coll + length_of_one_write_all * i + 
	offset_by_rank_all[my_rank];
    offset = offset*sizeof(double);
    MPI_File_seek(fh, offset, MPI_SEEK_SET);
    if( MPI_File_read_all(fh, wbuf, length_by_rank_all[my_rank], 
		MPI_DOUBLE, &status) != MPI_SUCCESS)
    {
      printf("RANK %d - ERROR: Unable to read object %d with size %d.\n",
             my_rank, i+1, length_by_rank_all[my_rank]);
      MPI_Finalize();
      return -1;
    }
    read_OK = -1;
    for (j=0; j< length_by_rank_all[my_rank]; j++)
    {
	if (wbuf[j] != (double)my_rank)
	{
	    fprintf(stdout,"[%03d]: Xread_1All_# = %02d of %02d offset: %011Ld size: %08u  next_offset: %011u ERROR wbuf[%05d]=%6.3f should be %6.3f######\n", 
		    my_rank, i+1, num_all_writes, offset, 
		    length_by_rank_all[my_rank]*sizeof(double), 
		    offset+ length_by_rank_all[my_rank]*sizeof(double), 
		    j,wbuf[j], (double)my_rank);
	    fflush(stdout);
	    read_OK = 0; 
	    nr_errors++;
	}
    }
    if (read_OK)
    {
#ifdef DEBUG
      fprintf(stdout,"[%03d]: Xread_1All_# = %02d of %02d offset: %011Ld size: %08ld next_offset: %011Ld ...OK ######\n", 
	      my_rank, i+1, num_all_writes, offset, 
	      length_by_rank_all[my_rank]*sizeof(double),
	      offset+ length_by_rank_all[my_rank]*sizeof(double));
      fflush(stdout);
#endif
    }
  }

  start_offset_coll = start_offset_coll + length_of_one_write_all*num_all_writes;

  for(i=0; i < num_zero_writes; i++)
  {
    for (j=0; j< length_by_rank_all[my_rank]; j++) zbuf[j] = (double) numpes;
    offset = start_offset_coll + length_of_one_write_zero * i + 
	offset_by_rank_zero[my_rank];
    offset = offset*off_mult;

    MPI_File_seek(fh, offset, MPI_SEEK_SET);

    if( MPI_File_read_all(fh, zbuf, length_by_rank_zero[my_rank], MPI_DOUBLE, &status) != MPI_SUCCESS){
      printf("RANK %d - ERROR: Unable to read object %d with size %d.\n",
	     my_rank, i+1, length_by_rank_zero[my_rank]);
      MPI_Finalize();
      return -1;
    }
    read_OK = -1;
    if (!length_by_rank_zero[my_rank])
    {
       if (zbuf[0] != (double)numpes)
       {
	   fprintf(stdout,"[%03d]: Xread_2Zero# = %02d of %02d offset: %011Ld size: %6lu next_offset: %011u ERROR zbuf[00000]=%6.3f should be %d ######\n", 
		   my_rank, i+1, num_all_writes, offset, 
		   length_by_rank_zero[my_rank]*sizeof(double),
		   offset+ length_by_rank_zero[my_rank]*sizeof(double), 
		   zbuf[j],numpes);
        fflush(stdout);
        read_OK = 0;
	nr_errors++;
       }
    }
    else
    {
      for (j=0; j< length_by_rank_zero[my_rank]; j++)
      {
       if (zbuf[j] != (double)my_rank)
       {
          fprintf(stdout,"[%03d]: Xread_2Zero# = %02d of %02d offset: %011Ld size: %08ld next_offset: %011u ERROR zbuf[%05d]=%6.3f should be %d ######\n", 
                my_rank, i+1, num_zero_writes, offset, 
		length_by_rank_zero[my_rank]*sizeof(double),
		offset+ length_by_rank_zero[my_rank]*sizeof(double),
		j,zbuf[j], my_rank);
	  fflush(stdout);
	  read_OK = 0;
	  nr_errors++;
       }
      }
    }
    if (read_OK)
    {
#ifdef DEBUG
      fprintf(stdout,"[%03d]: Xread_2Zero# = %02d of %02d offset: %011Ld size: %08ld next_offset: %011Ld ...OK ######\n", 
	      my_rank, i+1, num_zero_writes, offset, 
	      length_by_rank_zero[my_rank]*sizeof(double),
	      offset+ length_by_rank_zero[my_rank]*sizeof(double),j,zbuf[j]);
      fflush(stdout);
#endif
    }
  }

  start_offset_coll = start_offset_coll + 
      length_of_one_write_zero*num_zero_writes;

  for(i=0; i < num_all_writes; i++)
  {
    for (j=0; j< length_by_rank_all[my_rank]; j++) wbuf[j] = (double) numpes;
    offset = start_offset_coll + length_of_one_write_all * i + 
	offset_by_rank_all[my_rank];
    offset = offset*sizeof(double);
    MPI_File_seek(fh, offset, MPI_SEEK_SET);
    if( MPI_File_read_all(fh, wbuf, length_by_rank_all[my_rank], 
		MPI_DOUBLE, &status) != MPI_SUCCESS)
    {
      printf("RANK %d - ERROR: Unable to read object %d with size %d.\n",
             my_rank, i+1, length_by_rank_all[my_rank]);
      MPI_Finalize();
      return -1;
    }
    read_OK = -1;
    for (j=0; j< length_by_rank_all[my_rank]; j++)
    {
     if (wbuf[j] != (double)my_rank)
     {
       fprintf(stdout,"[%03d]: Xread_3All_# = %02d of %02d offset: %011Ld size: %08ld  next_offset: %011u ERROR wbuf[%05d]=%6.3f should be %d######\n", 
              my_rank, i+1, num_all_writes, offset, 
	      length_by_rank_all[my_rank]*sizeof(double),
	      offset+ length_by_rank_all[my_rank]*sizeof(double), 
	      j,wbuf[j], my_rank);
       fflush(stdout);
       read_OK = 0;
       nr_errors++;
     }
    }
    if (read_OK)
    {
#ifdef DEBUG
      fprintf(stdout,"[%03d]: Xread_3All_# = %02d of %02d offset: %011Ld size: %08ld next_offset: %011Ld ...OK ######\n", 
              my_rank, i+1, num_all_writes, offset, 
	      length_by_rank_all[my_rank]*sizeof(double),
	      offset+ length_by_rank_all[my_rank]*sizeof(double));
      fflush(stdout);
#endif
    }
  }


  MPI_File_close(&fh);

    /**************************************
     * Get the (persistent) hints
     ************************************/
  if( MPI_File_open(MPI_COMM_WORLD, fname,
                    MPI_MODE_RDONLY, MPI_INFO_NULL, &fh) != MPI_SUCCESS) {
    printf("RANK %d: ERROR: Unable to open file %s.\n",my_rank, fname);
    MPI_Finalize();
    return -1;
  }

#ifdef DEBUG
  if(my_rank == 0)
    printf("MPI-IO (persistent) Hints for: %s\n", fname);

  MPI_File_get_info(fh, &def_info);
  MPI_Info_get_nkeys(def_info, &nkeys);
  for(i=0; i < nkeys; i++){
    MPI_Info_get_nthkey(def_info, i, key);
    if(my_rank == 0){
      printf("key = %s ", key);
      MPI_Info_get(def_info, key, 200, value, &flag);
      printf("value = %s\n", value);
    }
  }
#endif

  MPI_File_close(&fh);

  MPI_Allreduce(&nr_errors, &toterrs, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
  if (my_rank == 0) {
      if (toterrs > 0) {
	  fprintf(stderr, "Found %d errors\n", toterrs);
      }
      else {
	  fprintf(stdout, " No Errors\n");
      }
  }

  MPI_Finalize();
  return(0);

}

/* 
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
