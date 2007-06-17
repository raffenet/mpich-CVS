/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This file is here only to keep includes in one place inside the
 * dataloop subdirectory. This file will be different for each use
 * of the dataloop code, but others should be identical.
 */
#ifndef VECCPY_H
#define VECCPY_H

#define MPIDI_VEC_TO_BUF(src,dest,stride,type,nelms,count,soffset,doffset) \
{								\
    type * l_src = (type *)src, * l_dest = (type *)dest;	\
    register int i, j;		                                \
    unsigned long total_count = count * nelms;                  \
    const int l_stride = stride;				\
                                                                \
    if (!nelms) {                                               \
        for (i = count; i; i--)			                \
            l_src = (type *) ((char *) l_src + l_stride);	\
    }                                                           \
    if (!(nelms % 8)) {			                        \
        for (i = total_count; i; i -= 8) {			\
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            *l_dest++ = l_src[4];				\
            *l_dest++ = l_src[5];				\
            *l_dest++ = l_src[6];				\
            *l_dest++ = l_src[7];				\
            if (!((i - 8) % nelms))                             \
                l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }								\
    else if (!(nelms % 7)) {			                \
        for (i = total_count; i; i -= 7) {			\
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            *l_dest++ = l_src[4];				\
            *l_dest++ = l_src[5];				\
            *l_dest++ = l_src[6];				\
            if (!((i - 7) % nelms))                             \
                l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }								\
    else if (!(nelms % 6)) {			                \
        for (i = total_count; i; i -= 6) {			\
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            *l_dest++ = l_src[4];				\
            *l_dest++ = l_src[5];				\
            if (!((i - 6) % nelms))                             \
                l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }								\
    else if (!(nelms % 5)) {			                \
        for (i = total_count; i; i -= 5) {			\
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            *l_dest++ = l_src[4];				\
            if (!((i - 5) % nelms))                             \
                l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }								\
    else if (!(nelms % 4)) {                                    \
        for (i = total_count; i; i -= 4) {			\
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            if (!((i - 4) % nelms))                             \
                l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else if (!(nelms % 3)) {                                    \
        for (i = total_count; i; i -= 3) {			\
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            if (!((i - 3) % nelms))                             \
                l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else if (!(nelms % 2)) {                                    \
        for (i = total_count; i; i -= 2) {			\
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            if (!((i - 2) % nelms))                             \
                l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 1) {                                      \
        for (i = total_count; i; i--) {			        \
            *l_dest++ = *l_src;				        \
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else {                                                      \
        for (i = count; i; i--) {			        \
	    for (j = 0; j < nelms; j++)                         \
                *l_dest++ = l_src[j];				\
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
                                                                \
    doffset = ((MPI_Aint) l_dest - (MPI_Aint) dest);	        \
    soffset = ((MPI_Aint) l_src - (MPI_Aint) src);	        \
}

#define MPIDI_BUF_TO_VEC(src,dest,stride,type,nelms,count,soffset,doffset) \
{								\
    type * l_src = (type *)src, * l_dest = (type *)dest;	\
    register int i, j;		                                \
    unsigned long total_count = count * nelms;                  \
    const int l_stride = stride;				\
                                                                \
    if (!nelms) {                                               \
        for (i = count; i; i--)			                \
            l_dest = (type *) ((char *) l_dest + l_stride);	\
    }                                                           \
    else if (!(nelms % 8)) {			                \
        for (i = total_count; i; i -= 8) {			\
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest[4] = *l_src++;				\
            l_dest[5] = *l_src++;				\
            l_dest[6] = *l_src++;				\
            l_dest[7] = *l_src++;				\
            if (!((i - 8) % nelms))                             \
                l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }								\
    else if (!(nelms % 7)) {			                \
        for (i = total_count; i; i -= 7) {			\
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest[4] = *l_src++;				\
            l_dest[5] = *l_src++;				\
            l_dest[6] = *l_src++;				\
            if (!((i - 7) % nelms))                             \
                l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }								\
    else if (!(nelms % 6)) {			                \
        for (i = total_count; i; i -= 6) {			\
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest[4] = *l_src++;				\
            l_dest[5] = *l_src++;				\
            if (!((i - 6) % nelms))                             \
                l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }								\
    else if (!(nelms % 5)) {			                \
        for (i = total_count; i; i -= 5) {			\
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest[4] = *l_src++;				\
            if (!((i - 5) % nelms))                             \
                l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }								\
    else if (!(nelms % 4)) {			                \
        for (i = total_count; i; i -= 4) {			\
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            if (!((i - 4) % nelms))                             \
                l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }								\
    else if (!(nelms % 3)) {			                \
        for (i = total_count; i; i -= 3) {			\
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            if (!((i - 3) % nelms))                             \
                l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }								\
    else if (!(nelms % 2)) {			                \
        for (i = total_count; i; i -= 2) {			\
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            if (!((i - 2) % nelms))                             \
                l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }								\
    else if (nelms == 1) {                                      \
        for (i = total_count; i; i--) {			        \
            l_dest[i] = *l_src++;				\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
    else {                                                      \
        for (i = count; i; i--) {			        \
	    for (j = 0; j < nelms; j++)                         \
                l_dest[j] = *l_src++;				\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
                                                                \
    doffset = ((MPI_Aint) l_dest - (MPI_Aint) dest);	        \
    soffset = ((MPI_Aint) l_src - (MPI_Aint) src);	        \
}

#endif /* VECCPY_H */
