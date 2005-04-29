#include "ad_ln_lnio.h"
#include "adio.h"

void ADIOI_LNIO_Free_handle(struct lnio_handle_t *handle) {
  if (!handle->lbone_server) free(handle->lbone_server);
  if (!handle->lbone_location) free(handle->lbone_location);
  if (!handle) free(handle);
}

int ADIOI_LNIO_Open(ADIO_File fd) 
{
  int perm, flag, file_exists, tempfd, ret;
  struct lnio_handle_t *handle;
  char *value;

  int i;
  IBP_depot *depot_array = NULL;
  LorsSet *set;
   
#ifdef JLEE_DEBUG
  printf("ADIOI_LNIO_Open: entering the function for %s\n", fd->filename);
#endif 

  handle = (struct lnio_handle_t *)calloc(1, sizeof(struct lnio_handle_t));
  if (!handle) return -1;
  
  if (fd->info == MPI_INFO_NULL) printf("fd->info == MPI_INFO_NULL\n");

  /* environment setup */
  value = (char *) ADIOI_Malloc((MPI_MAX_INFO_VAL + 1) * sizeof(char));
  MPI_Info_get(fd->info, "LBONE_SERVER", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    handle->lbone_server = strdup(value);
  } else {
    handle->lbone_server = strdup(LBONE_SERVER);
  }
    
  MPI_Info_get(fd->info, "LBONE_PORT", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    handle->lbone_port = atoi(value);
  } else {
    handle->lbone_port = LBONE_PORT;
  }
  
  MPI_Info_get(fd->info, "LBONE_LOCATION", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    handle->lbone_location = strdup(value);
  } else {
    handle->lbone_location = strdup(LBONE_LOCATION);
  }
  
  MPI_Info_get(fd->info, "LORS_BLOCKSIZE", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    handle->lors_blocksize = atoi(value) * MEGA;
  } else {
    if(fd->access_mode & ADIO_WRONLY || fd->access_mode & ADIO_CREATE)
      handle->lors_blocksize = LORS_BLOCKSIZE * MEGA;
    else
      handle->lors_blocksize = MEGA;
  }
  
  MPI_Info_get(fd->info, "LORS_DURATION", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    handle->lors_duration = atoi(value);
  } else {
    handle->lors_duration = LORS_DURATION;
  }
  
  MPI_Info_get(fd->info, "LORS_COPIES", MPI_MAX_INFO_VAL, value, &flag);
  if(flag) {
    handle->lors_copies = atoi(value);
  } else {
    handle->lors_copies = LORS_COPIES;
  }
  
  MPI_Info_get(fd->info, "LORS_THREADS", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    handle->lors_threads = atoi(value);
  } else {
    handle->lors_threads = LORS_THREADS;
  }
  
  MPI_Info_get(fd->info, "LORS_TIMEOUT", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    handle->lors_timeout = atoi(value);
  } else {
    handle->lors_timeout = LORS_TIMEOUT;
  }
  
  MPI_Info_get(fd->info, "LORS_SERVERS", MPI_MAX_INFO_VAL, value, &flag);
  if(flag) {
    handle->lors_servers = atoi(value);
  } else {
    handle->lors_servers = LORS_SERVERS;
  }
  
  MPI_Info_get(fd->info, "LORS_SIZE", MPI_MAX_INFO_VAL, value, &flag);
  if(flag) {
    handle->lors_size = atoi(value) * MEGA;
  } else {
    handle->lors_size = LORS_SIZE * MEGA;
  }
  
  MPI_Info_get(fd->info, "LORS_DEMO", MPI_MAX_INFO_VAL, value, &flag);
  if (flag) {
    g_lors_demo = 1;
  } 
  
  MPI_Info_get(fd->info, "LORS_IO_BUFFER_SIZE", MPI_MAX_INFO_VAL, value, &flag);
  if(flag) {
    handle->buffer_size = atoi(value);
    printf("handle->buffer_size %d\n", handle->buffer_size);
    handle->buffer = (char *)calloc(handle->buffer_size, 1);
    if (!handle->buffer) return -1; /* not enough memory */
  } else {
    handle->buffer_size = 0;
    handle->buffer = NULL;
  }
 
  free(value);
  
#ifdef JLEE_DEBUG
  printf("ADIOI_LNIO_Open: finished environment setup\n");
  printf("ADIOI_LNIO_Open: lbone_server %s lbone_port %d\n", handle->lbone_server, handle->lbone_port);
  printf("ADIOI_LNIO_Open: lbone_location %s lors_blocksize %d\n", handle->lbone_location, handle->lors_blocksize);
  printf("ADIOI_LNIO_Open: lors_duration %d lors_copies %d\n", handle->lors_duration, handle->lors_copies);
  printf("ADIOI_LNIO_Open: lors_threads %d lors_timeout %d\n", handle->lors_threads, handle->lors_timeout);
  printf("ADIOI_LNIO_Open: lors_servers %d lors_size %d\n", handle->lors_servers, handle->lors_size);
#endif 

  if (!handle->lbone_server && !(fd->access_mode & ADIO_RDONLY || 
				 !fd->access_mode || 
				 fd->access_mode & ADIO_RDWR)) {
    errno = EINVAL;
    return -1;
  }  /* Why do we need this? */


  handle->offset = 0;
  handle->exnode_modified = 0;
  
  /*  for buffered I/O */
  handle->buf_lb = 0;
  handle->buf_ub = -1;  /* if buf_lb > buf_ub, buf is empty */  
  handle->dirty_buffer = 0;

  /*
      handle->buffer = malloc(handle->lors_blocksize * handle->lors_threads);
      if (!handle->buffer) {
      return -1;
      }
  */
  
  /* check if the file exists */
  tempfd = open(fd->filename, O_RDONLY);
  if (tempfd == -1) file_exists = 0; 
  else {
    file_exists = 1;
    close(tempfd);
  }
  
#ifdef JLEE_DEBUG
  printf("ADIOI_LNIO_Open: specified file %s\n", file_exists ? "exists" : "doesn't exist");
#endif 

  if (!file_exists) {  /* file doesn't exist */
    if (!(fd->access_mode & ADIO_CREATE)) {
      errno = ENOENT;
      return -1;
    }
    
    /*      
	    fd = open(handle->filename, amode, perm);
	    if(fd == -1) {
	    ADIO_LN_Free_handle(handle);
	    errno = EIO;
	    return NULL;
	    }
	    close(fd);
    */
    
#ifdef JLEE_DEBUG
    printf("ADIOI_LNIO_Open: Calling lorsGetDepotPool\n");
#endif 
    
    depot_array = (IBP_depot *)calloc(2, sizeof(IBP_depot));
    for (i = 0; i < 1; i++)
      depot_array[i] = (IBP_depot)calloc(1, sizeof(struct ibp_depot));
    depot_array[1] = NULL; 
    /* need to put NULL at the end - or not needed because we did calloc?*/

    strcpy(depot_array[0]->host, "ibp.accre.vanderbilt.edu");
    depot_array[0]->port = 6715;
   
    ret = lorsGetDepotPool(&handle->dp, /*handle->lbone_server, 
					  handle->lbone_port, NULL,*/
			   NULL, 0, depot_array,
			   handle->lors_servers,
			   handle->lbone_location, 
			   handle->lors_size / MEGA + 1,
			   /* shouldn't this be divided by lors_servers? */
			   IBP_SOFT, 
			   handle->lors_duration, 
			   handle->lors_threads,
			   handle->lors_timeout, 
			   LORS_CHECKDEPOTS);
    if (ret != LORS_SUCCESS) {
      ADIOI_LNIO_Free_handle(handle);
      errno = EIO;
      return -1;
    }
    /* TODO: will probably reorder depots in the depot pool */
    
#ifdef JLEE_DEBUG
    printf("ADIOI_LNIO_Open: Calling lorsExnodeCreate\n");
#endif 

    ret = lorsExnodeCreate(&handle->ex);
    if (ret != LORS_SUCCESS) {
      ADIOI_LNIO_Free_handle(handle);
      errno = EIO;
      return -1;
    }
  } else { /* file exists */
    if (fd->access_mode & ADIO_EXCL) {
      ADIOI_LNIO_Free_handle(handle);
      errno = EEXIST;
      return -1;
    } 
    
    printf("ADIOI_LNIO_Open: calling lorsFileDeserialize\n");
    ret = lorsFileDeserialize(&handle->ex, fd->filename, NULL);
    if (ret != LORS_SUCCESS) {
      ADIOI_LNIO_Free_handle(handle);	
      errno = EIO;
      return -1;
    } 
    printf("ADIOI_LNIO_Open: successfully called lorsFileDeserialize\n");
    
    /*LorsEnum list = NULL, it = NULL;
    LorsMapping *mp = NULL;
    
    lorsExnodeEnum (handle->ex, &list);
    lorsEnumNext(list, &it, &mp);
    lorsEnumFree(list);
    */
    /* do we really need this part? */
    /* if(mp->e2e_bs && handle->lors_blocksize % mp->e2e_bs) {
       int size = (1 + handle->lors_blocksize / mp->e2e_bs)
       * mp->e2e_bs;*/
    /*    free(handle->buffer);
	  handle->buffer = (char *)malloc(size * 4);*/
    /*  handle->lors_blocksize = size;
	}*/
    
    /*  Buffered I/O Stuff
	handle->fill_buffer = 1; */
 
    ret = lorsQuery(handle->ex, &set, 0, handle->ex->logical_length, 0);
    if (ret != LORS_SUCCESS) return ret;
    if (jrb_empty(set->mapping_map)) {
      /* exnode file exists but it doesn't contain mappings */
      int i;
      IBP_depot *depot_array = NULL;
      depot_array = (IBP_depot *)calloc(2, sizeof(IBP_depot));
      for (i = 0; i < 1; i++)
	depot_array[i] = (IBP_depot)calloc(1, sizeof(struct ibp_depot));
      depot_array[1] = NULL; 
      /* need to put NULL at the end - or not needed because we did calloc?*/
      
      strcpy(depot_array[0]->host, "ibp.accre.vanderbilt.edu");
      depot_array[0]->port = 6715;
      
      ret = lorsGetDepotPool(&handle->dp, /*handle->lbone_server, 
					    handle->lbone_port, NULL,*/
			     NULL, 0, depot_array,
			     handle->lors_servers,
			     handle->lbone_location, 
			     handle->lors_size / MEGA + 1,
			     /* shouldn't this be divided by lors_servers? */
			     IBP_SOFT, 
			     handle->lors_duration, 
			     handle->lors_threads,
			     handle->lors_timeout, 
			     LORS_CHECKDEPOTS);
      if (ret != LORS_SUCCESS) {
	ADIOI_LNIO_Free_handle(handle);
	errno = EIO;
	return -1;
      }
    } else { /* exnode contains mappings */
      printf("ADIOI_LNIO_Open: calling lorsUpdateDepotPool\n");
      
      ret = lorsUpdateDepotPool(handle->ex, &handle->dp, 
				handle->lbone_server, 0, 
				NULL, handle->lors_threads, 
				handle->lors_timeout, 0);
      
      printf("lorsUpdateDepotPool returned %d\n", ret);
      if(ret != LORS_SUCCESS) {
	ADIOI_LNIO_Free_handle(handle);
	errno = EIO;
	return -1;
      }
      
      handle->dp->duration = handle->lors_duration;
    }
#ifdef JLEE_DEBUG
    printf("ADIOI_LNIO_Open: successfully called lorsUpdateDepotPool\n");
#endif
  }
  
  fd->fs_ptr = (void *)handle;
  
  return 0;
}

int ADIOI_LNIO_Close(ADIO_File fd)
{
  int ret;
  struct lnio_handle_t *handle = (struct lnio_handle_t *)fd->fs_ptr;
  
  if (!handle) return -1;
  
  if (!(fd->access_mode & ADIO_RDONLY)) {
    ret = ADIOI_LNIO_Flush(fd); 
    if (ret == -1) {
      return -1;
    }
  }  
  
  lorsExnodeFree(handle->ex);
  if (handle->dp) lorsFreeDepotPool(handle->dp);
  
  /* handle DELETE_ON_CLOSE case - processed in ADIO_Close? */
  
  ADIOI_LNIO_Free_handle(handle);

  return 0;
}


off_t ADIOI_LNIO_Lseek(ADIO_File fd, off_t offset, int whence) 
{
  struct lnio_handle_t *handle = (struct lnio_handle_t *)fd->fs_ptr;

  if (!handle) return -1;

#ifdef JLEE_DEBUG  
  printf("ADIOI_LNIO_Lseek: whence: %s, offset: %d\n", whence == SEEK_SET ? "SEEK_SET" : "not SEEK_SET", offset);
#endif

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
    errno = EINVAL;
    return -1;
  }

  return handle->offset;
}

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
  
  if (handle->buffer_size > 0) 
    return ADIOI_LNIO_Buffered_read(fd, buf, count);
  
  printf("ADIOI_LNIO_Read: entering with offset %Ld and count %d\n", handle->offset, count);

  if (count == 0) return 0;

  if (!handle || !buf) return -1;

  if(fd->access_mode & ADIO_WRONLY) {
    errno = EINVAL;
    return -1;
  }
  
  if (handle->offset < 0 || handle->offset >= handle->ex->logical_length) {
    return 0;
  }
  
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
    printf("I'm here -1\n");
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
    printf("I'm here 1\n");
    lorsSetFree(prev_ptr->set, 0);
    free(prev_ptr);
    printf("I'm here 2\n");
  }
    
  lorsSetFree(set, 0); 

  handle->offset += count;

  printf("READ: exiting\n");

  return count;
}


int ADIOI_LNIO_lorsSetMappingVersion(LorsSet *set, int version) {
  LorsEnum list = NULL, it = NULL;
  LorsMapping *mp = NULL;
  ExnodeValue val;
  int ret;

  ret = lorsSetEnum (set, &list);
  if (ret != LORS_SUCCESS) {
    return ret;
  }
  while (1) { /* set the mapping version of each mapping */
    ret = lorsEnumNext(list, &it, &mp);
    if (ret == LORS_END) break;
    val.i = version;
    if (!mp->md) exnodeCreateMetadata(&mp->md);
    /* lorsGetMappingMetadata doesn't call exnodeCreateMetadata 
       when md is NULL. Should we report this? */
    exnodeSetMetadataValue(mp->md, "mapping_version", val, INTEGER, TRUE);
  } 
  lorsEnumFree(list);
  
  return LORS_SUCCESS;
}

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
  
  if (handle->buffer_size > 0) 
    return ADIOI_LNIO_Buffered_write(fd, buf, count);

  if (!handle || !buf) return -1;

  if (count == 0) return 0;

  if (fd->access_mode == ADIO_RDONLY) {
    errno = EINVAL;
    return -1;
  }

#ifdef JLEE_DEBUG  
  printf("ADIO_LNIO_Write: count %d, offset %d\n", count, handle->offset);
#endif

  /*  if(count > handle->lors_blocksize * handle->lors_threads) {
    count = handle->lors_blocksize * handle->lors_threads;
    }*/
  
  ret = lorsQuery(handle->ex, &set, handle->offset, count, LORS_QUERY_REMOVE);
  if(ret != LORS_SUCCESS) {
    errno = EINVAL;
    return -1;
  }
  
  if (jrb_empty(set->mapping_map)) {
    /* empty set */
    
    /*ret = lorsSetInit(&set, hdl->end/hdl->lors_threads, 1, 0);*/
    ret = lorsSetInit(&set, handle->lors_blocksize, 1, 0);
    if(ret != LORS_SUCCESS) {
      errno = EIO;
      return -1;
    }
    
    ret = lorsSetStore(set, handle->dp, (char *)buf, 
		       handle->offset, count, NULL, 
		       handle->lors_threads, handle->lors_timeout, 
		       LORS_RETRY_UNTIL_TIMEOUT);
    
    if(ret != LORS_SUCCESS) {
      lorsSetFree(set, LORS_FREE_MAPPINGS);
      errno = EIO;
      return -1;
    }

  
    /* set the version of each new mapping to 1 */
    ret = ADIOI_LNIO_lorsSetMappingVersion(set, 1);
    if (ret != LORS_SUCCESS) {
      errno = EIO;
      return -1;
    }
  } else {
    set->copies = handle->lors_copies;
    set->data_blocksize= handle->lors_blocksize; 
    /*count / handle->lors_threads; <- why did he do this?*/
    
    /* basically repeating what lorsSetUpdate does */

    ret = lorsSetInit(&newset, handle->lors_blocksize, 1, 0);
    if (ret != LORS_SUCCESS) {
      errno = EIO;
      return -1;
    }
    
    ret = lorsSetStore(newset, handle->dp, (char *)buf, 
		       handle->offset, count, NULL, 
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

    ret = lorsSetTrim(set, handle->offset, count, handle->lors_threads,
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
  
  /*updated by YING
    hdl->begin += hdl->b_pos;
    hdl->b_pos=0;
    hdl->end = 0;
    updated by YING*/

  handle->offset += count; 
  handle->exnode_modified = 1;

  return count;
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
	printf("ADIOI_LNIO_lorsSetAddUpdateMappings: there is a mapping added to updates, offset %Ld, length %d, dp is %s NULL\n", mp->exnode_offset, mp->logical_length, mp->dp ? "NOT" : "");
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

int ADIOI_LNIO_Flush(ADIO_File fd) 
{
  struct lnio_handle_t *handle = (struct lnio_handle_t *)fd->fs_ptr;
  int ret, rank, size, count, tempfd, length, i, *em_vector, modified;
  char *buf;

  LorsExnode *new_ex, *temp_ex;
  LorsSet *updates, *new_ex_set;
  LorsEnum list = NULL, it = NULL;
  LorsMapping *mp = NULL;
  LorsDepotPool *temp_dp;

  MPI_Request request;
  MPI_Status status;


  MPI_Comm_rank(fd->comm, &rank);
  MPI_Comm_size(fd->comm, &size);

  /* flush the buffer if it is dirty */
  if (handle->dirty_buffer) ADIOI_LNIO_Buffer_flush(fd, handle->buf_lb, handle->buf_ub - handle->buf_lb + 1);
  
  em_vector = (int *)calloc(size, sizeof(int));

  printf("[%d/%d] ADIOI_LNIO_Flush: entering\n", rank, size);

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
      printf("[%d/%d] ADIOI_LNIO_Flush: call FileDeserialize\n", rank, size);
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
      printf("[%d/%d] ADIOI_LNIO_Flush: there's a message from proc %d, tag %d, size %d\n", rank, size, status.MPI_SOURCE, status.MPI_TAG, count);
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
				0, NULL, handle->lors_threads, 
				handle->lors_timeout, 0);
      if (ret != LORS_SUCCESS) return ret;     
 
      /* add updated mappings from received exnode to updates */
      printf("[%d/%d] ADIOI_LNIO_Flush: calling ADIOI_LNIO_lorsAddUpdatedMappings for proc %d\n", rank, size, i);
      ADIOI_LNIO_lorsSetAddUpdatedMappings(handle, updates, temp_ex);


      lorsFreeDepotPool(temp_dp);
      lorsExnodeFree(temp_ex);
      free(buf);
    }
    
    /* now master combine the mappings in updates with new_ex */
    /* first, creates a set of mappings from new_ex */
    
    ret = lorsSetInit(&new_ex_set, handle->lors_blocksize, 1, 0);
    if (ret != LORS_SUCCESS) return ret;
    
    ret = lorsQuery(new_ex, &new_ex_set, 0, new_ex->logical_length, 
		    LORS_QUERY_REMOVE);
    if (ret != LORS_SUCCESS) return ret;
    
    /* then trim each mapping in updates from new_ex_set 
       and add it back to new_ex_set */
    list = NULL; it = NULL; mp = NULL;
    
    ret = lorsSetEnum(updates, &list);
    if (ret != LORS_SUCCESS) return ret;
    
    while (1) {
      ret = lorsEnumNext(list, &it, &mp);
      if (ret == LORS_END) break;
      printf("there is a mapping in updates, offset %Ld, length %d, dp is %s NULL\n", mp->exnode_offset, mp->logical_length, mp->dp ? "NOT" : "");
      ret = lorsSetTrim(new_ex_set, mp->exnode_offset, mp->logical_length,
			handle->lors_threads, handle->lors_timeout, 
			LORS_RETRY_UNTIL_TIMEOUT | LORS_TRIM_ALL);	
      if (ret != LORS_SUCCESS) return ret;      
    }
    
    lorsEnumFree(list);
    
    ret = lorsSetMerge(updates, new_ex_set);
    if (ret != LORS_SUCCESS) return ret;

    ret = lorsAppendSet(new_ex, new_ex_set);
    if (ret != LORS_SUCCESS) return ret; 
  }
  
  /* now, master serializes new_ex, broadcasts to other procs, and 
     writes to the xml file 
     non-masters receive new_ex and serialize it 
  */

  if (!rank) { /* master */
  /* assume that master is one of the aggregators or has an access to 
     the file system */

    ret = lorsFileSerialize(new_ex, fd->filename, 0, 0); 
    if (ret != LORS_SUCCESS) return ret;
    
    ret = lorsSerialize(new_ex, &buf, 0, &length);
    if (ret != LORS_SUCCESS) return ret;
  
    /* broadcast the serialized exnode to everybody */
    MPI_Bcast((void *)&length, 1, MPI_INT, 0, fd->comm);
    MPI_Bcast((void *)buf, length, MPI_CHAR, 0, fd->comm);
    
    lorsExnodeFree(handle->ex);
    handle->ex = new_ex;
  } else { /* non-master */
    MPI_Bcast((void *)&length, 1, MPI_INT, 0, fd->comm);
    buf = (char *)calloc(length + 1, 1);
    MPI_Bcast((void *)buf, length, MPI_CHAR, 0, fd->comm);
    
    lorsExnodeFree(handle->ex);
    ret = lorsExnodeCreate(&handle->ex);
    if (ret != LORS_SUCCESS) return ret;
    ret = lorsDeserialize(&handle->ex, buf, length, NULL);
    if (ret != LORS_SUCCESS) return ret;
  }
 
  /* create a new depot pool from the new exnode */
  /* (this will update the dp field of each mapping in the exnode, too) */
  ret = lorsFreeDepotPool(handle->dp);
  if (ret != LORS_SUCCESS) return ret;

  ret = lorsUpdateDepotPool(handle->ex, &handle->dp, handle->lbone_server, 
			    0, NULL, handle->lors_threads, 
			    handle->lors_timeout, 0);
  if (ret != LORS_SUCCESS) return ret;

  printf("[%d/%d] ADIOI_LNIO_Flush: exiting\n", rank, size);

  handle->exnode_modified = 0;

  if (buf) free(buf);
  if (em_vector) free(em_vector);

  return 0;
}

int ADIOI_LNIO_Ftruncate(ADIO_File fd, off_t size) 
{
  struct lnio_handle_t *handle = (struct lnio_handle_t *)fd->fs_ptr;
  int ret;
  LorsSet *set;

  printf("ADIOI_LNIO_Ftruncate: size %Ld, ex->logical_length %Ld\n", size, handle->ex->logical_length);

  if (size >= handle->ex->logical_length) {
    handle->ex->logical_length = size; /* check if this is safe */
    return 0;
  }

  ret = lorsQuery(handle->ex, &set, size, handle->ex->logical_length - size,
		  0 /* LORS_QUERY_REMOVE*/);
  if (ret != LORS_SUCCESS) return -1;
  
  ret = lorsSetTrim(set, size, handle->ex->logical_length - size, 
		    handle->lors_threads, handle->lors_timeout, 
		    LORS_RETRY_UNTIL_TIMEOUT | LORS_TRIM_ALL);
  if (ret != LORS_SUCCESS) return -1;

 
  /* lorsAppendSet(handle->ex, set); */
  
  printf("ADIOI_LNIO_Ftruncate: now ex->logical_length %Ld\n", size, handle->ex->logical_length);
  
  handle->ex->logical_length = size;

  lorsSetFree(set, 0);
  return 0;
}


/*******************************/
/*    Buffered I/O Routines    */
/*******************************/
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
