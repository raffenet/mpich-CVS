/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIERRS_H_INCLUDED
#define MPIERRS_H_INCLUDED
/* ------------------------------------------------------------------------- */
/* mpierrs.h */
/* ------------------------------------------------------------------------- */

/* Error checking (see --enable-error-checking for control of this) */
#ifdef HAVE_ERROR_CHECKING

#define MPID_ERROR_LEVEL_ALL 1
#define MPID_ERROR_LEVEL_RUNTIME 2
/* Use MPID_ERROR_DECL to wrap declarations that are needed only when
   error checking is turned on */
#define MPID_ERROR_DECL(a) a

#if HAVE_ERROR_CHECKING == MPID_ERROR_LEVEL_ALL
#define MPID_BEGIN_ERROR_CHECKS
#define MPID_END_ERROR_CHECKS
#define MPID_ELSE_ERROR_CHECKS
#elif HAVE_ERROR_CHECKING == MPID_ERROR_LEVEL_RUNTIME
#define MPID_BEGIN_ERROR_CHECKS if (MPIR_Process.do_error_checks) {
#define MPID_ELSE_ERROR_CHECKS }else{
#define MPID_END_ERROR_CHECKS }
#else
#error "Unknown value for error checking"
#endif

#else
#define MPID_BEGIN_ERROR_CHECKS
#define MPID_END_ERROR_CHECKS
#define MPID_ERROR_DECL(a)
#endif /* HAVE_ERROR_CHECKING */

/* 
 *  Standardized error checking macros.  These provide the correct tests for
 *  common tests.  These set err with the encoded error value.
 */
#define MPIR_ERRTEST_INITIALIZED(err) \
  if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {\
      err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**initialized", 0 ); }
#define MPIR_ERRTEST_SEND_TAG(tag,err) \
  if ((tag) < 0 || (tag) > MPIR_Process.attrs.tag_ub) {\
      err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TAG, "**tag", "**tag %d", tag);}
#define MPIR_ERRTEST_RECV_TAG(tag,err) \
  if ((tag) < MPI_ANY_TAG || (tag) > MPIR_Process.attrs.tag_ub) {\
      err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TAG, "**tag", "**tag %d", tag );}
#define MPIR_ERRTEST_SEND_RANK(comm_ptr,rank,err) \
  if ((rank) < MPI_PROC_NULL || (rank) >= (comm_ptr)->remote_size) {\
      err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_RANK, "**rank", \
                                  "**rank %d %d", rank, (comm_ptr)->remote_size );}
#define MPIR_ERRTEST_RECV_RANK(comm_ptr,rank,err) \
  if ((rank) < MPI_ANY_SOURCE || (rank) >= (comm_ptr)->remote_size) {\
      err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_RANK, "**rank", \
                                  "**rank %d %d", rank, (comm_ptr)->remote_size );}
#define MPIR_ERRTEST_COUNT(count,err) \
    if ((count) < 0) {\
        err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_COUNT, "**countneg", \
                                    "**countneg %d", count );}
#define MPIR_ERRTEST_DISP(disp,err) \
    if ((disp) < 0) {\
        err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_DISP, "**rmadisp", 0 );}
#define MPIR_ERRTEST_ALIAS(ptr1,ptr2,err) \
    if ((ptr1)==(ptr2) && (ptr1) != MPI_BOTTOM) {\
        err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_BUFFER, "**bufalias", 0 );}
#define MPIR_ERRTEST_ARGNULL(arg,arg_name,err) \
   if (!(arg)) {\
       err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG, "**nullptr", \
                                   "**nullptr %s", arg_name ); } 
#define MPIR_ERRTEST_ARGNEG(arg,arg_name,err) \
   if ((arg) < 0) {\
       err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG, "**argneg", \
                                   "**argneg %s %d", arg_name, arg ); }
#define MPIR_ERRTEST_ARGNONPOS(arg,arg_name,err) \
   if ((arg) <= 0) {\
       err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG, "**argnonpos", \
                                   "**argnonpos %s %d", arg_name, arg ); }
#define MPIR_ERRTEST_DATATYPE_NULL(arg,arg_name,err) \
   if ((arg) == MPI_DATATYPE_NULL) {\
       err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE, "**dtypenull", "**dtypenull %s", arg_name ); }
/* An intracommunicator must have a root between 0 and local_size-1. */
/* intercomm can be between MPI_PROC_NULL (or MPI_ROOT) and local_size-1 */
#define MPIR_ERRTEST_INTRA_ROOT(comm_ptr,root,err) \
  if ((root) < 0 || (root) >= (comm_ptr)->local_size) {\
      err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ROOT, "**root", "**root %d", root );}
/* We use -2 (MPI_PROC_NULL and MPI_ROOT are negative) for the intercomm 
   test */
#define MPIR_ERRTEST_INTER_ROOT(comm_ptr,root,err) \
  if ((root) < -3 || (root) >= (comm_ptr)->local_size) {\
      err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ROOT, "**root", "**root %d", root );}
#define MPIR_ERRTEST_PERSISTENT(reqp,err) \
  if ((reqp)->kind != MPID_PREQUEST_SEND && reqp->kind != MPID_PREQUEST_RECV) { \
      err = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_REQUEST, "**requestnotpersist", 0 ); }
#define MPIR_ERRTEST_PERSISTENT_ACTIVE(reqp,err) \
  if (((reqp)->kind == MPID_PREQUEST_SEND || \
      reqp->kind == MPID_PREQUEST_RECV) && reqp->partner_request != NULL) { \
      err = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_REQUEST, "**requestpersistactive", 0 ); }
#define MPIR_ERRTEST_COMM_INTRA(comm_ptr, err ) \
    if ((comm_ptr)->comm_kind != MPID_INTRACOMM) {\
       err = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_COMM,"**commnotintra",0);}

#define MPIR_ERRTEST_DATATYPE(count, datatype,err)									\
{															\
    if (HANDLE_GET_MPI_KIND(datatype) != MPID_DATATYPE || (HANDLE_GET_KIND(datatype) == HANDLE_KIND_INVALID &&		\
	datatype != MPI_DATATYPE_NULL))											\
    {															\
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE, "**dtype", 0 );	\
    }															\
    if (count > 0 && datatype == MPI_DATATYPE_NULL)									\
    {															\
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE, "**dtypenull", 0 );	\
    }															\
}

#define MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf,count,err) \
  if (count > 0 && sendbuf == MPI_IN_PLACE) {\
      err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_BUFFER, "**sendbuf_inplace", 0 );}

#define MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf,count,err) \
  if (count > 0 && recvbuf == MPI_IN_PLACE) {\
      err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_BUFFER, "**recvbuf_inplace", 0 );}

#define MPIR_ERRTEST_BUF_INPLACE(buf,count,err) \
  if (count > 0 && buf == MPI_IN_PLACE) {\
      err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_BUFFER, "**buf_inplace", 0 );}

/*
 * Check that the triple (buf,count,datatype) does not specify a null
 * buffer.  This does not guarantee that the buffer is valid but does
 * catch the most common problems.
 * Question:
 * Should this be an (inlineable) routine?  
 * Since it involves extracting the datatype pointer for non-builtin
 * datatypes, should it take a dtypeptr argument (valid only if not
 * builtin)?
 */
#define MPIR_ERRTEST_USERBUFFER(buf,count,dtype,err)									\
    if (count > 0 && buf == 0) {											\
        int ferr = 0;													\
        if (HANDLE_GET_KIND(dtype) == HANDLE_KIND_BUILTIN) { ferr=1; }							\
        else {														\
            MPID_Datatype *errdtypeptr;											\
            MPID_Datatype_get_ptr(dtype,errdtypeptr);									\
            if (errdtypeptr && errdtypeptr->true_lb == 0) { ferr=1; }							\
        }														\
        if (ferr) {													\
            err = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_BUFFER, "**bufnull", 0 );}	\
    }
/* The following are placeholders.  We haven't decided yet whether these
   should take a handle or pointer, or if they should take a handle and return 
   a pointer if the handle is valid.  These need to be rationalized with the
   MPID_xxx_valid_ptr and MPID_xxx_get_ptr.

   [BRT] They should not take a handle and return a pointer if they will be
   placed inside of a #ifdef HAVE_ERROR_CHECKING block.  Personally, I think
   the macros should take handles.  We already have macros for validating
   pointers to various objects.
*/
/* --BEGIN ERROR MACROS-- */
#define MPIR_ERRTEST_VALID_HANDLE(handle_,kind_,err_,errclass_,gmsg_) {\
    if (HANDLE_GET_MPI_KIND(handle_) != kind_ || \
        HANDLE_GET_KIND(handle_) == HANDLE_KIND_INVALID) {\
        MPIU_ERR_SETANDSTMT(err_,errclass_,goto fn_fail,gmsg_);\
    }\
}
/* --END ERROR MACROS-- */

#define MPIR_ERRTEST_OP(op,err) if (op == MPI_OP_NULL) {\
    MPIU_ERR_SETANDSTMT(err,MPI_ERR_OP,goto fn_fail,"**opnull");\
}else { MPIR_ERRTEST_VALID_HANDLE(op,MPID_OP,err,MPI_ERR_OP,"**op");}

#define MPIR_ERRTEST_GROUP(group,err) if (group == MPI_GROUP_NULL) {\
    MPIU_ERR_SETANDSTMT(err,MPI_ERR_GROUP,goto fn_fail,"**groupnull");\
}else { MPIR_ERRTEST_VALID_HANDLE(group,MPID_GROUP,err,MPI_ERR_GROUP,"**group");}

#define MPIR_ERRTEST_COMM(comm,err)											\
{															\
    if (HANDLE_GET_MPI_KIND(comm) != MPID_COMM ||									\
	(HANDLE_GET_KIND(comm) == HANDLE_KIND_INVALID &&								\
	 comm != MPI_COMM_NULL))											\
    {															\
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_COMM, "**comm", 0 );	\
    }															\
    if (comm == MPI_COMM_NULL)												\
    {															\
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_COMM, "**commnull", 0 );	\
    }															\
}
#define MPIR_ERRTEST_REQUEST(request,err)
#define MPIR_ERRTEST_ERRHANDLER(errhandler,err) if (errhandler == MPI_ERRHANDLER_NULL) {\
    MPIU_ERR_SETANDSTMT(err,MPI_ERR_OTHER,,"**errhandlernull");\
}else { MPIR_ERRTEST_VALID_HANDLE(errhandler,MPID_ERRHANDLER,err,MPI_ERR_OTHER,"**errhandler");}

/* Special MPI error "class/code" for out of memory */
/* FIXME: not yet done */
#define MPIR_ERR_MEMALLOCFAILED MPI_ERR_INTERN

/* 
 * Standardized error setting and checking macros
 * These are intended to simplify the insertion of standardized error
 * checks
 *
 */
/* --BEGIN ERROR MACROS-- */
#ifdef HAVE_ERROR_CHECKING
#define MPIU_ERR_SETANDSTMT(err_,class_,stmt_,msg_) \
    {err_ = MPIR_Err_create_code( err_,MPIR_ERR_RECOVERABLE,FCNAME,\
           __LINE__, class_, msg_, 0 ); stmt_ ;}
#define MPIU_ERR_SETANDSTMT1(err_,class_,stmt_,gmsg_,smsg_,arg1_) \
    {err_ = MPIR_Err_create_code( err_,MPIR_ERR_RECOVERABLE,FCNAME,\
           __LINE__, class_, gmsg_, smsg_, arg1_ ); stmt_ ;}
#define MPIU_ERR_SETFATALANDSTMT(err_,class_,stmt_,msg_) \
    {err_ = MPIR_Err_create_code( err_,MPIR_ERR_FATAL,FCNAME,\
           __LINE__, class_, msg_, 0 ); stmt_ ;}
#define MPIU_ERR_SETFATALANDSTMT1(err_,class_,stmt_,gmsg_,smsg_,arg1_) \
    {err_ = MPIR_Err_create_code( err_,MPIR_ERR_FATAL,FCNAME,\
           __LINE__, class_, gmsg_, smsg_, arg1_ ); stmt_ ;}
#else
#define MPIU_ERR_SETANDSTMT(err_,class_,stmt_,msg_) \
     {if (!err_){err_=class_;} stmt_;}
#define MPIU_ERR_SETANDSTMT1(err_,class_,stmt_,gmsg_,smsg_,arg1_) \
     {if(!err_){err_=class_;}stmt_;}
#define MPIU_ERR_SETFATALANDSTMT(err_,class_,stmt_,msg_) \
     {if(!err_){err_=class_;} stmt_;}
#define MPIU_ERR_SETFATALANDSTMT1(err_,class_,stmt_,gmsg_,smsg_,arg1_) \
     {if(!err_){err_=class_;}stmt_;}
#endif

/* The following definitions are the same independent of the choice of 
   HAVE_ERROR_CHECKING */
#define MPIU_ERR_SETANDJUMP(err_,class_,msg_) \
     MPIU_ERR_SETANDSTMT(err_,class_,goto fn_fail,msg_)
#define MPIU_ERR_CHKANDSTMT(cond_,err_,class_,stmt_,msg_) \
    {if (cond_) { MPIU_ERR_SETANDSTMT(err_,class_,stmt_,msg_); }}
#define MPIU_ERR_CHKANDJUMP(cond_,err_,class_,msg_) \
     MPIU_ERR_CHKANDSTMT(cond_,err_,class_,goto fn_fail,msg_)

#define MPIU_ERR_SETANDJUMP1(err_,class_,gmsg_,smsg_,arg1_) \
     MPIU_ERR_SETANDSTMT1(err_,class_,goto fn_fail,gmsg_,smsg_,arg1_)
#define MPIU_ERR_CHKANDSTMT1(cond_,err_,class_,stmt_,gmsg_,smsg_,arg1_) \
    {if (cond_) { MPIU_ERR_SETANDSTMT1(err_,class_,stmt_,gmsg_,smsg_,arg1_); }}
#define MPIU_ERR_CHKANDJUMP1(cond_,err_,class_,gmsg_,smsg_,arg1_) \
     MPIU_ERR_CHKANDSTMT1(cond_,err_,class_,goto fn_fail,gmsg_,smsg_,arg1_)
/* --END ERROR MACROS-- */

/* 
 * Special case for "is initialized".  
 * This should be used in cases where there is no
 * additional error checking
 */
#ifdef HAVE_ERROR_CHECKING
#define MPIR_ERRTEST_INITIALIZED_ORRETURN()											\
{																\
    if (MPIR_Process.initialized != MPICH_WITHIN_MPI)										\
    {																\
	return MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**initialized", 0 );	\
    }																\
}
#else
#define MPIR_ERRTEST_INITIALIZED_ORRETURN(err_) {return MPI_ERR_OTHER;}
#endif
#define MPIR_ERRTEST_INITIALIZED_FIRSTORJUMP MPIR_ERRTEST_INITIALIZED_ORRETURN()

/* ------------------------------------------------------------------------- */
/* end of mpierrs.h */
/* ------------------------------------------------------------------------- */

#endif
