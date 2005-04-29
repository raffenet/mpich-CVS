/*
 *  libxio.c
 *
 *  Copyright (C) Lukas Hejtmanek - January 2004
 *
 *  This file is part of transcode, a linux video stream processing tool
 *      
 *  transcode is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  transcode is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#undef PACKAGE
#undef VERSION

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <stdarg.h>

#define ulong_t unsigned long int
#include "lors.h"
#include "lbone_client_lib.h"

#ifndef _GNU_SOURCE
char *strndup(const char *s, size_t n);
size_t strnlen(const char *s, size_t n);
#endif

struct xio_handle_t {

	ssize_t (*xio_read_v)(void *stream, void *buf, size_t count);
	ssize_t (*xio_write_v)(void *stream, const void *buf, size_t count);
	int (*xio_close_v)(void *stream);
	off_t (*xio_lseek_v)(void *stream, off_t offset, int whence);
	int (*xio_ftruncate_v)(void *stream, off_t length);
	int (*xio_fstat_v)(void *stream, struct stat *buf);
	
	void *data;
};

#define MPIXIO_MAX_HANDLES 256

struct xio_handle_t * _handles[MPIXIO_MAX_HANDLES];
int mpixio_initialized=0;
pthread_mutex_t mpixio_lock;

#define MEGABYTE 1024*1024

#define LBONE_SERVER "vertex.cs.utk.edu" 
#define LBONE_PORT 6767
#define LBONE_LOCATION "zip= 37996"
#define LORS_BLOCKSIZE 1  // 1 MB
#define LORS_DURATION 60*60*24*2  // 2 days
#define LORS_COPIES 1
#define LORS_THREADS 4 
#define LORS_TIMEOUT 1800
#define LORS_SERVERS 8
#define LORS_SIZE    10  //10 MB

struct mpixio_ibp_handle_t {
  LorsDepotPool *dp;
  LorsExnode	*ex;
  off_t          b_pos;
  off_t          begin;
  off_t		 end;
  void		*buffer;
  char          *filename;
  char		*lbone_server;
  int		 lbone_port;
  char		*lbone_location;
  int		 lors_blocksize;
  int 		 lors_duration;
  int		 lors_copies;
  int		 lors_threads;
  int		 lors_timeout;
  int		 lors_servers;
  off_t		 lors_size;
  int		 fill_buffer;
  int		 dirty_buffer;
  int 		 amode;
};

#if 0
LorsConditionStruct     glob_lc[] =
{
{"checksum",      0, lors_checksum,      lors_verify_checksum, LORS_E2E_FIXED, 1, NULL},
{"des_encrypt",   0, lors_des_encrypt,   lors_des_decrypt, LORS_E2E_FIXED,  1, NULL},
{"xor_encrypt",   0, lors_xor_encrypt,   lors_xor_decrypt, LORS_E2E_FIXED,  0, NULL},
{"zlib_compress", 0, lors_zlib_compress, lors_zlib_uncompress, LORS_E2E_VARIABLE, 1, NULL},
{"aes_encrypt",   0, lors_aes_encrypt,   lors_aes_decrypt, LORS_E2E_FIXED, 1, NULL},
{NULL,            0, NULL ,                          NULL,              0, 0, NULL},
};
#else
LorsConditionStruct *glob_lc=NULL;
#endif

void 
cancel_unlock(void *arg)
{
  pthread_mutex_unlock((pthread_mutex_t*)arg);
}

static struct mpxio_ibp_handle_t *
ibp_open(ADIO_File fd, int amode, int perm)
{
  struct mpxio_ibp_handle_t *handle;
  int ret, flag, fd;
  int file_exists, rank;
  char *value;
  

  handle = (struct mpxio_ibp_handle_t *)calloc(1,
		       sizeof(struct mpxio_ibp_handle_t));
  if (!handle) return NULL; // not enough memory

  
  // environment setup  
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
    handle->lors_blocksize = atoi(value);
  } else {
    if(amode & O_WRONLY || amode & O_CREAT)
      handle->lors_blocksize = LORS_BLOCKSIZE;
    else
      handle->lors_blocksize = 1;
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
    handle->lors_size = atoi(value);
  } else {
    handle->lors_size = LORS_SIZE;
  }
  
  MPI_Info_get(fd->info, "LORS_DEMO", MPI_MAX_INFO_VAL, valyem &flag);
  if (flag) {
    g_lors_demo = 1;
  } 

  free(value);


  if (!handle->lbone_server && !(amode & O_RDONLY || 
				!amode || amode & O_RDWR)) {
    errno = EINVAL;
    return (void *)-1;
  }
  
  handle->filename = strdup(uri);
  handle->lors_blocksize *= MEGABYTE;
  handle->lors_size *= MEGABYTE;
  
  handle->amode = amode;
  handle->b_pos = 0;
  handle->begin = 0;
  handle->end = 0;
  handle->dirty_buffer = 0;
  
  handle->buffer = malloc(handle->lors_blocksize * handle->lors_threads);
  if (!handle->buffer) {
    free(handle->filename);
    free(handle->lbone_server);
    free(handle->lbone_location);
    free(handle);
    /* or call free_handle() */

    errno = EIO;
    return NULL;
  }

  /* check if the file exists */
  fd = open(handle_file, O_RDONLY, perm);
  if (fd == -1) file_exists = 0; 
  else {
    file_exists = 1;
    close(fd);
  }

  if (!file_exists) {  /* file doesn't exist */

    fd = open(handle->filename, amode, perm);
    if(fd == -1) {
      free(handle->buffer);
      free(handle->filename);
      free(handle->lbone_server);
      free(handle->lbone_location)
      free(handle);

      errno = EIO;
      return NULL;
    }
    close(fd);

    ret = lorsGetDepotPool(&handle->dp, handle->lbone_server, 
			   handle->lbone_port, NULL, 
			   handle->lors_servers,
			   handle->lbone_location, 
			   handle->lors_size / MEGABYTE + 1,
			   /* shouldn't this be divided by lors_servers? */
			   IBP_SOFT, 
			   handle->lors_duration, 
			   handle->lors_threads,
			   handle->lors_timeout, 
			   LORS_CHECKDEPOTS);
    if(ret != LORS_SUCCESS) {
      free(handle->buffer);
      free(handle->filename);
      free(handle->lbone_server);
      free(handle->lbone_location);
      free(handle);
      
      errno = EIO;
      return NULL;
    }
    ret = lorsExnodeCreate(&handle->ex);
    if (ret != LORS_SUCCESS) {
      free(handle->buffer);
      free(handle->filename);
      free(handle->lbone_server);
      free(handle->lbone_location);
      free(handle);
      
      errno = EIO;
      return NULL;
    }
  } else { /* file exists */
    /* updated by YING */ 
    
    ret = lorsFileDeserialize(&handle->ex, handle->filename, NULL);
    if(ret != LORS_SUCCESS) {
      free(handle->buffer);
      free(handle->filename);
      free(handle->lbone_server);
      free(handle->lbone_location);
      free(handle);
      
      errno = EIO;
      return NULL;
    } else if(ret == LORS_SUCCESS) {
      LorsEnum list = NULL, it = NULL;
      LorsMapping *mp = NULL;
      lorsExnodeEnum (handle->ex, &list);
      lorsEnumNext(list, &it, &mp);
      lorsEnumFree(list);
      if(mp->e2e_bs && handle->lors_blocksize % mp->e2e_bs) {
	int size = (1 + handle->lors_blocksize / mp->e2e_bs)
	  * mp->e2e_bs;
	free(handle->buffer);
	handle->buffer = (char *)malloc(size * 4);
	handle->lors_blocksize = size;
      }
      handle->fill_buffer = 1;
      ret = lorsUpdateDepotPool(handle->ex, &handle->dp, 
				handle->lbone_server, 0, 
				NULL, handle->lors_threads, 
				handle->lors_timeout, 0);
      /* updated by YING */
      handle->dp->duration = handle->lors_duration;
      /* updated by YING */
      if(ret != LORS_SUCCESS) {
	free(handle->buffer);
	free(handle->filename);
	free(handle->lbone_server);
	free(handle->lbone_location);
	free(handle);
	
	errno = EIO;
	return NULL;
      }
    }
  }
  
  return handle;
}


static int
ibp_flush(void *handle)
{
	struct xio_ibp_handle_t *hdl = (struct xio_ibp_handle_t *)handle;
	int ret;
	LorsSet *set;

#ifdef YING_LORS_DEBUG
	LorsSet *tmpSet;
        LorsEnum list = NULL, iter = NULL;
        int lret=0;
        struct ibp_timer timer;
        int nbytes = 0;
        char *tmpbuf=NULL;
#endif
        errno = 0;
	if(handle == (void *)-1) {
                errno = EINVAL;
                return -1;
        }
	if(hdl->dirty_buffer == 0) {
		return 0;
	}

#ifdef YING_LORS_DEBUG
        fprintf(stderr,"BEFORE IBP_FLUSH:hdl->begin:%d;hdl->b_pos:%d;hdl->end:%d\n",hdl->begin,hdl->b_pos,hdl->end);
#endif
	ret = lorsQuery(hdl->ex, &set, hdl->begin, hdl->end, 
			LORS_QUERY_REMOVE);
	if(ret != LORS_SUCCESS) {
		errno = EINVAL;
		return -1;
	}
	if(jrb_empty(set->mapping_map)) {
		//ret = lorsSetInit(&set, hdl->end/hdl->lors_threads, 1, 0);
		ret = lorsSetInit(&set, hdl->lors_blocksize, 1, 0);
                if(ret != LORS_SUCCESS) {
	                errno = EIO;
	                return -1;
	        }
		ret = lorsSetStore(set, hdl->dp, hdl->buffer, 
				hdl->begin, hdl->end, NULL, 
				hdl->lors_threads, 
				hdl->lors_timeout, LORS_RETRY_UNTIL_TIMEOUT);
		
		if(ret != LORS_SUCCESS) {
			lorsSetFree(set,LORS_FREE_MAPPINGS);
			errno = EIO;
			return -1;
		}
	} else {
          set->copies=hdl->lors_copies;
          set->data_blocksize=hdl->end/hdl->lors_threads;

       	  ret = lorsSetUpdate(set, hdl->dp, hdl->buffer, 
				    hdl->begin, hdl->end,
				    hdl->lors_threads,hdl->lors_timeout,
				    LORS_RETRY_UNTIL_TIMEOUT);
          if(ret != LORS_SUCCESS) {
	                lorsSetFree(set,LORS_FREE_MAPPINGS);
		        errno=EIO;
		        return -1;
          }
      }

	ret = lorsAppendSet(hdl->ex, set);
        if(ret != LORS_SUCCESS) {
	        lorsSetFree(set, LORS_FREE_MAPPINGS);
                errno=EIO;
                return -1;
        }

#ifdef YING_LORS_DEBUG
        fprintf(stderr," IN IBP_FLUSH:SUCCESS ADD offset:%d;length:%d\n",hdl->begin,hdl->end);
        fprintf(stderr,"AFTER IBP_FLUSH:hdl->begin:%d;hdl->b_pos:%d;hdl->end:%d\n",hdl->begin,hdl->b_pos,hdl->end);
#endif	
        lorsSetFree(set,0);

/*updated by YING
	hdl->begin += hdl->b_pos;
        hdl->b_pos=0;
	hdl->end = 0;
updated by YING*/

	ret = lorsFileSerialize(hdl->ex, hdl->filename, 0, 0);
	
        if(ret != LORS_SUCCESS) {
	        perror("file serialize");
	}
#ifdef YING_LORS_DEBUG
        fprintf(stderr,"AFTER IBP_FLUSH;hdl->ex->logical_length:%d\n",hdl->ex->logical_length);
        fprintf(stderr,"EXIT IBP_FLUSH\n");
#endif
	hdl->dirty_buffer = 0;
        errno = 0;
	return 0;
}

static ssize_t
ibp_write(void *handle, const void *buffer, size_t size)
{
	struct xio_ibp_handle_t *hdl = (struct xio_ibp_handle_t *)handle;

        errno = 0;
	pthread_testcancel();
	if(handle == (void *)-1) {
                errno = EINVAL;
                return -1;
        }
	pthread_cleanup_push(cancel_unlock, &xio_lock);
	pthread_mutex_lock(&xio_lock);
	pthread_testcancel();

#ifdef YING_LORS_DEBUG
        fprintf(stderr,"In IBP_WRITE BEFORE,hdl->begin:%d, hdl->b_pos:%d, offset:%d size:%d\n",hdl->begin, hdl->b_pos,hdl->begin+hdl->b_pos,size);
#endif	
        if(size > hdl->lors_blocksize*hdl->lors_threads){
		size = hdl->lors_blocksize*hdl->lors_threads;
        }
#ifdef YING_LORS_DEBUG
        fprintf(stderr,"IN IBP_WRITE, need to write %d bytes\n",size);
#endif
	if(!hdl || !buffer || hdl->mode == O_RDONLY || !hdl->buffer) {
		errno = EINVAL;
		size = -1;
		goto out;
	}

	if(hdl->b_pos + size > hdl->lors_blocksize*hdl->lors_threads) {
           if(hdl->dirty_buffer){
#ifdef YING_LORS_DEBUG
                fprintf(stderr,"CALL IBP_FLUSH IN IBP_WRITE\n");
#endif
		if(ibp_flush(handle)) {
			errno = EIO;
			size = -1;
			goto out;
		}
           }
           hdl->begin += hdl->b_pos;
           hdl->b_pos = 0;
           hdl->end = 0;
           /* newadded */
           hdl->fill_buffer = 1; 
	}
		
	memcpy((char *)hdl->buffer + hdl->b_pos, buffer, size);
        hdl->fill_buffer = 0; 
	hdl->dirty_buffer = 1;
	hdl->b_pos += size;
	if(hdl->end < hdl->b_pos){
		hdl->end = hdl->b_pos;
        }
#ifdef YING_LORS_DEBUG
        fprintf(stderr,"In IBP_WRITE AFTER,hdl->begin:%d, hdl->b_pos:%d, hdl->end:%d \n",hdl->begin, hdl->b_pos,hdl->end);
#endif
out:	
	pthread_mutex_unlock(&xio_lock);
	pthread_cleanup_pop(0);

	return size;
}

static ssize_t
ibp_read(void *handle, void *buffer, size_t size)
{
  struct xio_ibp_handle_t *hdl = (struct xio_ibp_handle_t *)handle;
  int ret;
  LorsSet *set;
  
#ifdef YING_LORS_DEBUG
  fprintf(stderr,"In IBP_READ BEFORE ,hdl->begin:%d, hdl->b_pos:%d, hdl->end:%d \n",hdl->begin, hdl->b_pos,hdl->end);
#endif
  errno = 0;
  pthread_testcancel();
  if(handle == (void *)-1) {
    errno = EINVAL;
    return -1;
  }
  pthread_cleanup_push(cancel_unlock, &xio_lock);
  pthread_mutex_lock(&xio_lock);
  pthread_testcancel();
  
  /* updated by YING */	
  if(hdl->mode & O_WRONLY == 1) {
    errno = EINVAL;
    size = -1;
    goto out;
  }
  
#ifdef YING_LORS_DEBUG
  fprintf(stderr,"hdl->fill_buffer is %d\n",hdl->fill_buffer);
#endif
  if(hdl->b_pos < hdl->end && !hdl->fill_buffer) {
    if(size > hdl->end - hdl->b_pos){
      size = hdl->end - hdl->b_pos;
    }
#ifdef YING_LORS_DEBUG
    fprintf(stderr,"IN IBP_READ, will read %d bytes\n",size);
#endif
    memcpy(buffer, hdl->buffer+hdl->b_pos, size);
    hdl->b_pos += size;
    errno = 0;
    goto out;
  }
  /* updated by YING */
  /* if hdl->end==hdl->b_pos, buffer may still has free space, doesn't need to be released */
  if ((hdl->end < hdl->b_pos) || (hdl->b_pos == hdl->lors_blocksize*hdl->lors_threads)){ 
    if(hdl->dirty_buffer){
#ifdef YING_LORS_DEBUG
      fprintf(stderr,"CALL IBP_FLUSH_READ1\n");
#endif
      if(ibp_flush(handle)) {
	fprintf(stderr,"IN IBP_READ, RETURN -1 due to IBP_FLUSH\n");
	errno = EIO;
	size = -1;
	goto out;
      }
    }
    hdl->begin+=hdl->b_pos;
    hdl->b_pos=0;
    hdl->end = 0;
    /* new added */
    hdl->fill_buffer = 1;
  }
  
  if(hdl->ex->logical_length <= hdl->begin + hdl->b_pos) {
#ifdef YING_LORS_DEBUG
    fprintf(stderr,"IN IBP_READ, exnode is shorter than required, errno:%d\n",errno);
#endif
    errno = 0;
    size = 0;
    goto out;
  }
  
  if (size > hdl->lors_blocksize*hdl->lors_threads-hdl->b_pos) size= hdl->lors_blocksize*hdl->lors_threads-hdl->b_pos;
  /* updated by YING */
  
#ifdef YING_LORS_DEBUG
  fprintf(stderr,"IN IBP_READ, will read %d bytes\n",size);
#endif
  //hdl->fill_buffer = 0;
  
  ret = lorsQuery(hdl->ex, &set, hdl->begin + hdl->b_pos,size,0); 
  if(ret != LORS_SUCCESS) {
    fprintf(stderr,"IN IBP_READ;size=0 because LORSQUERY\n");
    errno = EIO;
    size = 0;
    goto out;
  }
  
  /* updated by YING */
  if (jrb_empty(set->mapping_map)){
#ifdef YING_LORS_DEBUG
    fprintf(stderr,"IN IBP_READ, NOTHING in SET\n");
#endif
    lorsSetFree(set, 0);
    if(hdl->dirty_buffer){
#ifdef YING_LORS_DEBUG
      fprintf(stderr,"CALL IBP_FLUSH_READ2\n");
#endif
      if(ibp_flush(handle)) {
	//fprintf(stderr,"IN IBP_READ, RETURN -1 due to IBP_FLUSH\n");
	errno = EIO;
	size = -1;
	goto out;
      }
    }
    hdl->end = size;
    hdl->begin += hdl->b_pos;
    hdl->b_pos = size;
    bzero(buffer,size);
    bzero(hdl->buffer,size);
    hdl->dirty_buffer = 1;
    /* new added */
    hdl->fill_buffer = 0;
    errno = 0;
    goto out;
  }
  /* updated by YING */
  
  
  ret = lorsSetLoad(set, hdl->buffer+hdl->b_pos, hdl->begin + hdl->b_pos, 
		    size, hdl->lors_blocksize,
		    glob_lc, hdl->lors_threads, hdl->lors_timeout, 0);
  
  
  /* updated by YING */	
  if ((ret < 0)&&(ret!=-9)) {
    fprintf(stderr,"IN IBP_READ, RETURN -1 due to LORSSETLOAD\n");
    errno = EIO;
    size = -1;
    goto out;
  }
  /* updated by YING */
  
  /* updated by YING */
  if (ret == -9){
    int lret;
    int curOffset=hdl->begin + hdl->b_pos;
    int leastOffset=-1;
#ifdef YING_LORS_DEBUG
    fprintf(stderr,"IN IBP_READ, ret is -9\n");
#endif
    LorsEnum  list = NULL, iter = NULL;
    lorsSetEnum(set, &list);
    do {
      LorsMapping        *lm = NULL;
      lret = lorsEnumNext(list, &iter, &lm);
      if (lm != NULL){ 
#ifdef YING_LORS_DEBUG
	fprintf(stderr,"Mapping in IBP_READ %d offset:%lld len:%ld %s:%4d\n",lm->id,lm->exnode_offset,lm->logical_length,lm->depot.host,lm->depot.port);
#endif
	if ((leastOffset == -1) || (lm->exnode_offset < leastOffset))
	  leastOffset = lm->exnode_offset;
      }
    }while ( lret != LORS_END ); 
    lorsEnumFree(list);
    lorsSetFree(set, 0);
    ret = leastOffset-curOffset;
    if(hdl->dirty_buffer){
#ifdef YING_LORS_DEBUG
      fprintf(stderr,"CALL IBP_FLUSH_READ3\n");
#endif
      if(ibp_flush(handle)) {
#ifdef YING_LORS_DEBUG
	fprintf(stderr,"IN IBP_READ, RETURN -1 due to IBP_FLUSH\n");
#endif
	errno = EIO;
	size = -1;
	goto out;
      }
    }
    hdl->end = ret;
    hdl->begin += hdl->b_pos;
    hdl->b_pos= ret;
    bzero(buffer,ret);
    bzero(hdl->buffer,ret);
    hdl->dirty_buffer = 1;
    /* new added */
    hdl->dirty_buffer = 1;
    hdl->fill_buffer = 0;
    size = ret;
    errno = 0;
    goto out;
  }
  /* updated by YING */
  
  /* new added */
  hdl->fill_buffer = 0;
  
  lorsSetFree(set, 0);
  memcpy(buffer, hdl->buffer+hdl->b_pos, ret);
  hdl->end += ret;
  hdl->b_pos += ret;
#ifdef YING_LORS_DEBUG
  fprintf(stderr,"In IBP_READ DEBUG AFTER ,hdl->begin:%d, hdl->b_pos:%d, hdl->end:%d \n",hdl->begin, hdl->b_pos,hdl->end);
#endif
  size = ret;
  errno =0;
 out:
  pthread_cleanup_pop(0);
  pthread_mutex_unlock(&xio_lock);
#ifdef YING_LORS_DEBUG
  fprintf(stderr,"in IBP_READ; finally return %d\n",size);
  fprintf(stderr,"In IBP_READ AFTER ,hdl->begin:%d, hdl->b_pos:%d, hdl->end:%d \n",hdl->begin, hdl->b_pos,hdl->end);
#endif
  return size;
}

static off_t
ibp_lseek(void *handle, off_t offset, int whence)
{
	struct xio_ibp_handle_t *hdl = (struct xio_ibp_handle_t *)handle;
	off_t ret;

#ifdef YING_LORS_DEBUG
        fprintf(stderr,"In IBP_SEEK BEFORE  ,hdl->begin:%d, hdl->b_pos:%d, hdl->end:%d \n",hdl->begin, hdl->b_pos,hdl->end);
#endif
        errno = 0;
	if(handle == (void *)-1) {
                errno = EINVAL;
                return -1;
        }
	pthread_cleanup_push(cancel_unlock, &xio_lock);
	pthread_mutex_lock(&xio_lock);
	pthread_testcancel();

	if(mode == SEEK_SET) {
		if(offs - hdl->begin > hdl->lors_blocksize*hdl->lors_threads || 
				offs < hdl->begin) {
#ifdef YING_LORS_DEBUG
                  fprintf(stderr,"IN SEEK_SET:offs:%d,hdl->begin:%d\n",offs,hdl->begin);
#endif
                  if(hdl->dirty_buffer){
#ifdef YING_LORS_DEBUG
                fprintf(stderr,"CALL IBP_FLUSH in LSEEK_SET\n");
#endif
		       if(ibp_flush(handle)) {
				errno = EIO;
				ret = -1;
				goto out;
			}
                   }
	           hdl->fill_buffer = 1;
		   hdl->begin = offs;
		   hdl->b_pos = 0;
                   /* updated by YING */
                   hdl->end = 0;
                   /* updated by YING */
		} else {
			hdl->b_pos = offs - hdl->begin;
		}
#ifdef YING_LORS_DEBUG
                fprintf(stderr,"In IBP_SEEKSET AFTER  ,hdl->begin:%d, hdl->b_pos:%d, hdl->end:%d \n",hdl->begin, hdl->b_pos,hdl->end);
#endif
	}
	else if(mode == SEEK_CUR) {
		if(hdl->b_pos + offs > hdl->lors_blocksize*hdl->lors_threads || 
				hdl->b_pos + offs < 0) {
                  if(hdl->dirty_buffer){
#ifdef YING_LORS_DEBUG
                        fprintf(stderr,"CALL IBP_FLUSH in LSEEK_CUR\n");
#endif
			if(ibp_flush(handle)) {
				errno = EIO;
				ret = -1;
				goto out;
			}
                   }	
                   hdl->fill_buffer = 1;
		   hdl->begin = hdl->begin + hdl->b_pos + offs;
		   hdl->b_pos = 0;
                   /* updated by YING */
                   hdl->end = 0;
                   /* updated by YING */
		} else {
			hdl->b_pos += offs;
		}
#ifdef YING_LORS_DEBUG
                //fprintf(stderr,"In IBP_SEEKCUR AFTER  ,hdl->begin:%d, hdl->b_pos:%d, hdl->end:%d \n",hdl->begin, hdl->b_pos,hdl->end);
#endif
	} else if(mode == SEEK_END) {
		if(ibp_flush(handle)) {
			errno = EIO;
			ret = -1;
			goto out;
		}
		hdl->fill_buffer = 1;
		hdl->begin = hdl->ex->logical_length + offs;
		hdl->b_pos = 0;
	} else {
		errno=EINVAL;
                ret = -1;
		goto out;
	}
	ret = hdl->begin + hdl->b_pos;
        errno = 0;
out:
#ifdef YING_LORS_DEBUG
        fprintf(stderr,"EXIT IBP_LSEEK, hdl->dp->duration is %d\n",hdl->dp->duration);
#endif
	pthread_mutex_unlock(&xio_lock);
	pthread_cleanup_pop(0);
	return ret;
}

static int
ibp_close(void *handle) 
{
	struct xio_ibp_handle_t *hdl = (struct xio_ibp_handle_t *)handle;
	int ret;

	if(handle == (void *)-1 || !hdl) {
                errno = EINVAL;
                return -1;
        }
	pthread_cleanup_push(cancel_unlock, &xio_lock);
	pthread_mutex_lock(&xio_lock);
	pthread_testcancel();
	if(hdl->mode != O_RDONLY) {
		if(hdl->dirty_buffer){
                        //fprintf(stderr,"CALL IBP_FLUSH IN IBP_CLOSE\n");
			ibp_flush(handle);
		}else {
			ret = lorsFileSerialize(hdl->ex, hdl->filename, 0, 0);
		        if(ret != LORS_SUCCESS) {
		                perror("file serialize");
		        }
		}
	}
	if(hdl->buffer)
		free(hdl->buffer);
	lorsExnodeFree(hdl->ex);
	if(hdl->dp)
		lorsFreeDepotPool(hdl->dp);
	pthread_mutex_unlock(&xio_lock);
	pthread_cleanup_pop(0);
	free(hdl);
	return 0;
}

static int
ibp_ftruncate(void *stream, off_t length)
{
	struct xio_ibp_handle_t* hdl = (struct xio_ibp_handle_t*)stream;
	int ret;
	LorsSet *set;

	pthread_cleanup_push(cancel_unlock, &xio_lock);
	pthread_mutex_lock(&xio_lock);
	pthread_testcancel();
	ibp_flush(stream);
	hdl->b_pos = 0;
	if(length == hdl->ex->logical_length) {
		ret = 0;
		goto out;
	}
	
	ret = lorsQuery(hdl->ex, &set, length, 
			hdl->ex->logical_length-length, 0);
        if(ret != LORS_SUCCESS) {
	        errno = EINVAL;
	        ret = -1;
	}
	ret = lorsSetTrim(set, length, hdl->ex->logical_length-length, 
			1, 20, LORS_TRIM_ALL);
	lorsSetFree(set, 0);
	if(ret != LORS_SUCCESS) {
		errno = EIO;
		ret = -1;
		goto out;
	}
	ret = 0;
out:
	pthread_mutex_unlock(&xio_lock);
	pthread_cleanup_pop(0);
	return ret;
}

static char *
ibp_lorstoname(char *file_name)
{
	char * uri = file_name;
	if(strncmp(uri, IBP_URI, strlen(IBP_URI)) != 0) {
		return NULL;
	}
	uri += strlen(IBP_URI);
	uri = (char *)(strchr(uri, '/')+1);
	if(!strchr(uri, '?')) {
		return strdup(uri);
	}
	return strndup(uri, (int)(strchr(uri, '?')-uri));
}

static int
ibp_stat(const char *file_name, struct stat *buf)
{
	LorsExnode *exnode;
	int ret;
	char *fn;
	
	fn = ibp_lorstoname((char *)file_name);
	if(fn) {
		ret = lorsFileDeserialize(&exnode, fn, NULL);
		if(ret!=0) {
			if(stat(fn, buf) == -1) 
				goto err;
			else /* asi jde o adresar */
				return 0;
	        }
		if(stat(fn, buf) == -1)
			goto err;
		free(fn);
	  	buf->st_ino=-1;
	        buf->st_dev=-1;
	        buf->st_size=exnode->logical_length;
	        lorsExnodeFree(exnode);
		return 0;
	}
	return stat(file_name, buf);
err:
	free(fn);
	errno=EACCES;
	return -1;
}

static int
ibp_lstat(const char *file_name, struct stat *buf)
{
	char *fn = ibp_lorstoname((char *)file_name);
	LorsExnode *exnode;
	int ret;
        if(lstat(fn,buf) == -1) {
		free(fn);
                return -1;
        }
	if(S_ISLNK(buf->st_mode)) {
		free(fn);
		return 0;
	} else {
		ret = lorsFileDeserialize(&exnode, fn, NULL);
		if(ret!=0) {
			return 0;
		}
		buf->st_ino=-1;
	        buf->st_dev=-1;
	        buf->st_size=exnode->logical_length;
	        lorsExnodeFree(exnode);
		free(fn);
		return 0;
	}
}

static int
ibp_fstat(void *stream, struct stat *buf)
{
	struct xio_ibp_handle_t *hdl = (struct xio_ibp_handle_t *)stream;
	LorsExnode *exnode;
        int ret;
        char *fn;

        ret = lorsFileDeserialize(&exnode, hdl->filename, NULL);
        if(ret!=0) {
	        if(stat(hdl->filename, buf) == -1)
	                goto err;
                else /* asi jde o adresar */
                        return 0;
        }
        if(stat(hdl->filename, buf) == -1)
  		goto err;
        buf->st_ino=-1;
        buf->st_dev=-1;
        buf->st_size=exnode->logical_length;
        lorsExnodeFree(exnode);
        return 0;
err:
        errno=EACCES;
        return -1;
}


static int
mpixio_init()
{
  int i;
  for(i = 0; i < MPIXIO_MAX_HANDLES; i++)
    _handles[i] = NULL;
  
  mpixio_initialized = 1;
  pthread_mutex_init(&mpixio_lock, NULL);
  return 0;
}

int 
mpixio_open(ADIO_File fd, int amode, int perm)
{
  int i;
  int ret_fd;
  int mode=0;
  
  if(!mpixio_initialized && io_init() != 0) {
    errno = EIO;
    return -1;
  }
	
  pthread_mutex_lock(&mpixio_lock);
  // Find free IO handle
  i = 0;
  while (_handles[i] != NULL && i < MPIXIO_MAX_HANDLES) i++;
  ret_fd = i;
  if (i == MPIXIO_MAX_HANDLES) {
    // no free io handle
    pthread_mutex_unlock(&mpixio_lock);
    errno = EIO;
    return -1;
  }
  pthread_mutex_unlock(&mpixio_lock);
  
  _handles[ret_fd] = ibp_open(fd, amode, perm);
  if (_handles[ret_fd] == NULL) {
    errno = EIO;
    /* update by YING */
    if (amode & O_EXCL) errno = EEXIST;
    return -1;
  }
  
  return ret_fd;
}

ssize_t
mpixio_read(int fd, void *buf, size_t count)
{
        ssize_t tmp;
#ifdef YING_DEBUG
        fprintf(stderr,"ENTER XIO_READ,count %d\n",count);
#endif
	if(!xio_initialized && io_init() != 0) {
		errno = EIO;
                return -1;
	}
	
	if(fd < 0 || fd > MAX_HANDLES) {
                errno = EINVAL;
                return -1;
        }

	if(!_handles[fd])
		return read(fd, buf, count);

	tmp=_handles[fd]->xio_read_v(_handles[fd]->data, buf, count);
#ifdef YING_DEBUG
        fprintf(stderr,"EXIT XIO_READ, return %d\n",tmp);
        if (((char *)buf)[0] == '\0') fprintf(stderr,"BUF IS NULL FOR XIO_READ\n");  
#endif
        return tmp;
}

ssize_t
xio_write(int fd, const void *buf, size_t count)
{
#ifdef YING_DEBUG
        fprintf(stderr,"ENTER XIO_WRITE,count %d\n",count);
#endif
        ssize_t tmp;
	if(!xio_initialized && io_init() != 0) {
		errno = EIO;
                return -1;
	}

	if(fd < 0 || fd > MAX_HANDLES) {
                errno = EINVAL;
                return -1;
        }
	
	if(!_handles[fd])
		return write(fd, buf, count);

	tmp=_handles[fd]->xio_write_v(_handles[fd]->data, buf, count);
#ifdef YING_DEBUG
        fprintf(stderr,"EXIT XIO_WRITE, return %d\n",tmp);
#endif
        return tmp;
}

int
xio_ftruncate(int fd, off_t length)
{
        int tmp;
	if(!xio_initialized && io_init() != 0) {
		errno = EIO;
		return -1;
	}

	if(fd < 0 || fd > MAX_HANDLES) {
                errno = EINVAL;
                return -1;
        }
	if(!_handles[fd])
		return ftruncate(fd, length);
	tmp=_handles[fd]->xio_ftruncate_v(_handles[fd]->data, length);
        return tmp;
}

off_t
xio_lseek(int fd, off_t offset, int whence)
{
        int tmp;
#ifdef YING_DEBUG
        fprintf(stderr,"ENTER XIO_LSEEK, offset: %d ",offset);
        if (whence == SEEK_SET) fprintf(stderr, "SEEK_SET\n");
        if (whence == SEEK_CUR) fprintf(stderr, "SEEK_CUR\n");
#endif
	if(!xio_initialized && io_init() != 0) {
		errno = EIO;
                return -1;
	}

	if(fd < 0 || fd > MAX_HANDLES) {
                errno = EINVAL;
                return -1;
        }
	
	if(!_handles[fd]){
		return lseek(fd, offset, whence); 
        }

	tmp =  _handles[fd]->xio_lseek_v(_handles[fd]->data, offset, whence);
#ifdef YING_DEBUG
        fprintf(stderr,"EXIT XIO_LSEEK, tmp: %d\n",tmp);
#endif
        return tmp;
}

int
xio_close(int fd)
{
	int ret;
	if(!xio_initialized && io_init() != 0) {
		errno = EIO;
		return -1;
	}
	if(fd < 0 || fd > MAX_HANDLES) {
		errno = EINVAL;
		return -1;
	}
	
	if(!_handles[fd])
		return close(fd);
	ret = _handles[fd]->xio_close_v(_handles[fd]->data);	
	free(_handles[fd]);
	_handles[fd]=NULL;
	return ret;
}

int
xio_stat(const char *file_name, struct stat *buf)
{
	if(!xio_initialized && io_init() != 0) {
		errno = EIO;
                return -1;
	}

	if(strncmp(file_name, IBP_URI, strlen(IBP_URI)) == 0) {
		return ibp_stat(file_name, buf);
	}
	return stat(file_name, buf);
}

int
xio_lstat(const char *file_name, struct stat *buf)
{
        if(!xio_initialized && io_init() != 0) {
                errno = EIO;
                return -1;
        }

	if(strncmp(file_name, IBP_URI, strlen(IBP_URI)) == 0) {
		return ibp_lstat(file_name, buf);
        }
        return lstat(file_name, buf);
}

int
xio_rename(const char *oldpath, const char *newpath)
{
	char *old, *old_p;
	char *newp;
	int ret;
	
	if(strncmp(IBP_URI, oldpath, strlen(IBP_URI)) == 0) {
		old_p = old = strdup(oldpath);
		old = strchr(old+strlen(IBP_URI),'/')+1;
		if(strchr(old, '?')) {
			*(strchr(old, '?')) = 0;
		} 
			
		newp = (char*)malloc(strlen(old)+1+4);
		snprintf(newp, strlen(old)+1+4, "%s%s", old, &newpath[strlen(newpath)-4]);
		ret = rename(old, newp);
		free(old_p);
		return ret;
	}
	return rename(oldpath, newpath);
}

int
xio_fstat(int fd, struct stat *buf)
{
        int tmp;
        if(!xio_initialized && io_init() != 0) {
                errno = EIO;
                return -1;
        }
	if(fd < 0 || fd > MAX_HANDLES) {
		errno = EINVAL;
		return -1;
	}

        tmp= _handles[fd]->xio_fstat_v(_handles[fd]->data, buf);
        return tmp;
}

//#ifndef _GNU_SOURCE
#if 1
/* Some platforms don't have strndup. */

char *strndup(const char *s, size_t n)
{
	char *ret;
	n = strnlen(s, n);
	ret = malloc(n+1);
	if (!ret) return NULL;
	memcpy(ret, s, n);
	ret[n] = 0;
	return ret;
}
//#ifndef HAVE_STRNLEN
/* Some platforms don't have strnlen */

size_t strnlen(const char *s, size_t n)
{
	int i;
	for (i=0; s[i] && i<n; i++)
	/* noop */ ;
	return i;
}
//#endif
#endif
