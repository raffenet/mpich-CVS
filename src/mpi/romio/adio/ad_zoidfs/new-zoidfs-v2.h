
/* TO BE COVERED:
 * 
 * - async io
 * - locking
 *
 * - need a consistent prefix on all our functions/enums/macros
 * Q: what do we do with the fsid/handle pair?
 *    - i think lee had some ideas/code for this
 */

/* add dir attrs
 * 
 */

/**
 * LEE: 
 *
 * - how do we want to do handles? opaque?
 * - what about creds?
 */

/* NOTES:
 *
 * - we optionally return the attributes for the parent handle
 *   with functions that require us to get the parent attrs, such as
 *   create, rel_lookup, etc.  The ZFNON allows the parent attr type
 *   to be optional by setting it to that value if the function doesn't
 *   fill it in.
 *
 *   - we make this optional because the idea behind doing this is that it
 *     is an inexpensive way to get cache updates back, but it's possible
 *     that grabbing the data during a particular operation might not be
 *     as cheap as we'd like
 *
 * - we should perhaps return attributes for other handles as well in cases
 *   where we get or have at least part of the attr in the function (I/O can
 *   update the size, or reset the atime, etc.).  For this we would perhaps
 *   need a mask that would allow us to specify which attr fields are being
 *   filled in.
 */

/**
 * This header defines an API for zoidfs.  Much of it is similar to the
 * nfsv2 functions, which are included for reference.
 * We also try to provide comments on the differences in this API. 
 */

/*
 *
 * enum ftype {
 *     NFNON = 0,
 *     NFREG = 1,
 *     NFDIR = 2,
 *     NFBLK = 3,
 *     NFCHR = 4,
 *     NFLNK = 5
 * };
 */
enum ztype {
    ZFNON = 0,
    ZFREG = 1,
    ZFDIR = 2

    /* do we need others? */
};

/**
 * struct fattr {
 *     ftype        type;
 *     unsigned int mode;
 *     unsigned int nlink;
 *     unsigned int uid;
 *     unsigned int gid;
 *     unsigned int size;
 *     unsigned int blocksize;
 *     unsigned int rdev;
 *     unsigned int blocks;
 * 
 *     unsigned int fsid;
 *     unsigned int fileid;
 *     timeval      atime;
 *     timeval      mtime;
 *     timeval      ctime;
 * };
 *
 * NOTES:
 * - nlink is always 1 in fattr for us.
 *   We don't need blocksize, rdev, or blocks.
 *   fsid and fileid are separate here (not part of fattr).
 * Q: should we have a bitfield to specify what is valid?  For some
 *    operations (e.g. setattr), we might not want to do the work to
 *    return valid results for all fields.
 */
typedef struct
{
    ztype type;
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint64_t size;
    struct zoidfs_time atime;
    struct zoidfs_time mtime;
    struct zoidfs_time ctime;

} zoidfs_attr_t;

/**
 * settable attributes are same as NFSv2 except that we make size 64-bit
 *
 * Q: do we want size in here, or do we want an explicit resize operation?
 * Q: don't we want a bitfield specifying what fields we want to set,
 *    rather than setting everything when we don't want to?
 */
typedef struct
{
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint64_t size;
    struct zoidfs_time atime;
    struct zoidfs_time mtime;
} zoidfs_sattr_t;


/**
 * We do want to specify a set of error codes so that we can build this
 * on multiple OSes and have consistent results.
 *   - look at pvfs2 and see if there is anything else we want to add to
 *     the NFSv2 set.
 *
 * enum stat {
 *     NFS_OK = 0,
 *     NFSERR_PERM=1,
 *     NFSERR_NOENT=2,
 *     NFSERR_IO=5,
 *     NFSERR_NXIO=6,
 *     NFSERR_ACCES=13,
 *     NFSERR_EXIST=17,
 *     NFSERR_NODEV=19,
 *     NFSERR_NOTDIR=20,
 *     NFSERR_ISDIR=21,
 *     NFSERR_FBIG=27,
 *     NFSERR_NOSPC=28,
 *     NFSERR_ROFS=30,
 *     NFSERR_NAMETOOLONG=63,
 *     NFSERR_NOTEMPTY=66,
 *     NFSERR_DQUOT=69,
 *     NFSERR_STALE=70,
 *     NFSERR_WFLUSH=99
 * };
 */

/**
 * Timeval - same as nfsv3
 */
struct zoidfs_time
{
    uint32_t seconds;
    uint32_t useconds;
};

/**
 * NULL
 *
 * INPUTS: none
 * OUTPUTS: none
 */
void
zoidfs_null(void);

/**
 * GETATTR
 *
 * INPUTS:
 *      - fhandle
 *
 * OUTPUTS:
 *      - attrstat
 *
 * NOTES:
 * - take a mask for which attrs to fill in? (like pvfs2)
 * 
 */
int
zoidfs_getattr(zoidfs_fsid_t fsid,
               zoidfs_handle_t handle,
               zoidfs_attr_t * attr);

/**
 * SETATTR
 *
 * INPUTS:
 *      - fhandle
 *      - sattr
 *
 * OUTPUTS:
 *      - fattr 
 *
 * NOTES:
 *
 * Q: symantically, should return attrs be optional here?
 *    - I think we always return something, but use a bitfield to
 *      specify what is valid in the returned structure (e.g we
 *      might not return a valid size)
 * 
 */
int zoidfs_setattr(const zoidfs_fsid_t * fsid,
                   const zoidfs_handle_t * handle,
                   zoidfs_sattr_t sattr,
                   zoidfs_attr_t * attr);
                   
/**
 * -- not needed
 * 
 * void
 * NFSPROC_ROOT(void)
 */

/**
 * LOOKUP
 *
 * INPUTS:
 *     fhandle  dir;
 *     filename name;
 *
 * OUTPUTS:
 *      fhandle file;
 *      fattr   attributes;
 *
 * NOTES:
 * - need to allow multi-component path lookups like in pvfs2.
 *   - we actually use these in mpi-io.
 * Q: do we return all the handles in the path?  What does pvfs2 actually do?
 * Q: do we also want all the attrs for each handle returned?  allows caches
 *    to be updated.  ( probably too much... )
 */
int
zoidfs_lookup(const zoidfs_fsid_t * fsid, 
 	      const char * path, 
              zoidfs_handle_t * handle);

int
zoidfs_rel_lookup(const zoidfs_fsid_t * fsid,
                  zoidfs_handle_t * parent_handle,
                  const char * name,
                  zoidfs_attr_t * parent_attr,
                  zoidfs_handle_t * handle);

/**
 * Lee can do this if desired :)
 *
 * readlinkres
 * NFSPROC_READLINK(fhandle)
 */

/**
 * I/O NOTES:
 *
 * - dunno what NFS totalcount is for
 * Q: don't return attributes from read/write?
 *    - might not be talking to the same servers
 *    Q: do same as in others; allow for NFNON or bitfield to tell us the
 *       values are invalid?
 * - offset needs to be 64bits
 * - also want listio
 *
 * - sizes must add up
 * - how do we do errors?  return sizes returned for each region?
 *   we propose if error, then adjust file sizes (file_sizes array -- dont have
 *   to copy array) to specify completed regions.  All other data 
 *   (such as offset that couldn't be written) is undefined. 
 */

/**
 * READ
 *
 * INPUTS:
 *     fhandle file;
 *     unsigned offset;
 *     unsigned count;
 *     unsigned totalcount;
 *
 * OUTPUTS:
 *     fattr attributes;
 *     nfsdata data;
 *
 * Q: what type corresponds to an integer that will hold something
 *    the size of a pointer?  that's what FOO should be.
 */
int
zoidfs_read(const zoidfs_fsid_t * fsid,
            const zoidfs_handle_t * handle,
            int mem_count,
            const void * mem_start[],
            const FOO mem_sizes[],
            int file_count,
            uint64_t file_start[],
            uint64_t file_sizes[]);
            
/*
 * Q: needed?
 * 
 * void
 * NFSPROC_WRITECACHE(void)
 *
 */

/**
 * WRITE
 *
 * INPUTS:
 *     fhandle file;
 *     unsigned beginoffset;
 *     unsigned offset;
 *     unsigned totalcount;
 *     nfsdata data;
 *
 * OUTPUTS:
 *     fattr attributes;
 *
 */
int
zoidfs_write(const zoidfs_fsid_t * fsid,
             const zoidfs_handle_t * handle,
             int mem_count,
             const void * mem_start[],
             const FOO mem_sizes[],
             int file_count,
             uint64_t file_start[],
             uint64_t file_sizes[]);

/**
 * Commit is needed to allow for a statelessness, where file handles
 * aren't opened/closed.
int
zoidfs_commit(const zoidfs_fsid_t * fsid,
              const zoidfs_handle_t * handle,
              int file_count,
              uint64_t file_start[],
              uint64_t file_sizes[]);

/**
 * CREATE
 *
 * INPUTS:
 *     fhandle handle;
 *     filename name;
 *     sattr attributes;
 *
 * OUTPUTS:
 *     fhandle file;
 *     fattr   attributes;
 *
 * NOTES:
 *
 * - we may not want to pass the entire path in to avoid a lookup on the
 *   client.
 * - we're returning parent attrs too as discussed previously.
 * - we've added a return flag specifying whether the file was created
 *   - return handle even if file already exists
 *   - avoids need for second lookup in the O_CREAT case
 */
int
zoidfs_create(const zoidfs_fsid_t * fsid,
              const char * parent,
              zoidfs_sattr_t attr,
              zoidfs_attr_t * parent_attr,
              zoidfs_handle_t * handle,
              int * created);

/**
 * REMOVE
 *
 * INPUTS:
 *      - fhandle
 *      - filename
 *
 * OUTPUTS:
 *      - stat
 *
 * NOTES:
 * - we're returning parent dir attrs
 */
int zoidsfs_remove(const zoidfs_fsid_t * fsid,
                   const zoidfs_handle_t * parent,
                   const char * name,
                   zoidfs_attr_t * parent_attr);

/*
 * we don't like hardlinks.
 *
 * stat
 * NFSPROC_LINK(linkargs)
 */

/*
 * Lee can do this if he's interested
 *
 * stat
 * NFSPROC_SYMLINK(symlinkargs)
 */
 
/** 
 * MKDIR
 *
 * INPUT:
 *     - fhandle dir;
 *     - filename name;
 *     - sattr attributes;
 *
 * OUTPUT:
 *     - stat
 * 
 * NOTES:
 * - added parent directory attribute return
 */
int zoidfs_mkdir(zoidfs_fsid_t fsid,
                 zoidfs_handle_t parent_handle,
                 const char * name,
                 zoidfs_sattr_t attr,
                 zoidfs_attr_t * parent_attr);

/**
 * RMDIR
 *
 * INPUT:
 *     - fhandle dir;
 *     - filename name;
 *
 * OUTPUT:
 *     - stat
 *
 * Q: what did the stat refer to in NFSv2?  Do we need that?
 */
int zoidfs_rmdir(zoidfs_fsid_t fsid,
                 zoidfs_handle_t dir_handle);


/**
 * READDIR
 *
 * INPUT:
 *     - fhandle dir;
 *     - nfscookie cookie;
 *     - unsigned count;
 *
 * OUTPUT:
 *     struct entry {
 *         unsigned fileid;
 *         filename name;
 *         nfscookie cookie;
 *         entry *nextentry;
 *     };
 *
 *     entry *entries;
 *     bool eof;
 *
 * TODO: we're going to need the cookie and don't have it here yet.
 */
typedef struct
{
    char name[ZOIDFS_NAME_MAX];
    zoidfs_handle_t handle;
} zoidfs_dirent_t;

int
zoidfs_readdir(const zoidfs_fsid_t * fsid,
               const zoidfs_handle_t * parent_handle,
               int * entry_count,
               zoidfs_dirent_t * entries,
               zoidfs_attr_t * parent_attr);

/**
 *
 * This is our way of getting a reference to a file system
 *
 * Q: what's the format of "fspath"?  Should that be "fsname" instead?
 */
int 
zoidfs_lookup(const char * fspath, 
              zoidfs_fsid_t * fsid);

/**
 * This is an optional call that hints to the other end that we're done
 * accessing the file system.  It's likely that clients will not get a
 * chance to call this in failure cases, so we can't count on it.
 */
int
zoidfs_release(zoidfs_fsid_t * fsid);

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End: 
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */

