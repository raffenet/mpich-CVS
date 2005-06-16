#include "ad_ln_lnio.h"
#include "adio.h"
#include "adio_extern.h"

#define LNIO_ERROR_CHECK(cond, ret, msg, ecode) {if (cond) { printf("ERROR: %s (return value %d) (%s:%d)\n", msg, ret, __FILE__, __LINE__); errno = ecode; return ret;}}

#define LNIO_DEBUG 0

void pmaps(struct lnio_handle_t *handle);
int ADIOI_LNIO_File_exists(char *);

/* TODO: Be sure to flush the buffer if buffer size has been changed by 
   calling set_view */


int ADIOI_LNIO_Debug_msg(char *format, ... )
{ /* almost same as lorsDebugPrint from lors_util.c in LoRS library */
  if (LNIO_DEBUG) {
    va_list marker;
    
    va_start(marker,format);
    vfprintf(stderr, format,marker);
    fflush(stderr);
    va_end( marker);
  }
}



void ADIOI_LNIO_Free_handle(struct lnio_handle_t *handle) {
  if (!handle->lbone_server) free(handle->lbone_server);
  if (!handle->lbone_location) free(handle->lbone_location);
  if (!handle) free(handle);
}



int ADIOI_LNIO_File_exists(char *fname) {
  struct stat s;
  
  if (stat(fname, &s) == -1 && errno == ENOENT) 
    return 0;
  else return 1;
}



int ADIOI_LNIO_Open(ADIO_File fd) 
{
  int perm, flag, file_exists, tempfd, ret;
  struct lnio_handle_t *handle;
  char *value = NULL, *c;
  int i, num_depot;
  LorsSet *set;

  LorsDepot *dp;
  JRB jrb_node;
  IBP_depot *depot_array;

  
  ADIOI_LNIO_Debug_msg("ADIOI_LNIO_Open: entering (for %s)\n", fd->filename);

  handle = (struct lnio_handle_t *)calloc(1, sizeof(struct lnio_handle_t));
  LNIO_ERROR_CHECK((!handle), -1, "Not enough memory for handle", EIO);
  

  /* parameter setup */
  ADIOI_LNIO_Debug_msg("ADIOI_LNIO_Open: setting up parameters from hints\n");
  
  value = (char *) ADIOI_Malloc((MPI_MAX_INFO_VAL + 1) * sizeof(char));
  LNIO_ERROR_CHECK((!value), -1, "Not enough memory for value", EIO);
  
  MPI_Info_get(fd->info, "LN_LBONE_SERVER", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    handle->lbone_server = strdup(value);
  } else {
    handle->lbone_server = strdup(LBONE_SERVER);
  }
    
  MPI_Info_get(fd->info, "LN_LBONE_PORT", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    handle->lbone_port = atoi(value);
  } else {
    handle->lbone_port = LBONE_PORT;
  }
  
  MPI_Info_get(fd->info, "LN_LBONE_LOCATION", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    handle->lbone_location = strdup(value);
  } else {
    handle->lbone_location = strdup(LBONE_LOCATION);
  }

  MPI_Info_get(fd->info, "LN_LORS_BLOCKSIZE", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    handle->lors_blocksize = atoi(value) * MEGA;
  } else {
    handle->lors_blocksize = LORS_BLOCKSIZE * MEGA;
  }
  
  MPI_Info_get(fd->info, "LN_LORS_DURATION", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    handle->lors_duration = atoi(value);
  } else {
    handle->lors_duration = LORS_DURATION;
  }
  
  MPI_Info_get(fd->info, "LN_LORS_COPIES", MPI_MAX_INFO_VAL, value, &flag);
  if(flag) {
    handle->lors_copies = atoi(value);
  } else {
    handle->lors_copies = LORS_COPIES;
  }
  
  MPI_Info_get(fd->info, "LN_LORS_THREADS", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    handle->lors_threads = atoi(value);
  } else {
    handle->lors_threads = LORS_THREADS;
  }
  
  MPI_Info_get(fd->info, "LN_LORS_TIMEOUT", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    handle->lors_timeout = atoi(value);
  } else {
    handle->lors_timeout = LORS_TIMEOUT;
  }
  
  MPI_Info_get(fd->info, "LN_LORS_SERVERS", MPI_MAX_INFO_VAL, value, &flag);
  if(flag) {
    handle->lors_servers = atoi(value);
  } else {
    handle->lors_servers = LORS_SERVERS;
  }
  
  MPI_Info_get(fd->info, "LN_LORS_SIZE", MPI_MAX_INFO_VAL, value, &flag);
  if(flag) {
    handle->lors_size = atoi(value) * MEGA;
  } else {
    handle->lors_size = LORS_SIZE * MEGA;
  }
  
  MPI_Info_get(fd->info, "LN_LORS_DEMO", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    g_lors_demo = 1;
  } else {
    g_lors_demo = 0;
  }
  
  MPI_Info_get(fd->info, "LN_IO_BUFFER_SIZE", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    handle->buffer_size = atoi(value);
    handle->buffer = (char *)calloc(handle->buffer_size, 1);
    LNIO_ERROR_CHECK((!handle->buffer), -1, "Not enough memory for handle->buffer", EIO);
    /* Should the buffer size be blocksize * threads? */
  } else {
    handle->buffer_size = 0;
    handle->buffer = NULL;
  }

  MPI_Info_get(fd->info, "LN_NO_SYNC_AT_COLLECTIVE_IO", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    handle->sync_at_collective_io = 0;
  } else {
    handle->sync_at_collective_io = 1; 
  }

  /***** TEST PURPOSE *****/
  MPI_Info_get(fd->info, "LN_NONCONTIG_WRITE_NAIVE", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    handle->noncontig_write_naive = 1;
  } else {
    handle->noncontig_write_naive = 0; 
  }

  MPI_Info_get(fd->info, "LN_DEPOT_LIST", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    /* parse the depot list (depot1:port1,depot2:port2,...) */
    /* for now, assume that the string is always in the correct format */
    /* TODO: check if the string is in the correct format */
    num_depot = 0;
    c = value;
    while (c) {
      c = strchr(c, ',');
      num_depot++;
      if (c) c++;
    }
    
    handle->depot_array = (IBP_depot *)calloc(num_depot + 1, sizeof(IBP_depot));
    c = strtok(value, ":");
    for (i = 0; i < num_depot; i++) {
      handle->depot_array[i] = (IBP_depot)calloc(1, sizeof(struct ibp_depot));
      strcpy(handle->depot_array[i]->host, c);
      c = strtok(NULL, ",");
      handle->depot_array[i]->port = atoi(c);
      c = strtok(NULL, ":");
    }
    handle->depot_array[num_depot] = NULL; 
  } else {
    /* depot list is NULL */
    num_depot = 0;
    handle->depot_array = NULL;
  }

  free(value);
  
  ADIOI_LNIO_Debug_msg("ADIOI_LNIO_Open: lbone_server %s \tlbone_port %d\n", handle->lbone_server, handle->lbone_port);
  ADIOI_LNIO_Debug_msg("ADIOI_LNIO_Open: lbone_location %s \tlors_blocksize %d\n", handle->lbone_location, handle->lors_blocksize);
  ADIOI_LNIO_Debug_msg("ADIOI_LNIO_Open: lors_duration %d \tlors_copies %d\n", handle->lors_duration, handle->lors_copies);
  ADIOI_LNIO_Debug_msg("ADIOI_LNIO_Open: lors_threads %d \tlors_timeout %d\n", handle->lors_threads, handle->lors_timeout);
  ADIOI_LNIO_Debug_msg("ADIOI_LNIO_Open: lors_servers %d \tlors_size %d\n", handle->lors_servers, handle->lors_size);


  /*if (!handle->lbone_server && !(fd->access_mode & ADIO_RDONLY || 
				 !fd->access_mode || 
				 fd->access_mode & ADIO_RDWR)) {
    errno = EINVAL;
    return -1;
    }*/  /* Why do we need this? */


  handle->offset = 0;
  handle->exnode_modified = 0;
  
  /* for buffered I/O */
  handle->buf_lb = 0;
  handle->buf_ub = -1;  /* if buf_lb > buf_ub, buf is empty */  
  handle->dirty_buffer = 0;

  
  /* check if the requested file exists */
  /*tempfd = open(fd->filename, O_RDONLY);
  if (tempfd == -1) file_exists = 0; 
  else {
    file_exists = 1;
    close(tempfd);
    }*/

  system("sync;sync");
  MPI_Barrier(fd->comm);
  
  file_exists = ADIOI_LNIO_File_exists(fd->filename);
  
  ADIOI_LNIO_Debug_msg("ADIOI_LNIO_Open: requested file %s\n", file_exists ? "exists" : "does not exist");


  if (file_exists) {  /* file exists */

    if (fd->access_mode & ADIO_EXCL) {
      ADIOI_LNIO_Free_handle(handle);
      printf("ERROR: ADIO_EXCL is enabled and the requested file exists (%s:%d)\n", __FILE__, __LINE__);
      errno = EEXIST;
      return -1;
    } 


    /* if RDONLY or RDWR, deserialize exnode from the file 
       if WRONLY, create an exnode */
    if (fd->access_mode & ADIO_RDONLY || fd->access_mode & ADIO_RDWR) {
      /* handle->ex will be created when FileDeserialize is called */
      ret = lorsFileDeserialize(&handle->ex, fd->filename, NULL);
      if (ret != LORS_SUCCESS) {
	ADIOI_LNIO_Free_handle(handle);	
	printf("ERROR: lorsFileDeserialize failed (returned %d) (%s:%d)\n", ret, __FILE__, __LINE__);
	errno = EIO;
	return ret;
      } 
      
      pmaps(handle);

      /* put all the mappings in the exnode to "set" */
      ret = lorsQuery(handle->ex, &set, 0, handle->ex->logical_length, 0);
      LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsQuery failed", EIO);
      
    } else { /* WRONLY */
      ret = lorsExnodeCreate(&handle->ex);
      LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsExnodeCreate failed", EIO);
    }

    /* 
       if the file doesn't contain any mappings, 
       1-1. RDONLY - do nothing
       1-2. WRONLY - call lorsGetDepotPool
       1-3. RDWR - call lorsGetDepotPool

       if the file contains mappings
       2-1. RDONLY - call lorsUpdateDepotPool
       2-2. WRONLY - call lorsGetDepotPool
       2-3. RDWR - call lorsUpdateDepotPool, 
                 augment depot_array with the depots created by 
                 lorsUpdateDepotPool, then call lorsGetDepotPool
    */

    if (fd->access_mode & ADIO_WRONLY || jrb_empty(set->mapping_map)) {
      /* for 1-1, 1-2, 1-3, and 2-2 cases */
     
      if (fd->access_mode & ADIO_WRONLY || fd->access_mode & ADIO_RDWR)
	ret = lorsGetDepotPool(&handle->dp,		
			       handle->lbone_server, 
			       handle->lbone_port,/* NULL, 0,*/ 
			       handle->depot_array,
			       handle->lors_servers,
			       handle->lbone_location, 
			       (handle->lors_size - 1) / MEGA + 1,
			       IBP_SOFT, 
			       handle->lors_duration, 
			       handle->lors_threads,
			       handle->lors_timeout, 
			       LORS_CHECKDEPOTS);
      
      if (ret != LORS_SUCCESS) {
	ADIOI_LNIO_Free_handle(handle);
	printf("ERROR: lorsGetDepotPool failed (return value %d) (%s:%d)\n", ret, __FILE__, __LINE__);
	errno = EIO;
	return ret;
      }
    } else { /* for 2-1 and 2-3 */
      ret = lorsUpdateDepotPool(handle->ex, &handle->dp, 
				handle->lbone_server, 
				handle->lbone_port, 
				handle->lbone_location, 
				handle->lors_threads, 
				handle->lors_timeout, 0);
      if(ret != LORS_SUCCESS) {
	ADIOI_LNIO_Free_handle(handle);
	printf("ERROR: lorsUpdateDepotPool failed (return value %d) (%s:%d)\n", ret, __FILE__, __LINE__);
	errno = EIO;
	return ret;
      }
      
      /* this is not safe, but for now, let's leave it */
      handle->dp->duration = handle->lors_duration; 

      if (fd->access_mode & ADIO_RDWR) {
	/* RDWR should be treated specially. Depots for both existing 
	   mappings and future writes should be included in the depot pool. 
	   We first added the depots from existing mapping to the depotpool
	   by calling lorsUpdateDepotPool above. Now, we need to find 
	   depots for future writes */

	/* first, extract depots from handle->ex and add them to 
	 handle->depot_array */
	
	/* this function hasn't been included in the lors library yet, 
	   hence the prefix ADIOI_LNIO_ is added for now */
	ret = ADIOI_LNIO_lorsExtractDepotList(handle->ex, 
					      &handle->depot_array, 
					      num_depot);
	if (ret != LORS_SUCCESS) {
	  ADIOI_LNIO_Free_handle(handle);
	  printf("ERROR: ADIOI_LNIO_lorsExtractDepotList failed (return value %d) (%s:%d)\n", ret, __FILE__, __LINE__);
	  errno = EIO;
	  return ret;
	}
	
	/* then call lorsGetDepotPool */
	ret = lorsGetDepotPool(&handle->dp,		
			       handle->lbone_server, 
			       handle->lbone_port,
			       /*NULL, 0,  */
			       handle->depot_array,
			       handle->lors_servers,
			       handle->lbone_location, 
			       (handle->lors_size - 1) / MEGA + 1,
			       IBP_SOFT, 
			       handle->lors_duration, 
			       handle->lors_threads,
			       handle->lors_timeout, 
			       LORS_CHECKDEPOTS);
	
	if (ret != LORS_SUCCESS) {
	  ADIOI_LNIO_Free_handle(handle);
	  printf("ERROR: lorsGetDepotPool failed (return value %d) (%s:%d)\n", ret, __FILE__, __LINE__);
	  errno = EIO;
	  return ret;
	}
      }
    }
  } else { /* file doesn't exist */
    
    if (!(fd->access_mode & ADIO_CREATE)) {
      ADIOI_LNIO_Free_handle(handle);
      printf("ERROR: the requested file %s does not exist and ADIO_CREATE is not enabled (%s:%d)\n", fd->filename, __FILE__, __LINE__);
      errno = ENOENT;
      return -1;
    }
    
    /* One of the following two cases 
       1. ADIO_WRONLY | ADIO_CREATE 
       2. ADIO_RDWR | ADIO_CREATE 
     
       for both cases, create a depot pool by calling lorsGetDepotPool */

    /* for the test purpose... */
    /*    depot_array = (IBP_depot *)calloc(2, sizeof(IBP_depot));
	  for (i = 0; i < 1; i++)
	  depot_array[i] = (IBP_depot)calloc(1, sizeof(struct ibp_depot));
	  depot_array[1] = NULL; 
	  
	  
	  strcpy(depot_array[0]->host, "ibp.accre.vanderbilt.edu");
	  depot_array[0]->port = 6715;
	  
	  strcpy(depot_array[0]->host, "tsiln.ccs.ornl.gov");
	  depot_array[0]->port = 6714;
    */
    
    ret = lorsGetDepotPool(&handle->dp, 			  
			   /* handle->lbone_server,
			      handle->lbone_port,*/
			   NULL, 0, 
			   handle->depot_array,
			   handle->lors_servers,
			   handle->lbone_location, 
			   (handle->lors_size - 1) / MEGA + 1,
			   IBP_SOFT, 
			   handle->lors_duration, 
			   handle->lors_threads,
			   handle->lors_timeout, 
			   LORS_CHECKDEPOTS);
    if (ret != LORS_SUCCESS) {
      ADIOI_LNIO_Free_handle(handle);
      printf("ERROR: lorsGetDepotPool failed (return value %d) (%s:%d)\n", ret, __FILE__, __LINE__);
      errno = EIO;
      return ret;
    }
    /* TODO: will need to reorder depots in the depot pool */
    
    ret = lorsExnodeCreate(&handle->ex);
    if (ret != LORS_SUCCESS) {
      ADIOI_LNIO_Free_handle(handle);
      printf("ERROR: lorsExnodeCreate failed (return value %d) (%s:%d)\n", ret, __FILE__, __LINE__);
      errno = EIO;
      return ret;
    }
  } 
  

  /*
    jrb_traverse(jrb_node, handle->dp->list) {
    dp = jrb_node->val.v;
    printf("depot: %s:%d\n", dp->depot->host, dp->depot->port);
    };
  */
  
  fd->fs_ptr = (void *)handle;

  pmaps(handle);

  return 0;
}



