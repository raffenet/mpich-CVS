/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef ERRCODES_INCLUDED
#define ERRCODES_INCLUDED

/* Routine to return the error message for an error code.  Null on failure */
const char *MPIR_Err_get_string( int );

/* 
   This file contains the definitions of the error code fields
   
   An error code is organized as

   specific-msg-sequence# specific-msg-index generic-code is-dynamic? class

   where
   class: The MPI error class (including dynamically defined classes)
   is-dynamic?: Set if this is a dynamically created error code (using
   the routines to add error classes and codes at runtime)
   generic-code: Index into the array of generic messages
   specific-msg-index: index to the *buffer* containing recent 
   instance-specific error messages.
   specific-msg-sequence#: A sequence number used to check that the specific
   message text is still valid.
 */

#define ERROR_CLASS_MASK          0x000000FF
#define ERROR_DYN_MASK            0x00000100
#define ERROR_DYN_SHIFT           8
#define ERROR_GENERIC_MASK        0x0001FE00
#define ERROR_GENERIC_SHIFT       9
#define ERROR_SPECIFIC_INDEX_MASK 0x01FE0000
#define ERROR_SPECIFIC_INDEX_SHIFT 17
#define ERROR_SPECIFIC_SEQ_MASK   0x7E000000
/* Size is size of field as an integer, not the number of bits */
#define ERROR_SPECIFIC_SEQ_SIZE   64
#define ERROR_SPECIFIC_SEQ_SHIFT  25

#define ERROR_GET_CLASS( code ) ((code) & ERROR_CLASS_MASK)

/* These must correspond to the masks defined above */
#define ERROR_MAX_NCLASS 256
#define ERROR_MAX_NCODE  256
#endif
