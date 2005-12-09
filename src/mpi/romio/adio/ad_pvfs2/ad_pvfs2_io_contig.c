#include "adio.h"
#include "adio_extern.h"
#include "ad_pvfs2.h"
#include "ad_pvfs2_io.h"
#include "ad_pvfs2_common.h"

int ADIOI_PVFS2_Contig(ADIO_File fd, void *buf, int count,
		       MPI_Datatype datatype, int file_ptr_type,
		       ADIO_Offset offset, ADIO_Status *status,
		       int *error_code, int rw_type)
{
    PVFS_Request mem_req, file_req;
    int ret = -1, datatype_size = -1, len = -1;
    PVFS_sysresp_io resp_io;
    ADIOI_PVFS2_fs *pvfs_fs;
    static char myname[] = "ADIOI_PVFS2_CONTIG";
    
    pvfs_fs = (ADIOI_PVFS2_fs*)fd->fs_ptr;
    MPI_Type_size(datatype, &datatype_size);
    len = datatype_size * count;

    /* Do not issue any zero sized requests */
    if (len == 0)
	return 0;

#ifdef DEBUG_CONTIG
    fprintf(stderr, "ADIOI_PVFS2_Contig:(mem_offset,mem_length)=(%d,%d) "
	    "(disp,offset)=(%d,%d)\n",
	    (int) buf, len, (int) fd->fp_ind, (int) offset);
#endif    

    /* Setup the memory and file datatypes for PVFS2 */
    ret = PVFS_Request_contiguous(len, PVFS_BYTE, &mem_req);
    if (ret != 0)
    {
	*error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_RECOVERABLE,
                                           myname, __LINE__,
                                           ADIOI_PVFS2_error_convert(ret),
                                           "Error in PVFS_Request_contiguous"
					   " (memory)", 0);
	return -1;
    }
    ret = PVFS_Request_contiguous(len, PVFS_BYTE, &file_req);
    if (ret != 0)
    {
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_RECOVERABLE,
                                           myname, __LINE__,
                                           ADIOI_PVFS2_error_convert(ret),
                                           "Error in PVFS_Request_contiguous"
                                           " (file)", 0);
        return -1;
    }
    
    /* Make I/O calls */
    if (file_ptr_type == ADIO_EXPLICIT_OFFSET) {
	if (rw_type == READ)
	    ret = PVFS_sys_read(pvfs_fs->object_ref, file_req, offset,  buf,
				mem_req, &(pvfs_fs->credentials), &resp_io);
	else
	    ret = PVFS_sys_write(pvfs_fs->object_ref, file_req, offset,  buf,
				 mem_req, &(pvfs_fs->credentials), &resp_io);

        if (ret != 0) {
            *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                               MPIR_ERR_RECOVERABLE,
                                               myname, __LINE__,
                                               ADIOI_PVFS2_error_convert(ret),
                                               "Error in PVFS_sys_io", 0);
            return -1;
        }
        fd->fp_sys_posn = offset + (int) resp_io.total_completed;
    }
    else {
	if (rw_type == READ)
	    ret = PVFS_sys_read(pvfs_fs->object_ref, file_req, 
				 fd->fp_ind, buf, mem_req, 
				 &(pvfs_fs->credentials), &resp_io);
	else
	    ret = PVFS_sys_write(pvfs_fs->object_ref, file_req, 
				 fd->fp_ind, buf, mem_req, 
				 &(pvfs_fs->credentials), &resp_io);
	    
        if (ret != 0) {
            *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                               MPIR_ERR_RECOVERABLE,
                                               myname, __LINE__,
                                               ADIOI_PVFS2_error_convert(ret),
                                               "Error in PVFS_sys_io", 0);
            return -1;
        }

        fd->fp_ind += (int)resp_io.total_completed;
        fd->fp_sys_posn = fd->fp_ind;
    }
    
#ifdef DEBUG_CONTIG2
    do 
    {
	int z;
	for (z = 0; z < len; z++)
	{
	    if (z % 5 == 0)
		fprintf(stderr, "ADIOI_PVFS2_Contig: ");
	    fprintf(stderr, "buf[%d]=%c ", z, ((char *) buf)[z]);
	    if ((z+1) % 5 == 0 && z != 0)
		fprintf(stderr, "\n");
	}
	if (z % 5 != 0)
	    fprintf(stderr, "\n");
    } while (0);
#endif    
	
    PVFS_Request_free(&mem_req);
    PVFS_Request_free(&file_req);

#ifdef HAVE_STATUS_SET_BYTES
    MPIR_Status_set_bytes(status, datatype, (int)resp_io.total_completed);
#endif
    *error_code = MPI_SUCCESS;
    return 0;
}
