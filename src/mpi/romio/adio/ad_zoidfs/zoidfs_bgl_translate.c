
#include "zoidfs_bgl_int.h"
#include "zoidfs_translate.h"

/**
 * Translate the parameters from the decoded control message to the file
 * system ID.  In the BG/L case, the file system ID is represented as an
 * open file descriptor from /dev/null.  So the variable arguments passed
 * to this function should just consist of an int.
 *
 * @param new_fsid The resulting file system id parameter filled in by the
 *        function from the variable arguments passed in.  This should be
 *        a non-null pointer.  The fields within the struct will be set
 *        by the function.
 * @param count The number of variable arguments being passed in.  For this
 *        impl. (BG/L), the value should always be 1.
 * @param ... The variable arguments from the decoded control message.  This
 *        should just be the int file descriptor representing the file system.
 * 
 * @return errno
 */
int 
zoidfs_translate_fsid(
    zoidfs_fsid_t * new_fsid,
    int count,
    ...)
{
    va_list ap;

    assert(new_fsid);
    va_start(ap, count);

    new_fsid->fd = va_arg(ap, int);

    va_end(ap);
    return 0;
}

/**
 * Translate the parameters from the decoded control message to the file
 * handle.  In the BG/L case, the file handle is represented as a
 * uint64_t.  So for BG/L the expected parameters to call this function
 * should be:
 *
 * zoidfs_translate_handle(
 *     zoidfs_fsid_t *    fsid,
 *     zoidfs_handle_t *  new_handle,
 *     1,
 *     uint64_t pvfs_handle);
 *
 * @param fsid The fsid created from a previous call to zoidfs_translate_fsid
 * @param new_handle the handle to initialize from the variable arguments.  This
 *        should be non-null.  Its fields are filled in by the impl.
 * @param count The number of variable arguments to follow.  In this case (BG/L)
 *        the value should always be 1
 * @param ...  The variable arguments that are the decoded parameters from
 *        the control message to convert to an zoidfs_handle_t
 *
 * @return errno
 */
int
zoidfs_translate_handle(
    zoidfs_fsid_t *   fsid,
    zoidfs_handle_t * new_handle,
    int count,
    ...)
{
    va_list ap;

    assert(new_handle);

    va_start(ap, count);

    new_handle->fsid.fd = fsid->fd;
    new_handle->handle = va_arg(ap, uint64_t);
    
    va_end(ap);
    return 0;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End: 
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */

