
#ifndef _ZOIDFS_BGL_INT_H_
#define _ZOIDFS_BGL_INT_H_

#include "zoidfs.h"

struct zoidfs_fsid_s
{
    /* this is a file descriptor returned from an
     * open("/dev/null") on the service node.  It
     * allows us to track file systems and clients
     */
    int fd;
};

#define ZOIDFS_FSID_SIZE sizeof(struct zoidfs_fsid_s)

struct zoidfs_handle_s
{
    /* we stuff a reference to the file system in here
     * so that we don't have to pass it in with each subsequent
     * call
     */
    struct zoidfs_fsid_s fsid;

    /* this is the system specific (PVFS2, etc.) handle to
     * the file.
     */
    uint64_t handle;
};

#define ZOIDFS_HANDLE_SIZE sizeof(struct zoidfs_handle_s)

#endif /* _ZOIDFS_BGL_INT_H_ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End: 
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */

