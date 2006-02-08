#include "adio.h"
#include "adio_extern.h"
#include "ad_zoidfs.h"


void ADIOI_ZOIDFS_ReadContig(ADIO_File fd, void *buf, int count, 
			     MPI_Datatype datatype, int file_ptr_type,
			     ADIO_Offset offset, ADIO_Status *status,
			     int *error_code)
{
    int ret, datatype_size, len;
    ADIOI_ZOIDFS_fs *zoidfs_fs;
    static char myname[] = "ADIOI_ZOIDFS_READCONTIG";
    void *mem_starts[1];
    size_t mem_sizes[1];
    uint64_t file_starts[1], file_sizes[1];
    
    zoidfs_fs = (ADIOI_ZOIDFS_fs*)fd->fs_ptr;

    MPI_Type_size(datatype, &datatype_size);
    len = datatype_size * count;

    mem_starts[0] = buf;
    mem_sizes[0] = len;
    file_sizes[0] = len;
    
    if (file_ptr_type == ADIO_EXPLICIT_OFFSET) 
	file_starts[0] = offset;
    else 
	file_starts[0] = fd->fp_ind;
    
    ret = zoidfs_write(&(zoidfs_fs->fhandle), 
		       1,
		       mem_starts,
		       mem_sizes,
		       1,
		       file_starts,
		       file_sizes);
    
    /* --BEGIN ERROR HANDLING-- */
    if (ret != ZFS_OK) {
      *error_code = MPIO_Err_create_code(MPI_SUCCESS,
					 MPIR_ERR_RECOVERABLE,
					 myname, __LINE__,
					 MPI_ERR_IO,
					 "Error in zoidfs_read (zoidfs errno %d)", ret);
      return;
    }
    /* --END ERROR HANDLING-- */
    
    if (file_ptr_type == ADIO_EXPLICIT_OFFSET) 
      fd->fp_sys_posn = offset + file_sizes[0];
    else {
      fd->fp_ind += file_sizes[0];
      fd->fp_sys_posn = fd->fp_ind;
    } 
    
#ifdef HAVE_STATUS_SET_BYTES
    if (ret == ZFS_OK) MPIR_Status_set_bytes(status, datatype, file_sizes[0]);
#endif
    
    *error_code = MPI_SUCCESS;
}


void ADIOI_ZOIDFS_ReadStrided(ADIO_File fd, void *buf, int count,
			      MPI_Datatype datatype, int file_ptr_type,
			      ADIO_Offset offset, ADIO_Status *status, int
			      *error_code)
{
  /* offset is in units of etype relative to the filetype. */
  ADIOI_Flatlist_node *flat_buf, *flat_file;
  int i, j, k,  brd_size, frd_size=0, st_index=0;
  int bufsize, sum, n_etypes_in_filetype, size_in_filetype;
  int n_filetypes, etype_in_filetype;
  ADIO_Offset abs_off_in_filetype=0;
  int filetype_size, etype_size, buftype_size;
  MPI_Aint filetype_extent, buftype_extent; 
  int buf_count, buftype_is_contig, filetype_is_contig;
  ADIO_Offset off, disp, start_off;
  int flag, st_frd_size, st_n_filetypes;
  
  int mem_list_count, file_list_count;
  void **mem_starts;
  size_t *mem_sizes;
  uint64_t *file_starts, *file_sizes;
  int total_blks_to_read;
  
  int max_mem_list, max_file_list;
  
  int b_blks_read;
  int f_data_read;
  int size_read=0, n_read_lists, extra_blks;
  
  int end_brd_size, end_frd_size;
  int start_k, start_j, new_file_read, new_buffer_read;
  int start_mem_offset;
  
  ADIOI_ZOIDFS_fs * zoidfs_fs;
  
    int ret_flag, ret;
    MPI_Offset total_bytes_read = 0;
    static char myname[] = "ADIOI_ZOIDFS_ReadStrided";
    
#define MAX_ARRAY_SIZE 64

    *error_code = MPI_SUCCESS;  /* changed below if error */

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(fd->filetype, &filetype_is_contig);
    MPI_Type_size(fd->filetype, &filetype_size);
    if (!filetype_size) {
	*error_code = MPI_SUCCESS; 
	return;
    }
    
    MPI_Type_extent(fd->filetype, &filetype_extent);
    MPI_Type_size(datatype, &buftype_size);
    MPI_Type_extent(datatype, &buftype_extent);
    etype_size = fd->etype_size;
    
    bufsize = buftype_size * count;
    
    zoidfs_fs = (ADIOI_ZOIDFS_fs*)fd->fs_ptr;
    
    if (!buftype_is_contig && filetype_is_contig) {
	
/* noncontiguous in memory, contiguous in file. */

	ADIOI_Flatten_datatype(datatype);
	flat_buf = ADIOI_Flatlist;
	while (flat_buf->type != datatype) flat_buf = flat_buf->next;

	off = (file_ptr_type == ADIO_INDIVIDUAL) ? fd->fp_ind : 
	    fd->disp + etype_size * offset;

	file_list_count = 1;
	total_blks_to_read = count * flat_buf->count;
	/* allocate arrays according to max usage */
	if (total_blks_to_read > MAX_ARRAY_SIZE)
	    mem_list_count = MAX_ARRAY_SIZE;
	else mem_list_count = total_blks_to_read;

	mem_starts = (void **)ADIOI_Malloc(mem_list_count * sizeof(void *));
	mem_sizes = (size_t *)ADIOI_Malloc(mem_list_count * sizeof(size_t));
	file_starts = (uint64_t *)ADIOI_Malloc(file_list_count * sizeof(uint64_t));
	file_sizes = (uint64_t *)ADIOI_Malloc(file_list_count * sizeof(uint64_t));
	/* TODO: CHECK RESULTS OF MEMORY ALLOCATION */

	file_starts[0] = off;
	file_sizes[0] = 0;
	b_blks_read = 0;	
	
	j = 0;
	/* step through each block in memory, filling memory arrays */
	while (b_blks_read < total_blks_to_read) {
	    for (i=0; i<flat_buf->count; i++) {
		mem_starts[b_blks_read % MAX_ARRAY_SIZE] = 
		    (buf + j*buftype_extent + flat_buf->indices[i]);
		mem_sizes[b_blks_read % MAX_ARRAY_SIZE] = 
		    flat_buf->blocklens[i];
		file_sizes[0] += flat_buf->blocklens[i];
		b_blks_read++;
		if (!(b_blks_read % MAX_ARRAY_SIZE) ||
		    (b_blks_read == total_blks_to_read)) {

		    /* in the case of the last read list call,
		       adjust mem_list_count */
		    if (b_blks_read == total_blks_to_read) {
		        mem_list_count = total_blks_to_read % MAX_ARRAY_SIZE;
			/* in case last read list call fills max arrays */
			if (!mem_list_count) mem_list_count = MAX_ARRAY_SIZE;
		    }
		    
		    ret = zoidfs_read(&(zoidfs_fs->fhandle),
				      mem_list_count,
				      mem_starts,
				      mem_sizes,
				      file_list_count,
				      file_starts,
				      file_sizes);

		    /* --BEGIN ERROR HANDLING-- */
		    if (ret != ZFS_OK) {
			*error_code = MPIO_Err_create_code(MPI_SUCCESS,
							   MPIR_ERR_RECOVERABLE,
							   myname, __LINE__,
							   MPI_ERR_IO,
							   "Error in zoidfs_read (zoidfs errno %d)", ret);
			goto error_state;
		    }
		    total_bytes_read += file_sizes[0];
		    /* --END ERROR HANDLING-- */
		    
		    /* in the case of error or the last read list call, 
		     * leave here */
		    if (ret || b_blks_read == total_blks_to_read) break;
		    
		    file_starts[0] += file_sizes[0];
		    file_sizes[0] = 0;
		} 
	    } /* for (i=0; i<flat_buf->count; i++) */
	    j++;
	} /* while (b_blks_read < total_blks_to_read) */
	ADIOI_Free(mem_starts);
	ADIOI_Free(mem_sizes);
	ADIOI_Free(file_starts);
	ADIOI_Free(file_sizes);
	
        if (file_ptr_type == ADIO_INDIVIDUAL) 
	  fd->fp_ind += total_bytes_read;
	
	fd->fp_sys_posn = -1;  /* set it to null. */

#ifdef HAVE_STATUS_SET_BYTES
	MPIR_Status_set_bytes(status, datatype, total_bytes_read);
#endif
	ADIOI_Delete_flattened(datatype);

	return;
    } /* if (!buftype_is_contig && filetype_is_contig) */

    /* know file is noncontiguous from above */
    /* noncontiguous in file */

    /* filetype already flattened in ADIO_Open */
    flat_file = ADIOI_Flatlist;
    while (flat_file->type != fd->filetype) flat_file = flat_file->next;

    disp = fd->disp;

    /* for each case - ADIO_Individual pointer or explicit, find the file
       offset in bytes (offset), n_filetypes (how many filetypes into
       file to start), frd_size (remaining amount of data in present
       file block), and st_index (start point in terms of blocks in
       starting filetype) */
    if (file_ptr_type == ADIO_INDIVIDUAL) {
        offset = fd->fp_ind; /* in bytes */
	n_filetypes = -1;
	flag = 0;
	while (!flag) {
	    n_filetypes++;
	    for (i=0; i<flat_file->count; i++) {
	        if (disp + flat_file->indices[i] + 
		    (ADIO_Offset) n_filetypes*filetype_extent +
		    flat_file->blocklens[i]  >= offset) {
		    st_index = i;
		    frd_size = (int) (disp + flat_file->indices[i] + 
				      (ADIO_Offset) n_filetypes*filetype_extent
				      + flat_file->blocklens[i] - offset);
		    flag = 1;
		    break;
		}
	    }
	} /* while (!flag) */
    } /* if (file_ptr_type == ADIO_INDIVIDUAL) */
    else {
        n_etypes_in_filetype = filetype_size/etype_size;
	n_filetypes = (int) (offset / n_etypes_in_filetype);
	etype_in_filetype = (int) (offset % n_etypes_in_filetype);
	size_in_filetype = etype_in_filetype * etype_size;
	
	sum = 0;
	for (i=0; i<flat_file->count; i++) {
	    sum += flat_file->blocklens[i];
	    if (sum > size_in_filetype) {
	        st_index = i;
		frd_size = sum - size_in_filetype;
		abs_off_in_filetype = flat_file->indices[i] +
		    size_in_filetype - (sum - flat_file->blocklens[i]);
		break;
	    }
	}
	
	/* abs. offset in bytes in the file */
	offset = disp + (ADIO_Offset) n_filetypes*filetype_extent + 
	    abs_off_in_filetype;
    } /* else [file_ptr_type != ADIO_INDIVIDUAL] */

    start_off = offset;
    st_frd_size = frd_size;
    st_n_filetypes = n_filetypes;
    
    if (buftype_is_contig && !filetype_is_contig) {

/* contiguous in memory, noncontiguous in file. should be the most
   common case. */

	i = 0;
	j = st_index;
	n_filetypes = st_n_filetypes;
	
	mem_list_count = 1;
	
	/* determine how many blocks in file to read */
	f_data_read = ADIOI_MIN(st_frd_size, bufsize);
	total_blks_to_read = 1;
	j++;
	while (f_data_read < bufsize) {
	    f_data_read += flat_file->blocklens[j];
	    total_blks_to_read++;
	    if (j<(flat_file->count-1)) j++;
	    else j = 0;	
	}
      
	j = st_index;
	n_filetypes = st_n_filetypes;
	n_read_lists = total_blks_to_read/MAX_ARRAY_SIZE;
	extra_blks = total_blks_to_read%MAX_ARRAY_SIZE;
	
	mem_starts = (void **)ADIOI_Malloc(mem_list_count * sizeof(void *));
	mem_sizes = (size_t *)ADIOI_Malloc(mem_list_count * sizeof(size_t));
	
	/* if at least one full readlist, allocate file arrays
	   at max array size and don't free until very end */
	if (n_read_lists) {
	  file_starts = (uint64_t *)ADIOI_Malloc(MAX_ARRAY_SIZE*sizeof(uint64_t));
	  file_sizes = (uint64_t *)ADIOI_Malloc(MAX_ARRAY_SIZE*sizeof(uint64_t));
	}
	/* if there's no full readlist allocate file arrays according
	   to needed size (extra_blks) */
	else {
	  file_starts = (uint64_t *)ADIOI_Malloc(extra_blks*sizeof(uint64_t));
	  file_sizes = (uint64_t *)ADIOI_Malloc(extra_blks*sizeof(uint64_t));
	}
       
	/* TODO: CHECK RESULTS OF MEMORY ALLOCATION */
	
	mem_starts[0] = buf;
	mem_sizes[0] = 0;
	
	/* for file arrays that are of MAX_ARRAY_SIZE, build arrays */
	for (i=0; i<n_read_lists; i++) {
	  file_list_count = MAX_ARRAY_SIZE;
	  if (!i) {
	    file_starts[0] = offset;
	    file_sizes[0] = st_frd_size;
		mem_sizes[0] = st_frd_size;
	    }
	    for (k=0; k<MAX_ARRAY_SIZE; k++) {
	        if (i || k) {
		    file_starts[k] = disp + n_filetypes*filetype_extent
		      + flat_file->indices[j];
		    file_sizes[k] = flat_file->blocklens[j];
		    mem_sizes[0] += file_sizes[k];
		}
		if (j<(flat_file->count - 1)) j++;
		else {
		    j = 0;
		    n_filetypes++;
		}
	    } /* for (k=0; k<MAX_ARRAY_SIZE; k++) */

	    ret = zoidfs_read(&(zoidfs_fs->handle), 
			    mem_list_count,
			    mem_starts,
			    mem_sizes,
			    file_list_count,
			    file_starts,
			    file_sizes);
	    /* --BEGIN ERROR HANDLING-- */
	    if (ret != ZFS_OK) {
		*error_code = MPIO_Err_create_code(MPI_SUCCESS,
						   MPIR_ERR_RECOVERABLE,
						   myname, __LINE__,
						   MPI_ERR_IO
						   "Error in zoidfs_read (zoidfs errno %d)", ret);
		goto error_state;
	    }
	    /* --END ERROR HANDING-- */
	    total_bytes_read += file_sizes[0];

	    mem_starts[0] += mem_sizes[0];
	    mem_sizes[0] = 0;
	} /* for (i=0; i<n_read_lists; i++) */

	/* for file arrays smaller than MAX_ARRAY_SIZE (last read_list call) */
	if (extra_blks) {
	    file_list_count = extra_blks;
	    if(!i) {
	        file_starts[0] = offset;
		file_sizes[0] = st_frd_size;
	    }
	    for (k=0; k<extra_blks; k++) {
	        if(i || k) {
		    file_starts[k] = disp + n_filetypes*filetype_extent +
		      flat_file->indices[j];
		    if (k == (extra_blks - 1)) {
		        file_sizes[k] = bufsize - (uint64_t) mem_sizes[0]
			  - (uint64_t) mem_starts[0] + (uint64_t)  buf;
		    }
		    else file_sizes[k] = flat_file->blocklens[j];
		} /* if(i || k) */
		mem_sizes[0] += file_sizes[k];
		if (j<(flat_file->count - 1)) j++;
		else {
		    j = 0;
		    n_filetypes++;
		}
	    } /* for (k=0; k<extra_blks; k++) */

	    ret = zoidfs_read(&(zoidfs_fs->fhandle),
			    mem_list_count,
			    mem_starts,
			    mem_sizes,
			    file_list_count,
			    file_starts,
			    file_sizes);
	    /* --BEGIN ERROR HANDLING-- */
	    if (ret != ZFS_OK) {
		*error_code = MPIO_Err_create_code(MPI_SUCCESS,
						   MPIR_ERR_RECOVERABLE,
						   myname, __LINE__,
						   MPI_ERR_IO,
						   "Error in zoidfs_read (zoidfs errno %d)", ret);		
		goto error_state;
	    }
	    /* --END ERROR HANDLING-- */
	    total_bytes_read += file_sizes[0];
	}
    }
    else {
/* noncontiguous in memory as well as in file */
      
        ADIOI_Flatten_datatype(datatype);
	flat_buf = ADIOI_Flatlist;
	while (flat_buf->type != datatype) flat_buf = flat_buf->next;

	size_read = 0;
	n_filetypes = st_n_filetypes;
	frd_size = st_frd_size;
	brd_size = flat_buf->blocklens[0];
	buf_count = 0;
	start_mem_offset = 0;
	start_k = k = 0;
	start_j = st_index;
	max_mem_list = 0;
	max_file_list = 0;

	/* run through and file max_file_list and max_mem_list so that you 
	   can allocate the file and memory arrays less than MAX_ARRAY_SIZE
	   if possible */

	while (size_read < bufsize) {
	    k = start_k;
	    new_buffer_read = 0;
	    mem_list_count = 0;
	    while ((mem_list_count < MAX_ARRAY_SIZE) && 
		   (new_buffer_read < bufsize-size_read)) {
	        /* find mem_list_count and file_list_count such that both are
		   less than MAX_ARRAY_SIZE, the sum of their lengths are
		   equal, and the sum of all the data read and data to be
		   read in the next immediate read list is less than
		   bufsize */
	        if(mem_list_count) {
		    if((new_buffer_read + flat_buf->blocklens[k] + 
			size_read) > bufsize) {
		        end_brd_size = new_buffer_read + 
			    flat_buf->blocklens[k] - (bufsize - size_read);
			new_buffer_read = bufsize - size_read;
		    }
		    else {
		        new_buffer_read += flat_buf->blocklens[k];
			end_brd_size = flat_buf->blocklens[k];
		    }
		}
		else {
		    if (brd_size > (bufsize - size_read)) {
		        new_buffer_read = bufsize - size_read;
			brd_size = new_buffer_read;
		    }
		    else new_buffer_read = brd_size;
		}
		mem_list_count++;
		k = (k + 1)%flat_buf->count;
	     } /* while ((mem_list_count < MAX_ARRAY_SIZE) && 
	       (new_buffer_read < bufsize-size_read)) */
	    j = start_j;
	    new_file_read = 0;
	    file_list_count = 0;
	    while ((file_list_count < MAX_ARRAY_SIZE) && 
		   (new_file_read < new_buffer_read)) {
	        if(file_list_count) {
		    if((new_file_read + flat_file->blocklens[j]) > 
		       new_buffer_read) {
		        end_frd_size = new_buffer_read - new_file_read;
			new_file_read = new_buffer_read;
			j--;
		    }
		    else {
		        new_file_read += flat_file->blocklens[j];
			end_frd_size = flat_file->blocklens[j];
		    }
		}
		else {
		    if (frd_size > new_buffer_read) {
		        new_file_read = new_buffer_read;
			frd_size = new_file_read;
		    }
		    else new_file_read = frd_size;
		}
		file_list_count++;
		if (j < (flat_file->count - 1)) j++;
		else j = 0;
		
		k = start_k;
		if ((new_file_read < new_buffer_read) && 
		    (file_list_count == MAX_ARRAY_SIZE)) {
		    new_buffer_read = 0;
		    mem_list_count = 0;
		    while (new_buffer_read < new_file_read) {
		        if(mem_list_count) {
			    if((new_buffer_read + flat_buf->blocklens[k]) >
			       new_file_read) {
			        end_brd_size = new_file_read - new_buffer_read;
				new_buffer_read = new_file_read;
				k--;
			    }
			    else {
			        new_buffer_read += flat_buf->blocklens[k];
				end_brd_size = flat_buf->blocklens[k];
			    }
			}
			else {
			    new_buffer_read = brd_size;
			    if (brd_size > (bufsize - size_read)) {
			        new_buffer_read = bufsize - size_read;
				brd_size = new_buffer_read;
			    }
			}
			mem_list_count++;
			k = (k + 1)%flat_buf->count;
		    } /* while (new_buffer_read < new_file_read) */
		} /* if ((new_file_read < new_buffer_read) && (file_list_count
		     == MAX_ARRAY_SIZE)) */
	    } /* while ((mem_list_count < MAX_ARRAY_SIZE) && 
		 (new_buffer_read < bufsize-size_read)) */

	    /*  fakes filling the readlist arrays of lengths found above  */
	    k = start_k;
	    j = start_j;
	    for (i=0; i<mem_list_count; i++) {	     
		if(i) {
		    if (i == (mem_list_count - 1)) {
			if (flat_buf->blocklens[k] == end_brd_size)
			    brd_size = flat_buf->blocklens[(k+1)%
							  flat_buf->count];
			else {
			    brd_size = flat_buf->blocklens[k] - end_brd_size;
			    k--;
			    buf_count--;
			}
		    }
		}
		buf_count++;
		k = (k + 1)%flat_buf->count;
	    } /* for (i=0; i<mem_list_count; i++) */
	    for (i=0; i<file_list_count; i++) {
		if (i) {
		    if (i == (file_list_count - 1)) {
			if (flat_file->blocklens[j] == end_frd_size)
			    frd_size = flat_file->blocklens[(j+1)%
							  flat_file->count];   
			else {
			    frd_size = flat_file->blocklens[j] - end_frd_size;
			    j--;
			}
		    }
		}
		if (j < flat_file->count - 1) j++;
		else {
		    j = 0;
		    n_filetypes++;
		}
	    } /* for (i=0; i<file_list_count; i++) */
	    size_read += new_buffer_read;
	    start_k = k;
	    start_j = j;
	    if (max_mem_list < mem_list_count)
	        max_mem_list = mem_list_count;
	    if (max_file_list < file_list_count)
	        max_file_list = file_list_count;
	    if (max_mem_list == max_mem_list == MAX_ARRAY_SIZE)
	        break;
	} /* while (size_read < bufsize) */

	
	mem_starts = (void **)ADIOI_Malloc(max_mem_list * sizeof(void *));
	mem_sizes = (size_t *)ADIOI_Malloc(max_mem_list * sizeof(size_t));
	file_starts = (uint64_t *)ADIOI_Malloc(max_file_list * sizeof(uint64_t));
	file_sizes = (uint64_t *)ADIOI_Malloc(max_file_list * sizeof(uint64_t));
	/* TODO: check the results of mem allocation */
	
	size_read = 0;
	n_filetypes = st_n_filetypes;
	frd_size = st_frd_size;
	brd_size = flat_buf->blocklens[0];
	buf_count = 0;
	start_mem_offset = 0;
	start_k = k = 0;
	start_j = st_index;

	/*  this section calculates mem_list_count and file_list_count
	    and also finds the possibly odd sized last array elements
	    in new_frd_size and new_brd_size  */
	
	while (size_read < bufsize) {
	    k = start_k;
	    new_buffer_read = 0;
	    mem_list_count = 0;
	    while ((mem_list_count < MAX_ARRAY_SIZE) && 
		   (new_buffer_read < bufsize-size_read)) {
	        /* find mem_list_count and file_list_count such that both are
		   less than MAX_ARRAY_SIZE, the sum of their lengths are
		   equal, and the sum of all the data read and data to be
		   read in the next immediate read list is less than
		   bufsize */
	        if(mem_list_count) {
		    if((new_buffer_read + flat_buf->blocklens[k] + 
			size_read) > bufsize) {
		        end_brd_size = new_buffer_read + 
			    flat_buf->blocklens[k] - (bufsize - size_read);
			new_buffer_read = bufsize - size_read;
		    }
		    else {
		        new_buffer_read += flat_buf->blocklens[k];
			end_brd_size = flat_buf->blocklens[k];
		    }
		}
		else {
		    if (brd_size > (bufsize - size_read)) {
		        new_buffer_read = bufsize - size_read;
			brd_size = new_buffer_read;
		    }
		    else new_buffer_read = brd_size;
		}
		mem_list_count++;
		k = (k + 1)%flat_buf->count;
	     } /* while ((mem_list_count < MAX_ARRAY_SIZE) && 
	       (new_buffer_read < bufsize-size_read)) */
	    j = start_j;
	    new_file_read = 0;
	    file_list_count = 0;
	    while ((file_list_count < MAX_ARRAY_SIZE) && 
		   (new_file_read < new_buffer_read)) {
	        if(file_list_count) {
		    if((new_file_read + flat_file->blocklens[j]) > 
		       new_buffer_read) {
		        end_frd_size = new_buffer_read - new_file_read;
			new_file_read = new_buffer_read;
			j--;
		    }
		    else {
		        new_file_read += flat_file->blocklens[j];
			end_frd_size = flat_file->blocklens[j];
		    }
		}
		else {
		    if (frd_size > new_buffer_read) {
		        new_file_read = new_buffer_read;
			frd_size = new_file_read;
		    }
		    else new_file_read = frd_size;
		}
		file_list_count++;
		if (j < (flat_file->count - 1)) j++;
		else j = 0;
		
		k = start_k;
		if ((new_file_read < new_buffer_read) && 
		    (file_list_count == MAX_ARRAY_SIZE)) {
		    new_buffer_read = 0;
		    mem_list_count = 0;
		    while (new_buffer_read < new_file_read) {
		        if(mem_list_count) {
			    if((new_buffer_read + flat_buf->blocklens[k]) >
			       new_file_read) {
			        end_brd_size = new_file_read - new_buffer_read;
				new_buffer_read = new_file_read;
				k--;
			    }
			    else {
			        new_buffer_read += flat_buf->blocklens[k];
				end_brd_size = flat_buf->blocklens[k];
			    }
			}
			else {
			    new_buffer_read = brd_size;
			    if (brd_size > (bufsize - size_read)) {
			        new_buffer_read = bufsize - size_read;
				brd_size = new_buffer_read;
			    }
			}
			mem_list_count++;
			k = (k + 1)%flat_buf->count;
		    } /* while (new_buffer_read < new_file_read) */
		} /* if ((new_file_read < new_buffer_read) && (file_list_count
		     == MAX_ARRAY_SIZE)) */
	    } /* while ((mem_list_count < MAX_ARRAY_SIZE) && 
		 (new_buffer_read < bufsize-size_read)) */

	    /*  fills the allocated readlist arrays  */
	    k = start_k;
	    j = start_j;
	    for (i=0; i<mem_list_count; i++) {	     
	      mem_starts[i] = (buf + buftype_extent*
			       (buf_count/flat_buf->count) +
			       (int)flat_buf->indices[k]);
		if(!i) {
		    mem_sizes[0] = brd_size;
		    mem_starts[0] += flat_buf->blocklens[k] - brd_size;
		}
		else {
		    if (i == (mem_list_count - 1)) {
		        mem_starts[i] = end_brd_size;
			if (flat_buf->blocklens[k] == end_brd_size)
			    brd_size = flat_buf->blocklens[(k+1)%
							  flat_buf->count];
			else {
			    brd_size = flat_buf->blocklens[k] - end_brd_size;
			    k--;
			    buf_count--;
			}
		    }
		    else {
		        mem_sizes[i] = flat_buf->blocklens[k];
		    }
		}
		buf_count++;
		k = (k + 1)%flat_buf->count;
	    } /* for (i=0; i<mem_list_count; i++) */
	    for (i=0; i<file_list_count; i++) {
	        file_starts[i] = disp + flat_file->indices[j] + n_filetypes *
		    filetype_extent;
	        if (!i) {
		    file_sizes[0] = frd_size;
		    file_starts[0] += flat_file->blocklens[j] - frd_size;
		}
		else {
		    if (i == (file_list_count - 1)) {
		        file_sizes[i] = end_frd_size;
			if (flat_file->blocklens[j] == end_frd_size)
			    frd_size = flat_file->blocklens[(j+1)%
							  flat_file->count];   
			else {
			    frd_size = flat_file->blocklens[j] - end_frd_size;
			    j--;
			}
		    }
		    else file_sizes[i] = flat_file->blocklens[j];
		}
		if (j < flat_file->count - 1) j++;
		else {
		    j = 0;
		    n_filetypes++;
		}
	    } /* for (i=0; i<file_list_count; i++) */

	    ret = zoidfs_read(&(zoidfs_fs->fhandle),
			      mem_list_count,
			      mem_starts,
			      mem_sizes,
			      file_list_count,
			      file_starts,
			      file_sizes);
	    /* CHECK: mem_list_count or max_mem_list? */

	    /* --BEGIN ERROR HANDLING-- */
	    if (ret != ZFS_OK) {
		*error_code = MPIO_Err_create_code(MPI_SUCCESS,
						   MPIR_ERR_RECOVERABLE,
						   myname, __LINE__,
						   MPI_ERR_IO,
						   "Error in zoidfs_read (zoidfs errno %d)", len);
	    }
	    /* --END ERROR HANDLING-- */
	    total_bytes_read += file_sizes[0]; /* CHECK: correct? */
	    size_read += new_buffer_read;
	    start_k = k;
	    start_j = j;
	} /* while (size_read < bufsize) */
	ADIOI_Free(mem_starts);
	ADIOI_Free(mem_sizes);
    }
    ADIOI_Free(file_starts);
    ADIOI_Free(file_sizes);
    
    /* Other ADIO routines will convert absolute bytes into counts of datatypes */
    if (file_ptr_type == ADIO_INDIVIDUAL) fd->fp_ind += total_bytes_read;
    if (err_flag == 0) *error_code = MPI_SUCCESS;

error_state:
    fd->fp_sys_posn = -1;   /* set it to null. */

#ifdef HAVE_STATUS_SET_BYTES
    MPIR_Status_set_bytes(status, datatype, bufsize);
    /* This is a temporary way of filling in status. The right way is to 
       keep track of how much data was actually read and placed in buf 
       by ADIOI_BUFFERED_READ. */
#endif
    
    if (!buftype_is_contig) ADIOI_Delete_flattened(datatype);
}

/*
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
