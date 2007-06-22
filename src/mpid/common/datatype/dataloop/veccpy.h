/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef VECCPY_H
#define VECCPY_H

#define MPIDI_VEC_TO_BUF(src,dest,stride,type,nelms,count,soffset,doffset) \
{								\
    type * l_src = (type *)src, * l_dest = (type *)dest;	\
    type * tmp_src = l_src, * tmp_dest = l_dest;                \
    register int i, j, k;		                        \
    unsigned long total_count = count * nelms;                  \
    const int l_stride = stride;				\
                                                                \
    if (!nelms) {                                               \
        for (i = count; i; i--)			                \
            l_src = (type *) ((char *) l_src + l_stride);	\
    }                                                           \
    else if (nelms == 1) {                                      \
        for (i = total_count; i; i--) {			        \
            *l_dest++ = *l_src;				        \
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 2) {                                      \
        for (i = total_count; i; i -= 2) {			\
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 3) {                                      \
        for (i = total_count; i; i -= 3) {			\
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 4) {                                      \
        for (i = total_count; i; i -= 4) {			\
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 5) {                                      \
        for (i = total_count; i; i -= 5) {			\
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            *l_dest++ = l_src[4];				\
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 6) {                                      \
        for (i = total_count; i; i -= 6) {			\
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            *l_dest++ = l_src[4];				\
            *l_dest++ = l_src[5];				\
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 7) {                                      \
        for (i = total_count; i; i -= 7) {			\
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            *l_dest++ = l_src[4];				\
            *l_dest++ = l_src[5];				\
            *l_dest++ = l_src[6];				\
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 8) {                                      \
        for (i = total_count; i; i -= 8) {			\
            *l_dest++ = l_src[0];				\
            *l_dest++ = l_src[1];				\
            *l_dest++ = l_src[2];				\
            *l_dest++ = l_src[3];				\
            *l_dest++ = l_src[4];				\
            *l_dest++ = l_src[5];				\
            *l_dest++ = l_src[6];				\
            *l_dest++ = l_src[7];				\
            l_src = (type *) ((char *) l_src + l_stride);	\
        }							\
    }                                                           \
    else {                                                      \
        i = total_count;                                        \
        j = nelms;                                              \
        while (i) {                                             \
            tmp_src = l_src;                                    \
            while (j > 8) {                                     \
                *l_dest++ = tmp_src[0];				\
                *l_dest++ = tmp_src[1];				\
                *l_dest++ = tmp_src[2];				\
                *l_dest++ = tmp_src[3];				\
                *l_dest++ = tmp_src[4];				\
                *l_dest++ = tmp_src[5];				\
                *l_dest++ = tmp_src[6];				\
                *l_dest++ = tmp_src[7];				\
                j -= 8;                                         \
                tmp_src += 8;                                   \
            }                                                   \
            for (k = 0; k < j; k++) {                           \
                *l_dest++ = *tmp_src++;                         \
            }                                                   \
            l_src = (type *) ((char *) l_src + l_stride);	\
            i -= nelms;                                         \
        }                                                       \
    }                                                           \
                                                                \
    doffset = ((MPI_Aint) l_dest - (MPI_Aint) dest);	        \
    soffset = ((MPI_Aint) l_src - (MPI_Aint) src);	        \
}

#define MPIDI_BUF_TO_VEC(src,dest,stride,type,nelms,count,soffset,doffset) \
{								\
    type * l_src = (type *)src, * l_dest = (type *)dest;	\
    type * tmp_src = l_src, * tmp_dest = l_dest;                \
    register int i, j, k;		                        \
    unsigned long total_count = count * nelms;                  \
    const int l_stride = stride;				\
                                                                \
    if (!nelms) {                                               \
        for (i = count; i; i--)			                \
            l_dest = (type *) ((char *) l_dest + l_stride);	\
    }                                                           \
    else if (nelms == 1) {                                      \
        for (i = total_count; i; i--) {			        \
            *l_dest = *l_src++;				        \
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 2) {                                      \
        for (i = total_count; i; i--) {			        \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 3) {                                      \
        for (i = total_count; i; i--) {			        \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 4) {                                      \
        for (i = total_count; i; i--) {			        \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 5) {                                      \
        for (i = total_count; i; i--) {			        \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest[4] = *l_src++;				\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 6) {                                      \
        for (i = total_count; i; i--) {			        \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest[4] = *l_src++;				\
            l_dest[5] = *l_src++;				\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 7) {                                      \
        for (i = total_count; i; i--) {			        \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest[4] = *l_src++;				\
            l_dest[5] = *l_src++;				\
            l_dest[6] = *l_src++;				\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
    else if (nelms == 8) {                                      \
        for (i = total_count; i; i--) {			        \
            l_dest[0] = *l_src++;				\
            l_dest[1] = *l_src++;				\
            l_dest[2] = *l_src++;				\
            l_dest[3] = *l_src++;				\
            l_dest[4] = *l_src++;				\
            l_dest[5] = *l_src++;				\
            l_dest[6] = *l_src++;				\
            l_dest[7] = *l_src++;				\
            l_dest = (type *) ((char *) l_dest + l_stride);	\
        }							\
    }                                                           \
    else {                                                      \
        i = total_count;                                        \
        j = nelms;                                              \
        while (i) {                                             \
            tmp_dest = l_dest;                                  \
            while (j > 8) {                                     \
                tmp_dest[0] = *l_src++;				\
                tmp_dest[1] = *l_src++;				\
                tmp_dest[2] = *l_src++;				\
                tmp_dest[3] = *l_src++;				\
                tmp_dest[4] = *l_src++;				\
                tmp_dest[5] = *l_src++;				\
                tmp_dest[6] = *l_src++;				\
                tmp_dest[7] = *l_src++;				\
                j -= 8;                                         \
                tmp_dest += 8;                                  \
            }                                                   \
            for (k = 0; k < j; k++) {                           \
                *tmp_dest++ = *l_src++;                         \
            }                                                   \
            l_dest = (type *) ((char *) l_dest + l_stride);	\
            i -= nelms;                                         \
        }                                                       \
    }                                                           \
                                                                \
    doffset = ((MPI_Aint) l_dest - (MPI_Aint) dest);	        \
    soffset = ((MPI_Aint) l_src - (MPI_Aint) src);	        \
}

#endif /* VECCPY_H */