int ADIOI_LNIO_lorsExtractDepotList(LorsExnode *exnode, IBP_depot **dp_array,
				    int num_user_depot)
{   /* By Scott Atchley */
  JRB node = NULL;
  LorsMapping *lm = NULL;
  int         cnt;
  Dllist      depot_list = NULL, dlnode = NULL;
  IBP_depot   *dp_list = NULL;
  int             i = 0, ret = 0;
  
  depot_list = new_dllist();
  
  /* need to include user-provided depots in the array */
  cnt = num_user_depot; 
  jrb_traverse(node, exnode->mapping_map)
    {
      lm = (LorsMapping *)node->val.v;
      dll_append(depot_list, new_jval_v(&lm->depot));
      cnt++;
    }
  
  if (num_user_depot == 0) 
    dp_list = (IBP_depot *)calloc(cnt + 1, sizeof(IBP_depot));
  else
    dp_list = (IBP_depot *) realloc(*dp_array, (cnt + 1) * sizeof(IBP_depot));
  if ( dp_list == NULL )
    {
      free_dllist(depot_list);
      return LORS_NO_MEMORY;
    }
  
  i=0;
  dp_list[i] = NULL;
  dll_traverse(dlnode, depot_list)
    {
      if ( _lorsFindDepot(dp_list,(IBP_depot)dlnode->val.v) >= 0 ){
	continue;
      }
      dp_list[i] = (IBP_depot)dlnode->val.v;
      i++;
      dp_list[i] = NULL;
    }
  
  free_dllist(depot_list);
  *dp_array = dp_list;
  return LORS_SUCCESS;
}

int ADIOI_LNIO_Close(ADIO_File fd)
{
  int ret;
  struct lnio_handle_t *handle = (struct lnio_handle_t *)fd->fs_ptr;
  
  LNIO_ERROR_CHECK((!handle), -1, "handle is NULL", EIO);

  /* if the file is open for write, flush it */
  if (!(fd->access_mode & ADIO_RDONLY)) {
    ret = ADIOI_LNIO_Flush(fd); 
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "ADIOI_LNIO_Flush failed", EIO);
  }
  
  /* calling lorsExnodeFree can cause problems when we have multiple mappings
     to the same allocation created by this library (some of the data 
     structures are shared among the mappings, and lorsExnodeFree will try 
     to delete them multiple times). For now, we just free handle->ex, 
     but this will need to be fixed in the future */
  
  if (handle->ex) free(handle->ex);
  /*ret = lorsExnodeFree(handle->ex);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsExnodeFree failed", EIO);*/
  
  if (handle->dp) {
    ret = lorsFreeDepotPool(handle->dp);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsFreeDepotPool failed", EIO);
  }
  
  /* TODO: check if we need to handle DELETE_ON_CLOSE case here (is it done in ADIO_Close?) */
  
  ADIOI_LNIO_Free_handle(handle);
  
  return 0;
}



off_t ADIOI_LNIO_Lseek(ADIO_File fd, off_t offset, int whence) 
{
  struct lnio_handle_t *handle = (struct lnio_handle_t *)fd->fs_ptr;
  
  LNIO_ERROR_CHECK((!handle), -1, "handle is NULL", EIO);
  
  ADIOI_LNIO_Debug_msg("ADIOI_LNIO_Lseek: whence: %s, offset: %d\n", whence == SEEK_SET ? "SEEK_SET" : (whence == SEEK_CUR ? "SEEK_CUR" : (whence == SEEK_END ? "SEEK_END" : "wrong whence value")), offset);
  
  switch (whence) {
  case SEEK_SET:
    handle->offset = offset;
    break;
  case SEEK_CUR:
    handle->offset += offset;
    break;
  case SEEK_END:
    handle->offset = handle->ex->logical_length + offset;
    break;
  default:
    printf("ERROR: wrong whence value. should be either SEEK_SET, SEEK_CUR, or SEEK_END (%s:%d)\n", __FILE__, __LINE__);
    errno = EINVAL;
    return -1;
  }
  
  return handle->offset;
}



/* TODO: Combine buffered read with non-buffered read */
ssize_t ADIOI_LNIO_Read(ADIO_File fd, void *buf, size_t count)
{
  struct lnio_handle_t *handle = (struct lnio_handle_t *)fd->fs_ptr;
  int ret;
  LorsSet *set;
  
  LorsEnum list = NULL, it = NULL;
  LorsMapping *mp = NULL;
  lnio_mclist_item *mclist_head, *mclist_ptr, *prev_ptr, *newitem; 
  int found;
  longlong start_offset;
  ulong_t sub_count;

  
  ADIOI_LNIO_Debug_msg("ADIOI_LNIO_Read: entering the function with offset %Ld and count %d\n", handle->offset, count);
  
  LNIO_ERROR_CHECK((!handle), -1, "handle is NULL", EIO);
  LNIO_ERROR_CHECK((!buf), -1, "buf is NULL", EIO);
  LNIO_ERROR_CHECK(fd->access_mode & ADIO_WRONLY, -1, "read cannot be called with ADIO_WRONLY", EINVAL);

  if (count == 0) return 0;  
  if (handle->offset < 0 || handle->offset >= handle->ex->logical_length) {
    return 0;
  }
  
  if (handle->buffer_size > 0)  /* buffered I/O */
    return ADIOI_LNIO_Buffered_read(fd, buf, count);
  
  
  bzero(buf, count);

  ret = lorsQuery(handle->ex, &set, handle->offset, count, 0); 
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsQuery failed", EIO);
  
  if (jrb_empty(set->mapping_map)) {
    /* nothing in the set - do nothing */
    goto done;
  }
  
  
  ret = lorsSetEnum(set, &list);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetEnum failed", EIO);
  
  /* currently LoRS does not support reading a region with holes
     (it returns an error if attempted), this should succeed. 
     What we are doing here is to examine each mapping in the set 
     and identify contiguous extents that we have mappings to 
     ("mapping clusters") */
  
  
  mclist_head = NULL; /* mapping cluster list is currently empty */
  
  /* check each mapping in the set and form a list of mapping clusters */
  while (1) {
    ret = lorsEnumNext(list, &it, &mp);
    if (ret == LORS_END) break;
    
    mclist_ptr = mclist_head;
    prev_ptr = NULL;
    found = 0;
    
    while (mclist_ptr) {
      if (mp->exnode_offset + mp->logical_length - 1 < mclist_ptr->lb - 1 ||
	  mp->exnode_offset > mclist_ptr->ub + 1) {
	prev_ptr = mclist_ptr;
	mclist_ptr = mclist_ptr->next;
      } else {
	ADIOI_LNIO_Debug_msg("ADIOI_LNIO_Read: found a cluster (%Ld-%Ld) that overlaps with a mapping (%Ld-%Ld)\n", mclist_ptr->lb, mclist_ptr->ub, 
	        mp->exnode_offset, mp->exnode_offset + mp->logical_length - 1);

	ret = lorsSetAddMapping(mclist_ptr->set, mp);
	LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetAddMapping failed", EIO);
	if (mp->exnode_offset < mclist_ptr->lb) 
	  mclist_ptr->lb = mp->exnode_offset;
	if (mp->exnode_offset + mp->logical_length - 1 > mclist_ptr->ub)
	  mclist_ptr->ub = mp->exnode_offset + mp->logical_length - 1;

	ADIOI_LNIO_Debug_msg("ADIOI_LNIO_Read: a cluster has now been expanded to (%Ld-%Ld)\n", mclist_ptr->lb, mclist_ptr->ub);
	found = 1;
	break;
      }
    }
    
    if (!found) {  /* if an overlapping cluster doesn't exist, create one */
      newitem = (lnio_mclist_item *)calloc(1, sizeof(lnio_mclist_item));
      newitem->next = NULL;
      ret = lorsSetInit(&newitem->set, handle->lors_blocksize, 1, 0); 
      LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetInit failed", EIO);
      ret = lorsSetAddMapping(newitem->set, mp);
      LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetAddMapping falied", EIO);
      
      newitem->lb = mp->exnode_offset;
      newitem->ub = mp->exnode_offset + mp->logical_length - 1;
      ADIOI_LNIO_Debug_msg("ADIOI_LNIO_Read: there is no cluster that overlaps with mapping (%Ld-%Ld), and now created one\n", newitem->lb, newitem->ub);
      if (!mclist_head) mclist_head = newitem;
      else prev_ptr->next = newitem;
    }
  } 
  
  ret = lorsEnumFree(list);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsEnumFree failed", EIO);
  
  /* traverse the mapping cluster list and issue a separate lorsSetLoad
     for each cluster */
  mclist_ptr = mclist_head;
  while (mclist_ptr) {
    /* set the proper starting offset and count for reading 
       [handle->offset, handle->offset + count) */
    if (mclist_ptr->lb < handle->offset) start_offset = handle->offset;
    else start_offset = mclist_ptr->lb;
    if (mclist_ptr->ub > handle->offset + count - 1) 
      sub_count = handle->offset + count - start_offset;
    else sub_count = mclist_ptr->ub - start_offset + 1;
    
    ADIOI_LNIO_Debug_msg("ADIOI_LNIO_Read: calling lorsSetLoad with offset %Ld, count %ld from buf[%Ld]\n", start_offset, sub_count, start_offset - handle->offset);

    ret = lorsSetLoad(mclist_ptr->set, buf + start_offset - handle->offset, 
		      start_offset, sub_count, handle->lors_blocksize,
		      NULL/*glob_lc*/, handle->lors_threads, 
		      handle->lors_timeout, 0);
    if (ret < 0) {
      printf("ERROR: lorsSetLoad failed (return value %d) (%s:%d)\n", ret, __FILE__, __LINE__);
      errno = EIO;
      return ret;
    }
    
    prev_ptr = mclist_ptr;
    mclist_ptr = mclist_ptr -> next;
    lorsSetFree(prev_ptr->set, 0);
    free(prev_ptr);
  }
  

 done:
  ret = lorsSetFree(set, 0);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetFree failed", EIO);
  
  if (handle->offset + count < handle->ex->logical_length) {
    handle->offset += count;
  } else {
    count = handle->ex->logical_length - handle->offset;
    handle->offset = handle->ex->logical_length - 1;
  }
  return count;
}



ssize_t ADIOI_LNIO_Buffered_read(ADIO_File fd, void *buf, size_t count)
{
  struct lnio_handle_t *handle = (struct lnio_handle_t *)fd->fs_ptr;
  off_t buf_lb, buf_ub, req_lb, req_ub, overlap_lb, overlap_ub;
  int ret;

  LorsSet *set;
  LorsEnum list = NULL, it = NULL;
  LorsMapping *mp = NULL;
  lnio_mclist_item *mclist_head, *mclist_ptr, *prev_ptr, *newitem; 
  int found;
  longlong start_offset;
  ulong_t sub_count;


  printf("Entering ADIOI_LNIO_Buffered_read - cur_buf [%Ld...%Ld], cur_offset %ld\n", handle->buf_lb, handle->buf_ub, handle->offset);

  if (count == 0) return 0;

  if (!handle || !buf) return -1;

  if (fd->access_mode & ADIO_WRONLY) {
    errno = EINVAL;
    return -1;
  }

  /*  if (handle->offset < 0 || handle->offset >= handle->ex->logical_length) {
      return 0;
      } <-- Do Something About THIS! */
  
  
  buf_lb = handle->buf_lb;
  buf_ub = handle->buf_ub;
  req_lb = handle->offset;
  req_ub = handle->offset + count - 1;

  printf("buf [%Ld...%Ld], req [%Ld...%Ld]\n", buf_lb, buf_ub, req_lb, req_ub);

  /* if the requested data are contained in the current buffer, 
     simply return the buffered data */
  if (buf_lb <= req_lb && req_ub <= buf_ub) {
    printf("!memcpy from handle->buffer[%Ld] to buf for %d bytes\n", req_lb - buf_lb, count);
    memcpy(buf, handle->buffer + (req_lb - buf_lb), count);
    handle->offset += count;
    return count;
  }  

  /* otherwise, read the file from the network except for the buf region, 
     and then fill the hole with the buffered data */
  
  ret = lorsQuery(handle->ex, &set, handle->offset, count, 0); 
  if (ret != LORS_SUCCESS) {
    errno = EIO;
    return 0; /* not -1? */
  }
  
  if (jrb_empty(set->mapping_map)) {
    /* nothing in the set - data not written yet (not an error) */
    lorsSetFree(set, 0);
    handle->offset += count;
    bzero(buf, count);
    return count;
  }

  

  ret = lorsSetEnum(set, &list);
  if (ret != LORS_SUCCESS) return ret;
  
  mclist_head = NULL; /* mapping cluster list is currently empty */
  
  /* check each mapping in the set and form a list of mapping clusters */
  while (1) {
    ret = lorsEnumNext(list, &it, &mp);
    if (ret == LORS_END) break;

    /* skip the mappings that are contained in current buffer */
    if (buf_lb <= mp->exnode_offset && 
	mp->exnode_offset + mp->logical_length - 1 <= buf_ub) continue;

    mclist_ptr = mclist_head;
    prev_ptr = NULL;
    found = 0;
    
    while (mclist_ptr) {
      if (mp->exnode_offset + mp->logical_length - 1 < mclist_ptr->lb - 1 ||
	  mp->exnode_offset > mclist_ptr->ub + 1) {
	prev_ptr = mclist_ptr;
	mclist_ptr = mclist_ptr->next;
      } else {
	printf("READ: found a cluster (%Ld-%Ld) that overlaps with a mapping (%Ld-%Ld)\n", mclist_ptr->lb, mclist_ptr->ub, mp->exnode_offset, 
	       mp->exnode_offset + mp->logical_length - 1);
	ret = lorsSetAddMapping(mclist_ptr->set, mp);
	if (ret != LORS_SUCCESS) return ret;
	if (mp->exnode_offset < mclist_ptr->lb) 
	  mclist_ptr->lb = mp->exnode_offset;
	if (mp->exnode_offset + mp->logical_length - 1 > mclist_ptr->ub)
	  mclist_ptr->ub = mp->exnode_offset + mp->logical_length - 1;
	printf("READ: a cluster has now been expanded to (%Ld-%Ld)\n", mclist_ptr->lb, mclist_ptr->ub); 
	found = 1;
	break;
      }
    }
    
    if (!found) {
      newitem = (lnio_mclist_item *)calloc(1, sizeof(lnio_mclist_item));
      newitem->next = NULL;
      ret = lorsSetInit(&newitem->set, handle->lors_blocksize, 1, 0); 
      if (ret != LORS_SUCCESS) return ret;
      lorsSetAddMapping(newitem->set, mp);
      if (ret != LORS_SUCCESS) return ret;
      newitem->lb = mp->exnode_offset;
      newitem->ub = mp->exnode_offset + mp->logical_length - 1;
      printf("READ: there is no cluster that overlaps with mapping (%Ld-%Ld), and now created one\n", newitem->lb, newitem->ub);
      if (!mclist_head) mclist_head = newitem;
      else prev_ptr->next = newitem;
    }
  } 
  
  ret = lorsEnumFree(list);
  if (ret != LORS_SUCCESS) return ret;

  /* traverse the mapping cluster list and issue a separate lorsSetLoad
     for each cluster */
  mclist_ptr = mclist_head;
  while (mclist_ptr) {
    if (mclist_ptr->lb < handle->offset) start_offset = handle->offset;
    else start_offset = mclist_ptr->lb;
    if (mclist_ptr->ub > handle->offset + count - 1) 
      sub_count = handle->offset + count - start_offset;
    else sub_count = mclist_ptr->ub - start_offset + 1;
    
    printf("READ: issuing lorsSetLoad with offset %Ld, count %ld from buf[%Ld]\n", start_offset, sub_count, start_offset - handle->offset);
    if (jrb_empty(mclist_ptr->set->mapping_map)) printf("READ: mclist_ptr->set is empty\n"); else printf("READ: mclist_ptr->set is not empty\n");

    ret = lorsSetLoad(mclist_ptr->set, buf + start_offset - handle->offset, 
		      start_offset, sub_count, handle->lors_blocksize,
		      NULL/*glob_lc*/, handle->lors_threads, 
		      handle->lors_timeout, 0);
    if (ret < 0) {
      printf("lorsSetLoad error with an error code %d\n", ret);
      return ret;
    }
    
    prev_ptr = mclist_ptr;
    mclist_ptr = mclist_ptr -> next;
 
    lorsSetFree(prev_ptr->set, 0);
    free(prev_ptr);
  }
  
  /* copy the content in the current buffer into the user buffer */
  if (buf_lb <= buf_ub) { /* if the current buffer is not empty */ 
    if (buf_lb > req_lb) overlap_lb = buf_lb; else overlap_lb = req_lb;
    if (buf_ub < req_ub) overlap_ub = buf_ub; else overlap_ub = req_ub;
    if (overlap_lb <= overlap_ub) { /* if they overlap */
      printf("memcpy from handle->buffer[%Ld] to buf[%Ld] for %Ld bytes\n", overlap_lb - buf_lb, overlap_lb - req_lb, overlap_ub - overlap_lb + 1);
      memcpy(buf + (overlap_lb - req_lb), handle->buffer + (overlap_lb - buf_lb), overlap_ub - overlap_lb + 1);
    }
  }

  /* update the content of buffer? - make sure there are no holes in the 
     data contained in the buffer*/
  /* need to do something cleverer here */
  
  lorsSetFree(set, 0); 

  handle->offset += count;

  return count;
}



int ADIOI_LNIO_lorsSetMappingVersion(LorsSet *set, int version) {
  LorsEnum list = NULL, it = NULL;
  LorsMapping *mp = NULL;
  ExnodeValue val;
  int ret;
  
  /* set the mapping version of each mapping in the "set" to "version" */
  
  ret = lorsSetEnum (set, &list);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetEnum failed", EIO);
  
  while (1) { 
    ret = lorsEnumNext(list, &it, &mp);
    if (ret == LORS_END) break;
    val.i = version;
    if (!mp->md) {
      ret = exnodeCreateMetadata(&mp->md);
      LNIO_ERROR_CHECK(ret != EXNODE_SUCCESS, ret, "exnodeCreateMetadata failed", EIO);
    }
    /* lorsGetMappingMetadata doesn't call exnodeCreateMetadata 
       when md is NULL. Should we report this? */
    ret = exnodeSetMetadataValue(mp->md, "mapping_version", val, INTEGER, TRUE);
    LNIO_ERROR_CHECK(ret != EXNODE_SUCCESS, ret, "exnodeSetMetadataValue failed", EIO);
  } 
  
  ret = lorsEnumFree(list);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsEnumFree failed", EIO);
  
  return LORS_SUCCESS;
}



/* TODO: combine this with buffered write */
ssize_t ADIOI_LNIO_Write(ADIO_File fd, const void *buf, size_t count) 
{
  struct lnio_handle_t *handle = (struct lnio_handle_t *)fd->fs_ptr;
  int ret;
  LorsSet *set, *newset;
  
#ifdef YING_LORS_DEBUG
  LorsSet *tmpSet;
  LorsEnum list = NULL, iter = NULL;
  int lret=0;
  struct ibp_timer timer;
  int nbytes = 0;
  char *tmpbuf=NULL;
#endif

 ADIOI_LNIO_Debug_msg("ADIOI_LNIO_Write: entering the function with offset %Ld and count %d\n", handle->offset, count);
  
  LNIO_ERROR_CHECK((!handle), -1, "handle is NULL", EIO);
  LNIO_ERROR_CHECK((!buf), -1, "buf is NULL", EIO);
  LNIO_ERROR_CHECK(fd->access_mode & ADIO_RDONLY, -1, "write cannot be called with ADIO_RDONLY", EINVAL);

  if (count == 0) return 0;  
  
  if (handle->buffer_size > 0)  /* buffered I/O */
    return ADIOI_LNIO_Buffered_write(fd, buf, count);
  

  /*  
      if (count > handle->lors_blocksize * handle->lors_threads) {
      count = handle->lors_blocksize * handle->lors_threads;
      }
  */
  
  
  /* lorsQuery will allocate memory for "set" */
  /* (is this confusing? for lorsSetStore and lorSetLoad, set needs to be 
     allocated and initialized beforehand) */
  ret = lorsQuery(handle->ex, &set, handle->offset, count, LORS_QUERY_REMOVE);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsQuery failed", EIO);
  
  if (jrb_empty(set->mapping_map)) {
    /* empty set - no need to update mappings */
    
    /* "set" has already been allocate -> no need to call lorsSetInit */
    /* but, need to set the parameters for "set" as in lorsSetInit */
    set->max_length = 0;
    set->exnode_offset = 0;
    set->data_blocksize = handle->lors_blocksize;
    set->copies = handle->lors_copies;
    
    ret = lorsSetStore(set, handle->dp, (char *)buf, 
		       handle->offset, count, NULL, 
		       handle->lors_threads, handle->lors_timeout, 
		       LORS_RETRY_UNTIL_TIMEOUT);    
    if (ret != LORS_SUCCESS) {
      lorsSetFree(set, LORS_FREE_MAPPINGS);
      printf("ERROR: lorsSetStore failed (return value %d) (%s:%d)\n", ret, __FILE__, __LINE__);
      errno = EIO;
      return ret;
    }    
  
    /* set the version of each new mapping in the set to 1 */
    ret = ADIOI_LNIO_lorsSetMappingVersion(set, 1);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "ADIOI_LNIO_lorsSetMappingVersion failed", EIO);

  } else {
    /* likewise, set the parameters */
    set->copies = handle->lors_copies;
    set->data_blocksize= handle->lors_blocksize; 
    
    /* basically repeating what lorsSetUpdate does */

    /* TODO: check if we have to pass lors_copies to lorsSetInit */
    ret = lorsSetInit(&newset, handle->lors_blocksize, 1, 0);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetInit failed", EIO);
    
    ret = lorsSetStore(newset, handle->dp, (char *)buf, 
		       handle->offset, count, NULL, 
		       handle->lors_threads, 
		       handle->lors_timeout, 
		       LORS_RETRY_UNTIL_TIMEOUT);
    
    if (ret != LORS_SUCCESS) {
      lorsSetFree(newset, LORS_FREE_MAPPINGS);
      printf("ERROR: lorsSetStore failed (return value %d) (%s:%d)\n", ret, __FILE__, __LINE__);
      errno = EIO;
      return ret;
    }
    
    /* set the version of each new mapping to 1 
       (touched since the last sync) */
    ret = ADIOI_LNIO_lorsSetMappingVersion(newset, 1);
    if (ret != LORS_SUCCESS) {
      lorsSetFree(newset, LORS_FREE_MAPPINGS);
      printf("ERROR: ADIOI_LNIO_lorsSetMappingVersion failed (return value %d) (%s:%d)\n", ret, __FILE__, __LINE__);
      errno = EIO;
      return ret;
    }

    /* now trim the portion overlapped with new mappings from "set" */
    ret = lorsSetTrim(set, handle->offset, count, handle->lors_threads,
		      handle->lors_timeout, LORS_RETRY_UNTIL_TIMEOUT | 
		      LORS_TRIM_ALL);
    if (ret != LORS_SUCCESS) {
      lorsSetFree(newset, LORS_FREE_MAPPINGS);
      printf("ERROR: lorsSetTrim failed (return value %d) (%s:%d)\n", ret, __FILE__, __LINE__);
      errno = EIO;
      return ret;
    }
    
    /* then merge the two sets */
    ret = lorsSetMerge(newset, set);
    if (ret != LORS_SUCCESS) {
      lorsSetFree(newset, LORS_FREE_MAPPINGS);
      printf("ERROR: lorsSetMerge failed (return value %d) (%s:%d)\n", ret, __FILE__, __LINE__);
      errno = EIO;
      return ret;
    }
    
    ret = lorsSetFree(newset, 0);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetFree failed", EIO);
  }
  
  /* append the updated set to the exnode */
  ret = lorsAppendSet(handle->ex, set);
  if (ret != LORS_SUCCESS) {
    lorsSetFree(set, LORS_FREE_MAPPINGS);
    printf("ERROR: lorsAppendSet failed (return value %d) (%s:%d)\n", ret, __FILE__, __LINE__);
    errno = EIO;
    return ret;
  }
  
  ret = lorsSetFree(set, 0);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetFree failed", EIO);
  
  handle->offset += count; 
  handle->exnode_modified = 1;
  
  return count;
}



ssize_t ADIOI_LNIO_Buffered_write(ADIO_File fd, const void *buf, size_t count)
{
  struct lnio_handle_t *handle = (struct lnio_handle_t *)fd->fs_ptr;
  off_t cur_lb, cur_ub, new_lb, new_ub, max_ub, min_lb, flush_start_offset;
  size_t i, subcount, flush_size;
  ssize_t nwritten;

  printf("Entering ADIOI_LNIO_Buffered_write\n");

  if (!handle || !buf) return -1;
  
  if (count == 0) return 0;
  
  if (fd->access_mode == ADIO_RDONLY) {
    errno = EINVAL;
    return -1;
  }

  nwritten = 0;
  for (i = 0; i < count; i += handle->buffer_size) {
    /* unit of transfer = handle->buffer_size */

    if (i + handle->buffer_size < count) subcount = handle->buffer_size;
    else subcount = count - i;

    printf("Buffered_write: subcount %d, current offset %ld\n", subcount, handle->offset);

    cur_lb = handle->buf_lb;
    cur_ub = handle->buf_ub;
    new_lb = handle->offset;
    new_ub = handle->offset + subcount - 1;

    if (cur_ub > new_ub) max_ub = cur_ub; else max_ub = new_ub;
    if (cur_lb < new_lb) min_lb = cur_lb; else min_lb = cur_lb;

    /* case 1: where we have to flush the old buffer and fill the buffer */
    /*         with the new data                                         */
    /*         - there is a hole in [min_lb, max_ub]                     */
    /*          (e.g.  nnn ccc  or ccc nnn)                              */
    /*         - cannot put all the data in the current buffer           */
    /*          (i.e. max_ub - min_lb + 1 > buffer_size)                 */

    if (new_ub < cur_lb - 1 || cur_ub + 1 < new_lb || 
	max_ub - min_lb + 1 > handle->buffer_size) {
      printf("Buffered_write: Case 1\n");
      if (handle->dirty_buffer) {
	/* flush the current buffer */
	flush_start_offset = cur_lb;
	flush_size = subcount;
	if (max_ub - min_lb + 1 > handle->buffer_size) {
	  /* flush only the non-overwritten portion */
	  if (cur_lb <= new_lb) flush_size = new_lb - cur_lb;
	  else {
	    flush_start_offset = new_ub + 1;
	    flush_size = cur_ub - new_ub;
	  }
	}
	printf("Buffered write: calling Buffer Flush with size %d, offset %ld\n", flush_size, flush_start_offset);
	ADIOI_LNIO_Buffer_flush(fd, flush_start_offset, flush_size);
      }

      memcpy(handle->buffer, buf + i, subcount);
      handle->buf_lb = new_lb;
      handle->buf_ub = new_ub;
      handle->offset += subcount;
      nwritten += subcount;
      handle->dirty_buffer = 1;
    } else 

      /* case 2: cur_lb <= new_lb */
      /* cur:   ccccc      |   ccccc   |   ccccc   |   ccc   */
      /* new:      nnnnn   |    nnn    |   nnn     |   nnnnn */
      /* res:   cccnnnnn   |   cnnnc   |   nnncc   |   nnnnn */
      
      if (cur_lb <= new_lb) {
	/* update the buffer, no need to flush */
	memcpy(handle->buffer + (new_lb - cur_lb), buf + i, subcount);
      handle->buf_ub = max_ub;
      nwritten += subcount;
      handle->offset += subcount;
      handle->dirty_buffer = 1;

      printf("Buffered_write: Case 2 - now buffer [%ld...%ld], handle->offset %ld, nwritten %d\n", handle->buf_lb, handle->buf_ub, handle->offset, nwritten);
    } else
    
    /* case 3: new_lb < cur_lb */
    /* cur:           ccccc     |      ccc          */
    /* new:        nnnnn        |     nnnnn         */
    /* res:        nnnnnccc     |     nnnnn         */

    if (new_lb < cur_lb) {
      printf("Buffered_write: Case 3\n");
      /* move the data in the current buffer to an appropriate position */
      memmove(handle->buffer + (cur_lb - new_lb), handle->buffer, 
	      cur_ub - cur_lb + 1);
      /* overwrite the buffer */
      memcpy(handle->buffer, buf + i, subcount);
      handle->buf_lb = new_lb;
      handle->buf_ub = max_ub;
      nwritten+= subcount;
      handle->offset += subcount;
      handle->dirty_buffer = 1;
    }
  }

  return nwritten;
}

void pmaps(struct lnio_handle_t *handle) 
{
  int i;
  JRB node;
  LorsMapping *lm;

  i = 0;
  jrb_traverse(node, handle->ex->mapping_map) {
    lm = node->val.v;
    printf("mapping[%d]: ex_off %Ld log_len %ld alloc_off %ld metadata %s\n", i, lm->exnode_offset, lm->logical_length, lm->alloc_offset, lm->md ? "not NULL" : "NULL");
    i++;
  }
  if (i == 0) printf("No mappings in handle->ex\n");
}


/* TODO: need to flush buffer if buffered I/O is used and the buffer 
   overlaps with the extent of requested strided write */
/* noncontingous write implemented by packing data into contiguous 
   buffer, writing it to a single allocation, and creating multiple 
   mappings to it */
int ADIOI_LNIO_WriteStrided(ADIO_File fd, void *buf, int count,
			    MPI_Datatype buftype, int file_ptr_type,
			    ADIO_Offset offset, ADIO_Status *status)
{
  struct lnio_handle_t *handle = (struct lnio_handle_t *)fd->fs_ptr;
  
  struct ncmap_t {
    ADIO_Offset logical_offset;
    ADIO_Offset packed_offset;
    ADIO_Offset length
  } *ncmap;
  
  LorsEnum list = NULL, it = NULL;
  LorsMapping *lm = NULL, *new_lm = NULL;
  ExnodeValue val;
  LorsSet *set, *wholeset;
  int ncmap_cnt, ncmap_size;
  ADIO_Offset packed_off, acc_blk_size, new_len;
  char *packed_buf;
  int ret, i, j;
  JRB node;
  int lm_array_size, lm_array_cnt;
  LorsMapping **lm_array;

  ADIOI_Flatlist_node *flat_buf, *flat_file;
  /* bwr == buffer write; fwr == file write */
  int bwr_size, fwr_size=0, b_index;
  int bufsize, size, sum, n_etypes_in_filetype, size_in_filetype;
  int n_filetypes, etype_in_filetype;
  ADIO_Offset abs_off_in_filetype=0;
  int filetype_size, etype_size, buftype_size, req_len;
  MPI_Aint filetype_extent, buftype_extent; 
  int buf_count, buftype_is_contig;
  ADIO_Offset userbuf_off;
  ADIO_Offset off, req_off, disp, end_offset=0, start_off;
  ADIO_Status status1;
  
  int f_index, st_fwr_size, st_index = 0, st_n_filetypes;
  int flag; 
  
  /* LN-specific optimization for noncontiguous writes */
  /* (noncontiguous in file only) */
  
  /* offset is in units of etype relative to the filetype. */


  pmaps(handle);
  
  ADIOI_Datatype_iscontig(buftype, &buftype_is_contig);
  
  MPI_Type_size(fd->filetype, &filetype_size);
  if ( ! filetype_size ) {
    /* *error_code = MPI_SUCCESS;*/ 
    return -1;
  }
  
  MPI_Type_extent(fd->filetype, &filetype_extent);
  MPI_Type_size(buftype, &buftype_size);
  MPI_Type_extent(buftype, &buftype_extent);
  etype_size = fd->etype_size;
  
  bufsize = buftype_size * count;

  ncmap_size = 100; /* initial size */
  ncmap = (struct ncmap_t *)malloc(sizeof(struct ncmap_t) * ncmap_size);


  ret = lorsQuery(handle->ex, &wholeset, 0, handle->ex->logical_length, LORS_QUERY_REMOVE);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsQuery failed", EIO);

  /* noncontiguous in file */
 
  
  /* First we're going to calculate a set of values for use in all
   * the noncontiguous in file cases:
   * start_off - starting byte position of data in file
   * end_offset - last byte offset to be acessed in the file
   * st_n_filetypes - how far into the file we start in terms of
   *                  whole filetypes
   * st_index - index of block in first filetype that we will be
   *            starting in (?)
   * st_fwr_size - size of the data in the first filetype block
   *               that we will write (accounts for being part-way
   *               into writing this block of the filetype
   *
   */
  
  /* filetype already flattened in ADIO_Open */
  flat_file = ADIOI_Flatlist;
  while (flat_file->type != fd->filetype) flat_file = flat_file->next;
  disp = fd->disp;
  
  if (file_ptr_type == ADIO_INDIVIDUAL) {
    start_off = fd->fp_ind; /* in bytes */
    n_filetypes = -1;
    flag = 0;
    while (!flag) {
      n_filetypes++;
      for (f_index=0; f_index < flat_file->count; f_index++) {
	if (disp + flat_file->indices[f_index] + 
	    (ADIO_Offset) n_filetypes*filetype_extent + 
	    flat_file->blocklens[f_index] >= start_off) 
	  {
	    /* this block contains our starting position */
	    
	    st_index = f_index;
	    fwr_size = (int) (disp + flat_file->indices[f_index] + 
			      (ADIO_Offset) n_filetypes*filetype_extent + 
			      flat_file->blocklens[f_index] - start_off);
	    flag = 1;
	    break;
	  }
      }
    }
  }
  else {
    n_etypes_in_filetype = filetype_size/etype_size;
    n_filetypes = (int) (offset / n_etypes_in_filetype);
    etype_in_filetype = (int) (offset % n_etypes_in_filetype);
    size_in_filetype = etype_in_filetype * etype_size;
    
    sum = 0;
    for (f_index=0; f_index < flat_file->count; f_index++) {
      sum += flat_file->blocklens[f_index];
      if (sum > size_in_filetype) {
	st_index = f_index;
	fwr_size = sum - size_in_filetype;
	abs_off_in_filetype = flat_file->indices[f_index] +
	  size_in_filetype - 
	  (sum - flat_file->blocklens[f_index]);
	break;
      }
    }
    
    /* abs. offset in bytes in the file */
    start_off = disp + (ADIO_Offset) n_filetypes*filetype_extent + 
      abs_off_in_filetype;
  }
  
  st_fwr_size = fwr_size;
  st_n_filetypes = n_filetypes;
  
  /* start_off, st_n_filetypes, st_index, and st_fwr_size are 
   * all calculated at this point
   */
  
  /* Calculate end_offset, the last byte-offset that will be accessed.
   * e.g., if start_off=0 and 100 bytes to be written, end_offset=99
   */
  userbuf_off = 0;
  f_index = st_index;
  off = start_off;
  fwr_size = ADIOI_MIN(st_fwr_size, bufsize);
  while (userbuf_off < bufsize) {
    userbuf_off += fwr_size;
    end_offset = off + fwr_size - 1;
    
    if (f_index < (flat_file->count - 1)) f_index++;
    else {
      f_index = 0;
      n_filetypes++;
    }
    
    off = disp + flat_file->indices[f_index] + 
      (ADIO_Offset) n_filetypes*filetype_extent;
    fwr_size = ADIOI_MIN(flat_file->blocklens[f_index], 
			 bufsize-(int)userbuf_off);
  }
  
  /* End of calculations.  At this point the following values have
   * been calculated and are ready for use:
   * - start_off
   * - end_offset
   * - st_n_filetypes
   * - st_index
   * - st_fwr_size
   */
  
  /* if atomicity is true, lock (exclusive) the region to be accessed */
  /*if ((fd->atomicity) && (fd->file_system != ADIO_PIOFS) && 
    (fd->file_system != ADIO_PVFS))
    {
    ADIOI_WRITE_LOCK(fd, start_off, SEEK_SET, end_offset-start_off+1);
    }*/
  
  if (buftype_is_contig) {
    /* contiguous in memory, noncontiguous in file. should be the
     * most common case.
     */

    userbuf_off = 0;
    f_index = st_index;
    off = start_off;
    n_filetypes = st_n_filetypes;
    fwr_size = ADIOI_MIN(st_fwr_size, bufsize);
    
    ncmap_cnt = 0;
    /* while there is still space in the buffer, write more data */
    while (userbuf_off < bufsize) {
      if (fwr_size) { 
	/* TYPE_UB and TYPE_LB can result in 
	   fwr_size = 0. save system call in such cases */ 
	req_off = off;
	req_len = fwr_size;
	
	/*ADIO_WriteContig(fd, 
			 (char *) buf + userbuf_off,
			 req_len, 
			 MPI_BYTE, 
			 ADIO_EXPLICIT_OFFSET,
			 req_off,
			 &status1,
			 error_code);
			 if (*error_code != MPI_SUCCESS) return;*/

	if (ncmap_cnt >= ncmap_size) {
	  ncmap_size *= 2;
	  ncmap = (struct ncmap_t *)realloc(ncmap, sizeof(struct ncmap_t) * ncmap_size);
	}
	ncmap[ncmap_cnt].logical_offset = req_off;
	ncmap[ncmap_cnt].packed_offset = userbuf_off;
	ncmap[ncmap_cnt].length = req_len;
	/*printf("ncmap[%d]: logical_offset %Ld packed_offset %Ld, length %Ld\n", ncmap_cnt, req_off, userbuf_off, ncmap[ncmap_cnt].length);*/

	ret = lorsSetTrim(wholeset, req_off, req_len, handle->lors_threads,
			  handle->lors_timeout, LORS_RETRY_UNTIL_TIMEOUT | 
			  LORS_TRIM_ALL);
	LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetTrim failed", EIO);

	ncmap_cnt++;
      }
      userbuf_off += fwr_size;
      
      if (off + fwr_size < disp + flat_file->indices[f_index] +
	  flat_file->blocklens[f_index] + 
	  (ADIO_Offset) n_filetypes*filetype_extent)
	{
	  /* important that this value be correct, as it is
	   * used to set the offset in the fd near the end of
	   * this function.
	   */
	  off += fwr_size;
	}
      /* did not reach end of contiguous block in filetype.
       * no more I/O needed. off is incremented by fwr_size.
       */
      else {
	if (f_index < (flat_file->count - 1)) f_index++;
	else {
	  f_index = 0;
	  n_filetypes++;
	}
	off = disp + flat_file->indices[f_index] + 
	  (ADIO_Offset) n_filetypes*filetype_extent;
	fwr_size = ADIOI_MIN(flat_file->blocklens[f_index], 
			     bufsize-(int)userbuf_off);
      }
    }

    ret = lorsSetInit(&set, handle->lors_blocksize, 1, 0);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetInit failed", EIO);
    
    if (bufsize != userbuf_off) printf("CHECK THE PARAMETER\n");
    printf("calling lorsSetStore with offset %Ld and bufsize %d\n",start_off, bufsize);
    ret = lorsSetStore(set, handle->dp, buf, start_off, bufsize, NULL, 
		       handle->lors_threads, handle->lors_timeout,
		       LORS_RETRY_UNTIL_TIMEOUT);
    if (ret != LORS_SUCCESS) {
      printf("%s %d: lorsSetStore failed\n", __FILE__, __LINE__);
      lorsSetFree(set, LORS_FREE_MAPPINGS);
      return ret;
    }
  }
  else {
    int i, tmp_bufsize = 0;
    /* noncontiguous in memory as well as in file */
    printf("ADIOI_LN_WriteStrided: noncontiguous in mem, noncontiguous in file\n");
    
    ADIOI_Flatten_datatype(buftype);
    flat_buf = ADIOI_Flatlist;
    while (flat_buf->type != buftype) flat_buf = flat_buf->next;
    
    b_index = buf_count = 0;
    i = (int) (flat_buf->indices[0]);
    f_index = st_index;
    off = start_off;
    n_filetypes = st_n_filetypes;
    fwr_size = st_fwr_size;
    bwr_size = flat_buf->blocklens[0];

    ncmap_cnt = 0;
    packed_off = 0;
    packed_buf = (char *)calloc(bufsize, 1); 

    /* while we haven't read size * count bytes, keep going */
    while (tmp_bufsize < bufsize) {
      int new_bwr_size = bwr_size, new_fwr_size = fwr_size;
            
      size = ADIOI_MIN(fwr_size, bwr_size);
      if (size) {
	req_off = off;
	req_len = size;
	userbuf_off = i;
	/*
	  ADIO_WriteContig(fd, 
	  (char *) buf + userbuf_off,
	  req_len, 
	  MPI_BYTE, 
	  ADIO_EXPLICIT_OFFSET,
	  req_off,
	  &status1,
	  error_code);
	  if (*error_code != MPI_SUCCESS) return;*/

	memcpy(packed_buf + packed_off, buf + userbuf_off, req_len);

	if (ncmap_cnt >= ncmap_size) {
	  ncmap_size *= 2;
	  ncmap = (struct ncmap_t *)realloc(ncmap, sizeof(struct ncmap_t) * ncmap_size);
	}
	ncmap[ncmap_cnt].logical_offset = req_off;
	ncmap[ncmap_cnt].packed_offset = packed_off;
	ncmap[ncmap_cnt].length = req_len;

	ret = lorsSetTrim(wholeset, req_off, req_len, handle->lors_threads,
			  handle->lors_timeout, LORS_RETRY_UNTIL_TIMEOUT | 
			  LORS_TRIM_ALL);
	LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetTrim failed", EIO);

	ncmap_cnt++;
	packed_off += req_len;
      }
     
      
      if (size == fwr_size) {
	/* reached end of contiguous block in file */
	if (f_index < (flat_file->count - 1)) f_index++;
	else {
	  f_index = 0;
	  n_filetypes++;
	}
	
	off = disp + flat_file->indices[f_index] + 
	  (ADIO_Offset) n_filetypes*filetype_extent;
	
	new_fwr_size = flat_file->blocklens[f_index];
	if (size != bwr_size) {
	  i += size;
	  new_bwr_size -= size;
	}
      }
      
      if (size == bwr_size) {
	/* reached end of contiguous block in memory */
	
	b_index = (b_index + 1)%flat_buf->count;
		    buf_count++;
		    i = (int) (buftype_extent*(buf_count/flat_buf->count) +
			       flat_buf->indices[b_index]);
		    new_bwr_size = flat_buf->blocklens[b_index];
		    if (size != fwr_size) {
		      off += size;
		      new_fwr_size -= size;
		    }
      }
      tmp_bufsize += size;
      fwr_size = new_fwr_size;
      bwr_size = new_bwr_size;
    }

    ret = lorsSetInit(&set, handle->lors_blocksize, 1, 0);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetInit failed", EIO);
    
    if (bufsize != userbuf_off) printf("CHECK THE PARAMETER\n");
    /* check the parameters for lorsSetStore */
    ret = lorsSetStore(set, handle->dp, packed_buf, start_off, packed_off, NULL, 
		       handle->lors_threads, handle->lors_timeout,
		       LORS_RETRY_UNTIL_TIMEOUT);
    if (ret != LORS_SUCCESS) {
      lorsSetFree(set, LORS_FREE_MAPPINGS);
      return ret;
    }
  }
  
  /* unlock the file region if we locked it */
  /*if ((fd->atomicity) && (fd->file_system != ADIO_PIOFS) && 
    (fd->file_system != ADIO_PVFS))
    {
    ADIOI_UNLOCK(fd, start_off, SEEK_SET, end_offset-start_off+1);
    }*/
  
  if (file_ptr_type == ADIO_INDIVIDUAL) fd->fp_ind = off;

  /* change handle->offset? <- this should follow fp_sys_posn? */  

  /* append trimmed "wholeset" back to handle->ex */
  ret = lorsAppendSet(handle->ex, wholeset);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsAppendSet failed", EIO);

  /* now do some mapping manipulation */
  ret = lorsSetEnum (set, &list);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetEnum failed", EIO);
  
  /* TODO: need to check the order of multiple blocks 
     -> first thing first? or last things first? */
  lm_array_size = 4;
  lm_array = (LorsMapping **)calloc(sizeof(LorsMapping *), lm_array_size);
  lm_array_cnt = 0;
  while (1) {
    ret = lorsEnumNext(list, &it, &lm);
    if (ret == LORS_END) break;

    lm_array[lm_array_cnt] = lm;
    lm_array_cnt++;
    if (lm_array_cnt == lm_array_size) {
      lm_array_size *= 2;
      lm_array = (LorsMapping **)realloc(lm_array, (sizeof(LorsMapping *) * 
						    lm_array_size));
    }
  }
  for (i = 0; i < lm_array_cnt - 1; i++) 
    for (j = i + 1; j < lm_array_cnt; j++) 
      if (lm_array[i]->exnode_offset > lm_array[j]->exnode_offset) {
	lm = lm_array[i];
	lm_array[i] = lm_array[j];
	lm_array[j] = lm;
      }

  i = 0;
  acc_blk_size = 0;
  /*  while (1) { 
    ret = lorsEnumNext(list, &it, &lm);
    if (ret == LORS_END) break;*/
  
  for (j = 0; j < lm_array_cnt; j++) {
    lm = lm_array[j];
    
    while (i < ncmap_cnt && 
	   ncmap[i].packed_offset + ncmap[i].length - 1 - acc_blk_size < 
	   lm->alloc_length) { 
      /*   printf("ncmap[%d/%d]: packed_offset %Ld length %Ld acc_blk_size %Ld, lm->alloc_length %d\n", i, ncmap_cnt, ncmap[i].packed_offset, ncmap[i].length, acc_blk_size, lm->alloc_length);*/
      new_lm = (LorsMapping *)calloc(sizeof(LorsMapping), 1);
      memcpy(new_lm, lm, sizeof(LorsMapping));
      new_lm->exnode_offset = ncmap[i].logical_offset;
      new_lm->logical_length = ncmap[i].length;
      new_lm->alloc_offset = ncmap[i].packed_offset - acc_blk_size;
      new_lm->alloc_length = ncmap[i].length;

      val.i = 1;
      if (!new_lm->md) exnodeCreateMetadata(&new_lm->md);
      exnodeSetMetadataValue(new_lm->md, "mapping_version", val, INTEGER, TRUE);
      ret = lorsAppendMapping(handle->ex, new_lm);
      LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsAppendMapping failed", EIO);

      i++;
    }

    if (i < ncmap_cnt) {
      if (ncmap[i].packed_offset - acc_blk_size < lm->alloc_length) {
	new_lm = (LorsMapping *)calloc(sizeof(LorsMapping), 1);
	memcpy(new_lm, lm, sizeof(LorsMapping));
	
	new_len = lm->alloc_length - ncmap[i].packed_offset;
	new_lm->exnode_offset = ncmap[i].logical_offset;
	new_lm->logical_length = new_len;
	new_lm->alloc_offset = ncmap[i].packed_offset - acc_blk_size;
	new_lm->alloc_length = new_len;

	val.i = 1;
	if (!new_lm->md) exnodeCreateMetadata(&new_lm->md);
	exnodeSetMetadataValue(new_lm->md, "mapping_version", val, INTEGER, TRUE);
	ret = lorsAppendMapping(handle->ex, new_lm);
	LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsAppendMapping failed", EIO);

	ncmap[i].logical_offset += new_len;
	ncmap[i].packed_offset += new_len;
	ncmap[i].length -= new_len;
      }

      acc_blk_size += lm->alloc_length;
    } 
  }
  
  ret = lorsEnumFree(list);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsEnumFree failed", EIO);
  
  fd->fp_sys_posn = -1;   /* mark it as invalid. */

  pmaps(handle);
  
  handle->offset = -1; /* necessary? */
  handle->exnode_modified = 1;

#ifdef HAVE_STATUS_SET_BYTES
  MPIR_Status_set_bytes(status, buftype, bufsize);
  /* This is a temporary way of filling in status. The right way is to 
   * keep track of how much data was actually written and placed in buf 
   */
#endif
  
  if (!buftype_is_contig) ADIOI_Delete_flattened(buftype);

  return 0;
}



/* _lorsCreateDepot is a static function and cannot be called from here 
   so I copied it */
int ADIOI_LNIO_lorsCreateDepot(LorsDepot **depot, int dpid, IBP_depot ibpdepot,
			       double bandwidth, double proximity)
{
  LorsDepot *dp = NULL;
  
  if ((dp = (LorsDepot *)calloc(1,sizeof(LorsDepot))) == NULL ){
    lorsDebugPrint(D_LORS_ERR_MSG,"_lorsCreateDepot: Out of memory\n");
    return (LORS_NO_MEMORY);
  };
  
  if ( (dp->lock = (pthread_mutex_t *)calloc(1,sizeof(pthread_mutex_t))) == NULL ){
    lorsDebugPrint(D_LORS_ERR_MSG,"_lorsCreateDepot: Out of memory\n");
    free(dp);
    return(LORS_NO_MEMORY);
  };
  if ( (dp->depot = (IBP_depot)calloc(1,sizeof(struct ibp_depot))) == NULL ){
    lorsDebugPrint(D_LORS_ERR_MSG,"_lorsCreateDepot: Out of memory\n");
    free(dp->lock);
    free(dp);
    return(LORS_NO_MEMORY);
  };
  pthread_mutex_init(dp->lock,NULL);
  memcpy(dp->depot ,ibpdepot,sizeof(struct ibp_depot));
  lorsTrim(dp->depot->host);
  dp->id = dpid;
  dp->bandwidth = bandwidth;
  dp->proximity = proximity;
  dp->nthread = 0;
  dp->nthread_done = 0;
  dp->nfailure = 0;
  
  *depot = dp;
  return (LORS_SUCCESS);
}



int ADIOI_LNIO_Flush(ADIO_File fd) 
{
  struct lnio_handle_t *handle = (struct lnio_handle_t *)fd->fs_ptr;
  int ret, rank, size, count, tempfd, length, i, *em_vector = NULL;
  char *buf = NULL, *buf2 = NULL;

  longlong mp_exnode_offset, mp_logical_length;

  LorsExnode *file_ex, *temp_ex;
  LorsSet *updates, *file_ex_set;
  LorsEnum list = NULL, it = NULL;
  LorsMapping *lm = NULL, *mp = NULL;
  LorsDepotPool *temp_dp;
  LorsDepot *ld;
  IBP_depot *depot_array = NULL;
  JRB node, tempnode;
  ExnodeValue val;
  ExnodeType type;

  MPI_Request request;
  MPI_Status status;

  MPI_Comm_rank(fd->comm, &rank);
  MPI_Comm_size(fd->comm, &size);

  em_vector = (int *)calloc(size, sizeof(int));


  /* flush the buffer if it is dirty */
  if (handle->dirty_buffer) 
    ADIOI_LNIO_Buffer_flush(fd, handle->buf_lb, handle->buf_ub - handle->buf_lb + 1);


  /* remove clean mappings (version 0) in my exnode*/
  jrb_traverse(node, handle->ex->mapping_map)
    {
      lm = node->val.v;
      if (lm->md)
	ret = exnodeGetMetadataValue(lm->md, "mapping_version", &val, &type);
      
      /* if "mapping_version" is not specified in the mapping, 
	 just assume it as a clean mapping */
      if (!lm->md || ret == EXNODE_NONEXISTANT || 
	  type == INTEGER && val.i == 0) {     
	tempnode = jrb_prev(node);
	jrb_delete_node(node);
	node = tempnode;
      } 
    }


  jrb_traverse(node, handle->ex->mapping_map)
    {
      lm = node->val.v;
      if (lm->md) {
	val.i = 0;
	ret = exnodeSetMetadataValue(lm->md, "mapping_version", val, INTEGER, TRUE);
	LNIO_ERROR_CHECK(ret != EXNODE_SUCCESS, ret, "exnodeSetMetadataValue failed", EIO);
      }
    }

  /* deserialize the exnode from the file */
  /* this is to sync the changes made to the file by others who open 
     the same file concurrently */
  /* the file also should contain (some of) clean mappings deleted earlier */
  ret = lorsExnodeCreate(&file_ex);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsExnodeCreate failed", EIO);
  
  tempfd = open(fd->filename, O_RDONLY);
  if (tempfd != -1) {
    close(tempfd);
    ret = lorsFileDeserialize(&file_ex, fd->filename, NULL);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsFileDeserialize failed", EIO);
  }

  /* put all the mappings in file_ex to a set */    
  ret = lorsSetInit(&file_ex_set, handle->lors_blocksize, 1, 0);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetInit failed", EIO);
  
  ret = lorsQuery(file_ex, &file_ex_set, 0, file_ex->logical_length, 
		  LORS_QUERY_REMOVE);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsQuery failed", EIO);
  
  /* trim from file_ex_set the overlapped extent between my dirty mappings 
     and the mappings contained in file_ex_set */
  /* (overwriting file_ex_set mappings with my dirty mappings) */

  jrb_traverse(node, handle->ex->mapping_map)
    {
      lm = node->val.v;
      mp_exnode_offset = lm->exnode_offset;
      mp_logical_length = lm->logical_length;      
      
      ret = lorsSetTrim(file_ex_set, mp_exnode_offset, mp_logical_length,
			handle->lors_threads, handle->lors_timeout, 
			LORS_RETRY_UNTIL_TIMEOUT | LORS_TRIM_ALL);	
      LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetTrim failed", EIO);     
    }
  
  /* check whose exnode has been modified since last sync/open*/
  MPI_Allgather((void *)&handle->exnode_modified, 1, MPI_INT, 
		(void *)em_vector, 1, MPI_INT, fd->comm);
  
  /* serialize my dirty mappings (mapping_version 1) */
  ret = lorsSerialize(handle->ex, &buf, 0, &length);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSerialize failed", EIO);

  /* send the serialized exnode to other procs */
  if (handle->exnode_modified) {
    for (i = 0; i < size; i++) {
      if (i == rank) continue;
      MPI_Isend((void *)buf, length, MPI_CHAR, i, rank, fd->comm, &request);
      printf("Isend to proc %d, msg length %d\n", i, length);
    }
  }
  
  count = 0;
  status.MPI_SOURCE = -1;

  /* receive dirty mappings from other procs */
  for (i = 0; i < size; i++) { 
    if (i == rank || !em_vector[i]) continue;
    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, fd->comm, &status);
    MPI_Get_count(&status, MPI_CHAR, &count);
    buf2 = (char *)calloc(count + 1, 1);
    if (!buf2) printf("buf2 is NULL\n");
    printf("There is a message from proc %d size %d\n", status.MPI_SOURCE, count);
    MPI_Recv(buf2, count, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, 
	     fd->comm, &status);

    /* add dirty mappings from other procs to my exnode */
    
    /* first deserialize the received buf*/
    ret = lorsExnodeCreate(&temp_ex);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsExnodeCreate failed", EIO);

    ret = lorsDeserialize(&temp_ex, buf2, count, NULL);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsDeserialize failed", EIO);

    /* then add each mapping in temp_ex to my exnode */
    /* TODO: how should we handle replicated mappings? */
    jrb_traverse(node, temp_ex->mapping_map)
      {
	lm = node->val.v;
	
	mp_exnode_offset = lm->exnode_offset;
	mp_logical_length = lm->logical_length;      
	
	/* before add it, trim the overlapped portion from file_ex_set */
	ret = lorsSetTrim(file_ex_set, mp_exnode_offset, mp_logical_length,
			  handle->lors_threads, handle->lors_timeout, 
			  LORS_RETRY_UNTIL_TIMEOUT | LORS_TRIM_ALL);	
	LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetTrim failed", EIO);       
	
	/* mark it as "clean" now */
	val.i = 0;
	ret = exnodeSetMetadataValue(lm->md, "mapping_version", val, INTEGER, TRUE);
	LNIO_ERROR_CHECK(ret != EXNODE_SUCCESS, ret, "exnodeSetMetadataValue failed", EIO);	  

	/* set the depotpool of each mapping to handle->dp */
	lm->dp = handle->dp;
	/* add the mapping to my exnode */
	ret = lorsAppendMapping(handle->ex, lm);
	LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsAppendMapping failed", EIO);
	
	/* add the depot used for the mapping to my depotpool */
	
	/*	printf("adding %s:%d from a new mapping to my depotpool.\n", lm->depot.host, lm->depot.port);*/
	ret = ADIOI_LNIO_lorsCreateDepot(&ld, 0, &(lm->depot), 0, 0);
	LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "ADIOI_LNIO_lorsCreateDepot failed", EIO);
	ret = lorsAddDepot(handle->dp->dl, ld);
	/* lorsAddDepot returns non-zero when the depot is already in the 
	   depotpool or is a bad depot */
      }

    /* if we call lorsExnodeFree, it will free the mappings which have been 
       added to my exnode. Just call free(temp->ex)? */

    if (buf2) {
      free(buf2);
      buf2 = NULL;
    }
  }
  
  
  /* finally add the mappings in the file_ex_set to my exnode */ 
  /* before calling lorsAppendSet, set each mapping's mapping_version to 0
     and make sure new depots are added to my depotpool */
  list = NULL; it = NULL; lm = NULL;
  
  ret = lorsSetEnum(file_ex_set, &list);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetEnum failed", EIO);

  while (1) {
    ret = lorsEnumNext(list, &it, &lm);
    if (ret == LORS_END) break;

    lm->dp = handle->dp;

    /*    printf("adding %s:%d from a new mapping in file_ex_set to my depotpool.\n", lm->depot.host, lm->depot.port);*/
    ret = ADIOI_LNIO_lorsCreateDepot(&ld, 0, &(lm->depot), 0, 0); 
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "ADIOI_LNIO_lorsCreateDepot", EIO);
    
    ret = lorsAddDepot(handle->dp->dl, ld);
    
    /* before appending, mark each mapping as "clean" */
    val.i = 0;
    ret = exnodeSetMetadataValue(lm->md, "mapping_version", val, INTEGER, TRUE);
    LNIO_ERROR_CHECK(ret != EXNODE_SUCCESS, ret, "exnodeSetMetadataValue failed", EIO);
  }
  
  ret = lorsEnumFree(list);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsEnumFree failed", EIO);
  
  ret = lorsAppendSet(handle->ex, file_ex_set);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsAppendSet failed", EIO); 
  
  if (!rank) { /* root writes exnodes to file */
    ret = lorsFileSerialize(handle->ex, fd->filename, 0, 0); 
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsFileSerialize failed", EIO);
  }
  
  /* now we have clean exnode */
  handle->exnode_modified = 0;
  
  /* safe to deallocate depot_array? */
   /*  if (buf) free(buf);*/
  if (em_vector) free(em_vector);

  return 0;
}



int ADIOI_LNIO_Ftruncate(ADIO_File fd, off_t size) 
{
  struct lnio_handle_t *handle = (struct lnio_handle_t *)fd->fs_ptr;
  int ret, rank, tempfd;
  LorsSet *set;
  LorsExnode *temp_ex;
  char c = 0;

  printf("ADIOI_LNIO_Ftruncate: size %Ld, ex->logical_length %Ld\n", size, handle->ex->logical_length);
  pmaps(handle);

  if (size == handle->ex->logical_length) return 0; /* do nothing */

  if (size > handle->ex->logical_length) {   
    /* to be safe, write a single byte at the last offset */
    ret = lorsSetInit(&set, handle->lors_blocksize, 1, 0);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetInit failed", EIO);

    ret = lorsSetStore(set, handle->dp, &c, size - 1, 1, NULL, 
		       handle->lors_threads, handle->lors_timeout,
		       LORS_RETRY_UNTIL_TIMEOUT);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetStore failed", EIO);

    /* is this necessary? */
    ret = ADIOI_LNIO_lorsSetMappingVersion(set, 1);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "ADIOI_LNIO_lorsSetMappingVersion failed", EIO);

  } else {
    /* first, root updates the mappings in the file */
    MPI_Comm_rank(fd->comm, &rank);
    if (!rank) {
      tempfd = open(fd->filename, O_RDONLY);
      if (tempfd != -1) {   /* file exists */
	close(tempfd);
	
	ret = lorsFileDeserialize(&temp_ex, fd->filename, NULL);
	LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsFileDeserialize failed", EIO);
	
	ret = lorsQuery(temp_ex, &set, size, handle->ex->logical_length - size,
			LORS_QUERY_REMOVE);
	LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsQuery failed", EIO);
	
	ret = lorsSetTrim(set, size, handle->ex->logical_length - size, 
			  handle->lors_threads, handle->lors_timeout, 
			  LORS_RETRY_UNTIL_TIMEOUT | LORS_TRIM_ALL);
	LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetTrim failed", EIO);
	
	ret = lorsAppendSet(temp_ex, set); 
	LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsAppendSet failed", EIO);
	
	ret = lorsFileSerialize(temp_ex, fd->filename, 0, 0);
	LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsFileSerialize failed", EIO);
	
	ret = lorsSetFree(set, LORS_FREE_MAPPINGS);
	LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetFree failed", EIO);

	ret = lorsExnodeFree(temp_ex);
	LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsExnodeFree failed", EIO);

      }
    }
    MPI_Barrier(fd->comm);  /* Ftruncate should be collective */
    
    
    /* then, update in-core mappings */

    ret = lorsQuery(handle->ex, &set, size, handle->ex->logical_length - size,
		  LORS_QUERY_REMOVE);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsQuery failed", EIO);
    
    ret = lorsSetTrim(set, size, handle->ex->logical_length - size, 
		      handle->lors_threads, handle->lors_timeout, 
		      LORS_RETRY_UNTIL_TIMEOUT | LORS_TRIM_ALL);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetTrim failed", EIO);
  }

  ret = lorsAppendSet(handle->ex, set); 
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsAppendSet failed", EIO);
  
  printf("ADIOI_LNIO_Ftruncate: requested size %Ld, ex->logical_length now %Ld\n", size, handle->ex->logical_length);
  
  /* there is a bug in lorsQuery(), and it doesn't update ex->logical_length
     after some mappings are removed. Until it is fixed, we manually set 
     logical_length here */
  handle->ex->logical_length = size;
  
  pmaps(handle);
  
  ret = lorsSetFree(set, 0);
  LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetFree failed", EIO);
  return 0;
}




/*******************************/
/*    Buffered I/O Routines    */
/*******************************/




int ADIOI_LNIO_Buffer_flush(ADIO_File fd, off_t flush_start_offset, size_t flush_size)
{
  struct lnio_handle_t *handle = (struct lnio_handle_t *)fd->fs_ptr;
  int ret;

  LorsSet *set, *newset;

  if (!handle) return -1;

  if (!handle->buffer || !handle->dirty_buffer) return 0; 
  
  ret = lorsQuery(handle->ex, &set, flush_start_offset, 
		  flush_size, LORS_QUERY_REMOVE);
  if (ret != LORS_SUCCESS) return ret;

  if (jrb_empty(set->mapping_map)) {
    ret = lorsSetInit(&set, handle->lors_blocksize, 1, 0);
    if (ret != LORS_SUCCESS) return ret;
    
    ret = lorsSetStore(set, handle->dp, 
		       handle->buffer + (flush_start_offset - handle->buf_lb), 
		       flush_start_offset, flush_size,
		       NULL, handle->lors_threads, handle->lors_timeout,
		       LORS_RETRY_UNTIL_TIMEOUT);
    if (ret != LORS_SUCCESS) {
      lorsSetFree(set, LORS_FREE_MAPPINGS);
      return ret;
    }

    /* set the version of each new mapping to 1 */
    ret = ADIOI_LNIO_lorsSetMappingVersion(set, 1);
    if (ret != LORS_SUCCESS) {
      return -1;
    }
  } else {
    set->copies = handle->lors_copies;
    set->data_blocksize= handle->lors_blocksize; 
       
    /* basically repeating what lorsSetUpdate does */

    ret = lorsSetInit(&newset, handle->lors_blocksize, 1, 0);
    if (ret != LORS_SUCCESS) {
      errno = EIO;
      return -1;
    }
    
    ret = lorsSetStore(newset, handle->dp, 
		       handle->buffer + (flush_start_offset - handle->buf_lb), 
		       flush_start_offset, flush_size,
		       NULL, 
		       handle->lors_threads, 
		       handle->lors_timeout, 
		       LORS_RETRY_UNTIL_TIMEOUT);
  
    if (ret != LORS_SUCCESS) {
      lorsSetFree(newset, LORS_FREE_MAPPINGS);
      errno = EIO;
      return -1;
    }

    /* set the version of each new mapping to 1 
       (touched since the last sync) */
    ret = ADIOI_LNIO_lorsSetMappingVersion(newset, 1);
    if (ret != LORS_SUCCESS) {
      lorsSetFree(newset, LORS_FREE_MAPPINGS);
      errno = EIO;
      return -1;
    }

    ret = lorsSetTrim(set, flush_start_offset, flush_size,
		      handle->lors_threads,
		      handle->lors_timeout, LORS_RETRY_UNTIL_TIMEOUT | 
		      LORS_TRIM_ALL);
    if (ret != LORS_SUCCESS) {
      lorsSetFree(newset, LORS_FREE_MAPPINGS);
      errno = EIO;
      return -1;
    }
    
    ret = lorsSetMerge(newset, set);
    if (ret != LORS_SUCCESS) {
      lorsSetFree(newset, LORS_FREE_MAPPINGS);
      errno = EIO;
      return -1;
    }
    
    lorsSetFree(newset, 0);
  }
  
  ret = lorsAppendSet(handle->ex, set);
  if (ret != LORS_SUCCESS) {
    lorsSetFree(set, LORS_FREE_MAPPINGS);
    errno = EIO;
    return -1;
  }
  
  lorsSetFree(set, 0);
  
  handle->exnode_modified = 1;
  handle->dirty_buffer = 0;
  
  return 0;
}


/***********************************************************/
/*                 OLD CODE                                */
/***********************************************************/

int ADIOI_LNIO_Flush_old(ADIO_File fd) 
{
  struct lnio_handle_t *handle = (struct lnio_handle_t *)fd->fs_ptr;
  int ret, rank, size, count, tempfd, length, i, *em_vector, modified;
  char *buf;

  LorsExnode *new_ex, *temp_ex;
  LorsSet *updates, *new_ex_set;
  LorsEnum list = NULL, it = NULL;
  LorsMapping *mp = NULL;
  LorsDepotPool *temp_dp;
  LorsDepot *ld;
  IBP_depot *depot_array = NULL;

  MPI_Request request;
  MPI_Status status;


  MPI_Comm_rank(fd->comm, &rank);
  MPI_Comm_size(fd->comm, &size);

  /*  printf("handle->dirty_buffer: %d\n", handle->dirty_buffer);*/
  /* flush the buffer if it is dirty */
  if (handle->dirty_buffer) 
    ADIOI_LNIO_Buffer_flush(fd, handle->buf_lb, handle->buf_ub - handle->buf_lb + 1);
  
  em_vector = (int *)calloc(size, sizeof(int));

  printf("ADIOI_LNIO_Flush: entering\n");

  if (!rank) {  /* master deserializes exnode from the file */
    /* first, create an exnode structure */
    ret = lorsExnodeCreate(&new_ex);
    if (ret != LORS_SUCCESS) {
      errno = EIO;
      return -1;
    }
    
    /* check if the file already exists */
    tempfd = open(fd->filename, O_RDONLY);
    if (tempfd != -1) {
      close(tempfd);
      /* if the file exists, deserialize it */
      ret = lorsFileDeserialize(&new_ex, fd->filename, NULL);
      if (ret != LORS_SUCCESS) {
	errno = EIO;
	return -1;
      }
    }

    MPI_Gather((void *)&handle->exnode_modified, 1, MPI_INT, em_vector, 
	       1, MPI_INT, 0, fd->comm);
    
    modified = 0;
    for (i = 0; i < size; i++) 
      if (em_vector[i]) {
	modified = 1;
	break;
      }

    if (modified) printf("***** exnode modified since the last sync\n");
    else printf("***** exnode not modified since the last sync\n");

    /* add updated mappings from master's exnode to updates */
    
    if (modified) {
      ret = lorsSetInit(&updates, handle->lors_blocksize, 1, 0);
      if(ret != LORS_SUCCESS) {
	errno = EIO;
	return -1;
      }
      
      if (handle->exnode_modified) 
	ADIOI_LNIO_lorsSetAddUpdatedMappings(handle, updates, handle->ex);
    }  
  } else {
    MPI_Gather((void *)&handle->exnode_modified, 1, MPI_INT, em_vector, 
	       1, MPI_INT, 0, fd->comm);
  }

  printf("handle->exnode_modified %d!\n", handle->exnode_modified);

  if (rank && handle->exnode_modified) {  /* non-master */
    /* non-master procs serialize their exnodes and send it to master */
    ret = lorsSerialize(handle->ex, &buf, 0, &length);
    if (ret != LORS_SUCCESS) {
      errno = EIO;
      return -1;
    }
    MPI_Isend((void *)buf, length, MPI_CHAR, 0, rank, fd->comm, &request);
    free(buf);
  } else if (!rank && modified) {  /* master */
    /* master gathers exnodes from non-master procs */
    for (i = 1; i < size; i++) {  /* there are size - 1 senders */
      if (!em_vector[i]) continue;
      MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, fd->comm, &status);
      MPI_Get_count(&status, MPI_CHAR, &count);
      buf = (char *)calloc(count + 1, 1);
      printf("ADIOI_LNIO_Flush: there's a message from proc %d, tag %d, size %d\n", status.MPI_SOURCE, status.MPI_TAG, count);
      MPI_Recv(buf, count, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, 
	       fd->comm, &status);
      
      ret = lorsExnodeCreate(&temp_ex);
      if (ret != LORS_SUCCESS) {
	errno = EIO;
	return -1;
      }
      
      ret = lorsDeserialize(&temp_ex, buf, count, NULL);
      if (ret != LORS_SUCCESS) {
	errno = EIO;
	return -1;
      }

      ret = lorsUpdateDepotPool(temp_ex, &temp_dp, handle->lbone_server, 
				handle->lbone_port, 
				handle->lbone_location, 
				handle->lors_threads, 
				handle->lors_timeout, 0);
      if (ret != LORS_SUCCESS) return ret;     

      handle->dp->duration = handle->lors_duration;
      /*      printf("### handle->dp->duration : %d\n", handle->dp->duration);*/

      /* add updated mappings from received exnode to updates */
      printf("ADIOI_LNIO_Flush: calling ADIOI_LNIO_lorsAddUpdatedMappings for proc %d\n", i);
      ADIOI_LNIO_lorsSetAddUpdatedMappings(handle, updates, temp_ex);


      lorsFreeDepotPool(temp_dp);
      lorsExnodeFree(temp_ex);
      free(buf);
    }
    
    /* now master combine the mappings in updates with new_ex */
    /* first, creates a set of mappings from new_ex */
    
    ret = lorsSetInit(&new_ex_set, handle->lors_blocksize, 1, 0);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetInit failed", EIO);
    
    ret = lorsQuery(new_ex, &new_ex_set, 0, new_ex->logical_length, 
		    LORS_QUERY_REMOVE);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsQuery failed", EIO);
    
    /* then trim each mapping in updates from new_ex_set 
       and add it back to new_ex_set */
    list = NULL; it = NULL; mp = NULL;
    
    ret = lorsSetEnum(updates, &list);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetEnum failed", EIO);
    
    while (1) {
      ret = lorsEnumNext(list, &it, &mp);
      if (ret == LORS_END) break;
      /*      printf("there is a mapping in updates, offset %Ld, length %d, dp is %s NULL\n", mp->exnode_offset, mp->logical_length, mp->dp ? "NOT" : "");*/
      ret = lorsSetTrim(new_ex_set, mp->exnode_offset, mp->logical_length,
			handle->lors_threads, handle->lors_timeout, 
			LORS_RETRY_UNTIL_TIMEOUT | LORS_TRIM_ALL);	
      LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetTrim failed", EIO);
    }
    
    lorsEnumFree(list);
    
    ret = lorsSetMerge(updates, new_ex_set);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSetMerge failed", EIO);

    ret = lorsAppendSet(new_ex, new_ex_set);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsAppendSet failed", EIO);

  }
  
  /* now, master serializes new_ex, broadcasts to other procs, and 
     writes to the xml file 
     non-masters receive new_ex and serialize it 
  */

  printf("Checkpoint 1\n");

  if (!rank) { /* master */
  /* assume that master is one of the aggregators or has an access to 
     the file system */

    ret = lorsFileSerialize(new_ex, fd->filename, 0, 0); 
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsFileSerialize failed", EIO);
    
    ret = lorsSerialize(new_ex, &buf, 0, &length);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsSerialize failed", EIO);
  
    /* broadcast the serialized exnode to everybody */
    MPI_Bcast((void *)&length, 1, MPI_INT, 0, fd->comm);
    MPI_Bcast((void *)buf, length, MPI_CHAR, 0, fd->comm);

    /* extract depot info from current exnode before destroying it */
    /* (For WRONLY or RDWR, current exnode contains depots that should 
       be used for future writes, which were obtained by lorsGetDepotPool. 
       So we have to add them into newly created exnode */
    ADIOI_LNIO_lorsExtractDepotList(handle->ex, &depot_array, 0);

    lorsExnodeFree(handle->ex);
    handle->ex = new_ex;
  } else { /* non-master */
    printf("here 0\n");
    MPI_Bcast((void *)&length, 1, MPI_INT, 0, fd->comm);
    buf = (char *)calloc(length + 1, 1);
    printf("here 1\n");
    MPI_Bcast((void *)buf, length, MPI_CHAR, 0, fd->comm);

    printf("here 2\n");
    /* extract depot info from current exnode (see above) */
    ADIOI_LNIO_lorsExtractDepotList(handle->ex, &depot_array, 0);
    
    printf("here 3\n");
    ret = lorsExnodeFree(handle->ex);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsExnodeFree failed", EIO);

    printf("here 4\n");
    ret = lorsExnodeCreate(&handle->ex);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsExnodeCreate failed", EIO);

    printf("here 5\n");
    ret = lorsDeserialize(&handle->ex, buf, length, NULL);
    LNIO_ERROR_CHECK(ret != LORS_SUCCESS, ret, "lorsDeserialize failed", EIO);
  }

  /* if the new exnode has some mappings, create a new depot pool from it*/
  /* (this will update the dp field of each mapping in the exnode, too) */
  /* otherwise (like sync is called before any writes), keep the old depot */

  printf("Checkpoint 2\n");

  if (!jrb_empty(handle->ex->mapping_map)) {
    ret = lorsFreeDepotPool(handle->dp);
    if (ret != LORS_SUCCESS) return ret;
    
    ret = lorsUpdateDepotPool(handle->ex, &handle->dp, 
			      handle->lbone_server, 
			      handle->lbone_port, 
			      handle->lbone_location, 
			      handle->lors_threads, 
			      handle->lors_timeout, 0);
    /*    printf("lorsUpdateDepotPool returned %d\n", ret);*/
    if (ret != LORS_SUCCESS) return ret;
    
    handle->dp->duration = handle->lors_duration;
    
    
    /* add depots in depot_array into handle->dp */
    if (depot_array) {
      for (i = 0; depot_array[i] != NULL; i++) {
	
	/* below equivalent to _lorsCreateDepot(&ld, 0, depot_array[i], 0, 0); */
	/* _lorsCreateDepot cannot be called because it is a static function */
	/*****************/
	ld = NULL;
	
	if ((ld = (LorsDepot *)calloc(1,sizeof(LorsDepot))) == NULL ){
	  lorsDebugPrint(D_LORS_ERR_MSG,"_lorsCreateDepot: Out of memory\n");
	  return LORS_NO_MEMORY;
	};
	
	if ( (ld->lock = (pthread_mutex_t *)calloc(1,sizeof(pthread_mutex_t))) == NULL ){
	  lorsDebugPrint(D_LORS_ERR_MSG,"_lorsCreateDepot: Out of memory\n");
	  free(ld);
	  return LORS_NO_MEMORY;
	};
	if ( (ld->depot = (IBP_depot)calloc(1,sizeof(struct ibp_depot))) == NULL) {
	  lorsDebugPrint(D_LORS_ERR_MSG,"_lorsCreateDepot: Out of memory\n");
	  free(ld->lock);
	  free(ld);
	  return LORS_NO_MEMORY ;
	};
	pthread_mutex_init(ld->lock, NULL);
	memcpy(ld->depot, depot_array[i], sizeof(struct ibp_depot));
	lorsTrim(ld->depot->host);
	ld->id = 0;
	ld->bandwidth = 0.0;
	ld->proximity = 0.0;
	ld->nthread = 0;
	ld->nthread_done = 0;
	ld->nfailure = 0;
	/******************/
	
	ret = lorsAddDepot(handle->dp->dl, ld);
      }
    }
  }

  printf("ADIOI_LNIO_Flush: exiting\n");

  handle->exnode_modified = 0;

  /* safe to deallocate depot_array? */
  if (buf) free(buf);
  if (em_vector) free(em_vector);

  return 0;
}


int ADIOI_LNIO_lorsSetAddUpdatedMappings(struct lnio_handle_t *handle, 
					 LorsSet *updates, LorsExnode *ex)
{
  LorsSet *set;
  LorsEnum list = NULL, it = NULL;
  LorsMapping *mp = NULL;
  ExnodeValue val;
  ExnodeType type;
  int ret;

  ret = lorsSetInit(&set, handle->lors_blocksize, 1, 0);
  if (ret != LORS_SUCCESS) return ret;

  ret = lorsQuery(ex, &set, 0, ex->logical_length, LORS_QUERY_REMOVE);
  if (ret != LORS_SUCCESS) return ret;
  
  ret = lorsSetEnum(set, &list);
  while (1) {
    ret = lorsEnumNext(list, &it, &mp);
    if (ret == LORS_END) break;
    if (mp->md) {
      exnodeGetMetadataValue(mp->md, "mapping_version", &val, &type);
      if (type == INTEGER && val.i == 1) {
	/*	printf("ADIOI_LNIO_lorsSetAddUpdateMappings: there is a mapping added to updates, offset %Ld, length %d, dp is %s NULL\n", mp->exnode_offset, mp->logical_length, mp->dp ? "NOT" : "");*/
	val.i = 0;
	exnodeSetMetadataValue(mp->md, "mapping_version", val, INTEGER, TRUE);
      } else { /* remove mapping from the set - version # is 0 (intact) */ 
	ret = lorsSetRemoveMapping(set, mp);
	if (ret != LORS_SUCCESS) return ret;
      }
    } else { /* remove mapping from the set - no version # specified*/
      ret = lorsSetRemoveMapping(set, mp);
      if (ret != LORS_SUCCESS) return ret;
    } 
  }
  
  lorsEnumFree(list);

  ret = lorsSetMerge(set, updates);
  if (ret != LORS_SUCCESS) return ret;

  return LORS_SUCCESS;  
}



int ADIOI_LNIO_lorsSerialize_mm(LorsExnode *exnode, char **buffer, 
				int *length) 
{
  /* only serialize the mappings with mapping_version 1 */
  /* (modified mappings since the last sync or file open) */
  
  char *buf;
  int len;
  int     ret;
  JRB     tree, node;
  LorsMapping    *lm;
  ExnodeMapping  *em;
  ExnodeMetadata *emd;
  ExnodeValue     val;
  ExnodeType      type;
  
  if ( exnode->exnode != NULL ) {
    fprintf(stderr,"serialize: not null\n");
  };
  
  exnodeCreateExnode(&(exnode->exnode));
  
  jrb_traverse(node, exnode->mapping_map)
    {
      lm = node->val.v;
      
      if (lm->md) {
	exnodeGetMetadataValue(lm->md, "mapping_version", &val, &type);
	if (type == INTEGER && val.i == 1) {      
	  exnodeCreateMapping(&em);
	  exnodeGetMappingMetadata(em, &emd);
	  val.i = lm->exnode_offset;
	  exnodeSetMetadataValue(emd, "exnode_offset", val, INTEGER, TRUE);
	  val.i = lm->logical_length;
	  exnodeSetMetadataValue(emd, "logical_length", val, INTEGER, TRUE);
	  val.i = lm->alloc_length;
	  exnodeSetMetadataValue(emd, "alloc_length", val, INTEGER, TRUE);
	  val.i = lm->alloc_offset;
	  exnodeSetMetadataValue(emd, "alloc_offset", val, INTEGER, TRUE);
	  val.i = 1; 
	  exnodeSetMetadataValue(emd, "mapping_version", val, INTEGER, TRUE);
	  val.i = lm->e2e_bs;
	  /*fprintf(stderr, "e2e_bs: %d\n", lm->e2e_bs);*/
	  exnodeSetMetadataValue(emd, "e2e_blocksize", val, INTEGER, TRUE);
	  
	  exnodeSetCapabilities(em, lm->capset.readCap, 
				lm->capset.writeCap, 
				lm->capset.manageCap, TRUE);
	  /* copy mapping metadata into the exnode-native md */
	  if ( lm->md != NULL )
	    {
	      lorsMetadataMerge(lm->md, emd);
	    }
	  
	  /*fprintf(stderr, "Serializing Fxn : 0x%x\n", lm->function);*/
	  exnodeSetFunction(em, lm->function, TRUE);
	}
      }
      /*fprintf(stderr, "\tAppending Mapping:0x%x\n", em);*/
      exnodeAppendMapping(exnode->exnode, em); 
    }
  
    exnodeGetExnodeMetadata(exnode->exnode, &emd);

    lorsGetLibraryVersion(NULL, &val.d);
    if ( exnodeGetMetadataValue(emd, "lorsversion", &val, &type) != EXNODE_SUCCESS )
    {
#ifdef _MINGW
        exnodeSetMetadataValue(emd, "lorsversion", val, DOUBLET, TRUE);
#else
        exnodeSetMetadataValue(emd, "lorsversion", val, DOUBLE, TRUE);
#endif
    }

    if ( exnode->md != NULL )
    {
        lorsMetadataMerge(exnode->md, emd);
    }

    ret = exnodeSerialize(exnode->exnode, &buf,  &len);
    if ( ret != EXNODE_SUCCESS )
    {
        return LORS_FAILURE;
    }

    /*fprintf(stderr, "DESTROYING THE EXNODE IN SERIALIZE ______\n");*/
    exnodeDestroyExnode(exnode->exnode);
    exnode->exnode = NULL;
    *buffer = buf;
    *length = len;

    return LORS_SUCCESS;
}
