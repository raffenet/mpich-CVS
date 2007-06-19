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
    register int i, j;		                                \
    unsigned long total_count = count * nelms;                  \
    const int l_stride = stride;				\
                                                                \
    if (!nelms) {                                               \
        for (i = count; i; i--)			                \
            l_src = (type *) ((char *) l_src + l_stride);	\
    }                                                           \
    else if (!(nelms % 8)) {			                \
        for (i = total_count; i; i -= 8) {			\
            *l_dest++ = tmp_src[0];				\
            *l_dest++ = tmp_src[1];				\
            *l_dest++ = tmp_src[2];				\
            *l_dest++ = tmp_src[3];				\
            *l_dest++ = tmp_src[4];				\
            *l_dest++ = tmp_src[5];				\
            *l_dest++ = tmp_src[6];				\
            *l_dest++ = tmp_src[7];				\
            tmp_src += 8;                                       \
            if (!((total_count - i + 8) % nelms)) {             \
                l_src = (type *) ((char *) l_src + l_stride);	\
                tmp_src = l_src;                                \
            }                                                   \
        }							\
    }								\
    else if (!(nelms % 7)) {			                \
        for (i = total_count; i; i -= 7) {			\
            *l_dest++ = tmp_src[0];				\
            *l_dest++ = tmp_src[1];				\
            *l_dest++ = tmp_src[2];				\
            *l_dest++ = tmp_src[3];				\
            *l_dest++ = tmp_src[4];				\
            *l_dest++ = tmp_src[5];				\
            *l_dest++ = tmp_src[6];				\
            tmp_src += 7;                                       \
            if (!((total_count - i + 7) % nelms)) {             \
                l_src = (type *) ((char *) l_src + l_stride);	\
                tmp_src = l_src;                                \
            }                                                   \
        }							\
    }								\
    else if (!(nelms % 6)) {			                \
        for (i = total_count; i; i -= 6) {			\
            *l_dest++ = tmp_src[0];				\
            *l_dest++ = tmp_src[1];				\
            *l_dest++ = tmp_src[2];				\
            *l_dest++ = tmp_src[3];				\
            *l_dest++ = tmp_src[4];				\
            *l_dest++ = tmp_src[5];				\
            tmp_src += 6;                                       \
            if (!((total_count - i + 6) % nelms)) {             \
                l_src = (type *) ((char *) l_src + l_stride);	\
                tmp_src = l_src;                                \
            }                                                   \
        }							\
    }								\
    else if (!(nelms % 5)) {			                \
        for (i = total_count; i; i -= 5) {			\
            *l_dest++ = tmp_src[0];				\
            *l_dest++ = tmp_src[1];				\
            *l_dest++ = tmp_src[2];				\
            *l_dest++ = tmp_src[3];				\
            *l_dest++ = tmp_src[4];				\
            tmp_src += 5;                                       \
            if (!((total_count - i + 5) % nelms)) {             \
                l_src = (type *) ((char *) l_src + l_stride);	\
                tmp_src = l_src;                                \
            }                                                   \
        }							\
    }								\
    else if (!(nelms % 4)) {                                    \
        for (i = total_count; i; i -= 4) {			\
            *l_dest++ = tmp_src[0];				\
            *l_dest++ = tmp_src[1];				\
            *l_dest++ = tmp_src[2];				\
            *l_dest++ = tmp_src[3];				\
            tmp_src += 4;                                       \
            if (!((total_count - i + 4) % nelms)) {             \
                l_src = (type *) ((char *) l_src + l_stride);	\
                tmp_src = l_src;                                \
            }                                                   \
        }							\
    }                                                           \
    else if (!(nelms % 3)) {                                    \
        for (i = total_count; i; i -= 3) {			\
            *l_dest++ = tmp_src[0];				\
            *l_dest++ = tmp_src[1];				\
            *l_dest++ = tmp_src[2];				\
            tmp_src += 3;                                       \
            if (!((total_count - i + 3) % nelms)) {             \
                l_src = (type *) ((char *) l_src + l_stride);	\
                tmp_src = l_src;                                \
            }                                                   \
        }							\
    }                                                           \
    else if (!(nelms % 2)) {                                    \
        for (i = total_count; i; i -= 2) {			\
            *l_dest++ = tmp_src[0];				\
            *l_dest++ = tmp_src[1];				\
            tmp_src += 2;                                       \
            if (!((total_count - i + 2) % nelms)) {             \
                l_src = (type *) ((char *) l_src + l_stride);	\
                tmp_src = l_src;                                \
            }                                                   \
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
    type * tmp_src = l_src, * tmp_dest = l_dest;                \
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
            tmp_dest[0] = *l_src++;				\
            tmp_dest[1] = *l_src++;				\
            tmp_dest[2] = *l_src++;				\
            tmp_dest[3] = *l_src++;				\
            tmp_dest[4] = *l_src++;				\
            tmp_dest[5] = *l_src++;				\
            tmp_dest[6] = *l_src++;				\
            tmp_dest[7] = *l_src++;				\
            tmp_src += 8;                                       \
            if (!((total_count - i + 8) % nelms)) {             \
                l_dest = (type *) ((char *) l_dest + l_stride);	\
                tmp_dest = l_dest;                              \
            }                                                   \
        }							\
    }								\
    else if (!(nelms % 7)) {			                \
        for (i = total_count; i; i -= 7) {			\
            tmp_dest[0] = *l_src++;				\
            tmp_dest[1] = *l_src++;				\
            tmp_dest[2] = *l_src++;				\
            tmp_dest[3] = *l_src++;				\
            tmp_dest[4] = *l_src++;				\
            tmp_dest[5] = *l_src++;				\
            tmp_dest[6] = *l_src++;				\
            tmp_src += 7;                                       \
            if (!((total_count - i + 7) % nelms)) {             \
                l_dest = (type *) ((char *) l_dest + l_stride);	\
                tmp_dest = l_dest;                              \
            }                                                   \
        }							\
    }								\
    else if (!(nelms % 6)) {			                \
        for (i = total_count; i; i -= 6) {			\
            tmp_dest[0] = *l_src++;				\
            tmp_dest[1] = *l_src++;				\
            tmp_dest[2] = *l_src++;				\
            tmp_dest[3] = *l_src++;				\
            tmp_dest[4] = *l_src++;				\
            tmp_dest[5] = *l_src++;				\
            tmp_src += 6;                                       \
            if (!((total_count - i + 6) % nelms)) {             \
                l_dest = (type *) ((char *) l_dest + l_stride);	\
                tmp_dest = l_dest;                              \
            }                                                   \
        }							\
    }								\
    else if (!(nelms % 5)) {			                \
        for (i = total_count; i; i -= 5) {			\
            tmp_dest[0] = *l_src++;				\
            tmp_dest[1] = *l_src++;				\
            tmp_dest[2] = *l_src++;				\
            tmp_dest[3] = *l_src++;				\
            tmp_dest[4] = *l_src++;				\
            tmp_src += 5;                                       \
            if (!((total_count - i + 5) % nelms)) {             \
                l_dest = (type *) ((char *) l_dest + l_stride);	\
                tmp_dest = l_dest;                              \
            }                                                   \
        }							\
    }								\
    else if (!(nelms % 4)) {			                \
        for (i = total_count; i; i -= 4) {			\
            tmp_dest[0] = *l_src++;				\
            tmp_dest[1] = *l_src++;				\
            tmp_dest[2] = *l_src++;				\
            tmp_dest[3] = *l_src++;				\
            tmp_src += 4;                                       \
            if (!((total_count - i + 4) % nelms)) {             \
                l_dest = (type *) ((char *) l_dest + l_stride);	\
                tmp_dest = l_dest;                              \
            }                                                   \
        }							\
    }								\
    else if (!(nelms % 3)) {			                \
        for (i = total_count; i; i -= 3) {			\
            tmp_dest[0] = *l_src++;				\
            tmp_dest[1] = *l_src++;				\
            tmp_dest[2] = *l_src++;				\
            tmp_src += 3;                                       \
            if (!((total_count - i + 3) % nelms)) {             \
                l_dest = (type *) ((char *) l_dest + l_stride);	\
                tmp_dest = l_dest;                              \
            }                                                   \
        }							\
    }								\
    else if (!(nelms % 2)) {			                \
        for (i = total_count; i; i -= 2) {			\
            tmp_dest[0] = *l_src++;				\
            tmp_dest[1] = *l_src++;				\
            tmp_src += 2;                                       \
            if (!((total_count - i + 2) % nelms)) {             \
                l_dest = (type *) ((char *) l_dest + l_stride);	\
                tmp_dest = l_dest;                              \
            }                                                   \
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
