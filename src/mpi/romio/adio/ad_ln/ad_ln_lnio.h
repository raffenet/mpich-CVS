#ifndef AD_LN_LNIO_INCLUDE
#define AD_LN_LNIO_INCLUDE

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

#include "adio.h"
#include "lors.h"
#include "lbone_client_lib.h"
#include <string.h>

#define MEGA 1024*1024

/* some lbone servers 
   { "acre.sinrg.cs.utk.edu", 6767},
   { "vertex.cs.utk.edu",    6767},
   { "galapagos.cs.utk.edu", 6767},
   { "adder.cs.utk.edu",     6767}
*/

#define LBONE_SERVER "vertex.cs.utk.edu" 
#define LBONE_PORT 6767
#define LBONE_LOCATION "zip= 90210" /*"zip= 37996"*/
#define LORS_BLOCKSIZE 1  /* 1 MB */
#define LORS_DURATION 60*60*10 /* 10 hours // 60*60*24*2  // 2 days */
#define LORS_COPIES 1
#define LORS_THREADS 2 
#define LORS_TIMEOUT 30 /* 1800 */
#define LORS_SERVERS 5
#define LORS_SIZE    1  /* 10 MB */



struct lnio_handle_t {
  LorsDepotPool *dp;
  LorsExnode	*ex;
  off_t          offset;

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

  int            exnode_modified;      

  char          *buffer;
  int            buffer_size;
  off_t          buf_lb;
  off_t          buf_ub;
  int            dirty_buffer;

  int            noncontig_write_naive;
  int            sync_at_collective_io;
  IBP_depot     *depot_array;
};

typedef struct lnio_mclist_item_t {
  LorsSet *set;
  longlong ub;
  longlong lb;
  struct lnio_mclist_item_t *next;
} lnio_mclist_item; 


/*
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
LorsConditionStruct *glob_lc = NULL;
#endif
*/

void ADIOI_LNIO_Free_handle(struct lnio_handle_t *handle);
int ADIOI_LNIO_Open(ADIO_File fd);
int ADIOI_LNIO_Close(ADIO_File fd);
off_t ADIOI_LNIO_Lseek(ADIO_File fd, off_t offset, int whence);
ssize_t ADIOI_LNIO_Read(ADIO_File fd, void *buf, size_t count);
ssize_t ADIOI_LNIO_Write(ADIO_File fd, const void *buf, size_t count);
int ADIOI_LNIO_Flush(ADIO_File fd);
int ADIOI_LNIO_lorsSetMappingVersion(LorsSet *set, int version);
int ADIOI_LNIO_lorsSetAddUpdatedMappings(struct lnio_handle_t *handle,
					 LorsSet *updates, LorsExnode *ex);
int ADIOI_LNIO_lorsExtractDepotList(LorsExnode *exnode, IBP_depot **dp_array,
				    int num_user_depot);
ssize_t ADIOI_LNIO_Buffered_read(ADIO_File fd, void *buf, size_t count);
ssize_t ADIOI_LNIO_Buffered_write(ADIO_File fd, const void *buf, size_t count);
int ADIOI_LNIO_Buffer_flush(ADIO_File fd, off_t flush_start_offset, size_t flush_size);
int ADIOI_LNIO_lorsCreateDepot(LorsDepot **depot, int dpid , IBP_depot ibpdepot, double bandwidth, double proximity);
#endif
