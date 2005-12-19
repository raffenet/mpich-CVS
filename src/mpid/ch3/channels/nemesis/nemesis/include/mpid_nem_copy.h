#ifndef MPID_NEM_COPY_H
#define MPID_NEM_COPY_H
void ntcopy(void *dst, const void *src, int size); 
void memcpy_8(void *destination, const void *source, int nbytes);
void memcpy_16(void *destination, const void *source, int nbytes);
void MP_memcpy(void *dst, const void *src, int nbytes);
void mmx_copy(void *dest, void *src, int nblock);
#endif /* MPID_NEM_COPY_H */
