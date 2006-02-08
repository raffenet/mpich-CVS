
#include "zoidfs_int.h"

zoidfs_fsid_t *
zoidfs_fsid_alloc()
{
   struct zoidfs_fsid_s * fsid;
  
   fsid = malloc(ZOIDFS_FSID_SIZE);
   if(!fsid)
   {
       return NULL;
   }

   return fsid;
}

void
zoidfs_fsid_free(zoidfds_fsid_t * fsid)
{
    free(fsid);
}

zoidfs_handle_t *
zoidfs_handle_alloc()
{
    zoidfs_handle_s * fhandle;

    fhandle = malloc(ZOIDFS_HANDLE_SIZE);
    if(!fhandle)
    {
        return NULL;
    }
    return fhandle;
}

void
zoidfs_handle_free(zoidfs_handle_t * handle)
{
    free(handle);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End: 
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */



