
#ifndef _ZOIDFS_TRANSLATE_H_
#define _ZOIDFS_TRANSLATE_H_

/**
 * Translate the parameters from the decoded control message to the file
 * system ID.  Each zoidfs implementation will encode/represent control
 * messages differently, so the variable arguments allows for a single
 * interface to multiple implementations that translate from control message
 * parameters to an opaque zoidfs_fsid_t. 
 *
 * @param new_fsid The resulting file system id parameter filled in by the
 *        function from the variable arguments passed in.  This should be
 *        a non-null pointer.  The fields within the struct will be set
 *        by the function.
 * @param count The number of variable arguments being passed in.  
 * @param ... The variable arguments from the decoded control message.   
 *
 * @return errno
 */
int
zoidfs_translate_fsid(
    zoidfs_fsid_t * new_fsid,
    int count,
    ...);

/**
 * Translate the parameters from the decoded control message to the file
 * handle.  Different implementations of this interface will represent
 * the zoidfs_handle_t differently.  Thus the need for variable arguments
 * to this function which allows different implementations to take different
 * sets of parameters.
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

void
zoidfs_translate_handle(
    zoidfs_handle_t * fhandle,
    int count,
    ...);

#endif /* _ZOIDFS_TRANSLATE_H_ */

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End: 
 *
 * vim: ts=8 sts=4 sw=4 expandtab
 */

