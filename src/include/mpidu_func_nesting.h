/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIDU_FUNC_NESTING_H_INCLUDED)
#define MPIDU_FUNC_NESTING_H_INCLUDED

/* state declaration macros */
#define MPIR_STATE_DECL(a)		int a##_nest_level_in
#define MPID_MPI_STATE_DECL(a)		MPIR_STATE_DECL(a)
#define MPID_MPI_INIT_STATE_DECL(a)	MPIR_STATE_DECL(a)
#define MPID_MPI_FINALIZE_STATE_DECL(a)	MPIR_STATE_DECL(a)
#define MPIDI_STATE_DECL(a)		MPIR_STATE_DECL(a)
#define MPIDI_INIT_STATE_DECL(a)	MPIR_STATE_DECL(a)
#define MPIDI_FINALIZE_STATE_DECL(a)	MPIR_STATE_DECL(a)

/* function enter and exit macros */		\
#define MPIR_FUNC_ENTER(a)			\
{						\
    a##_nest_level_in = MPIR_Nest_value();	\
}

#define MPIR_FUNC_EXIT(a)										\
{													\
    int nest_level_out = MPIR_Nest_value();								\
													\
    if (a##_nest_level_in != nest_level_out)								\
    {													\
	MPIU_Error_printf("Error in nesting level: file=%s, line=%d, nest_in=%d, nest_out=%d\n",	\
			  __FILE__, __LINE__, a##_nest_level_in, nest_level_out);			\
	exit(1);											\
    }													\
}

/* mpich layer definitions */
#define MPID_MPI_FUNC_ENTER(a)			MPIR_FUNC_ENTER(a)
#define MPID_MPI_FUNC_EXIT(a)			MPIR_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_FRONT(a)	MPIR_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_FRONT(a)	MPIR_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BACK(a)	MPIR_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BOTH(a)	MPIR_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BACK(a)	MPIR_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BOTH(a)	MPIR_FUNC_EXIT(a)
#define MPID_MPI_COLL_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPID_MPI_COLL_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)
#define MPID_MPI_RMA_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPID_MPI_RMA_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)
#define MPID_MPI_INIT_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPID_MPI_INIT_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)
#define MPID_MPI_FINALIZE_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPID_MPI_FINALIZE_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)

/* device layer definitions */
#define MPIDI_FUNC_ENTER(a)			MPIR_FUNC_ENTER(a)
#define MPIDI_FUNC_EXIT(a)			MPIR_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_ENTER_FRONT(a)		MPIR_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_EXIT_FRONT(a)		MPIR_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_ENTER_BACK(a)		MPIR_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_ENTER_BOTH(a)		MPIR_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_EXIT_BACK(a)		MPIR_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_EXIT_BOTH(a)		MPIR_FUNC_EXIT(a)
#define MPIDI_COLL_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPIDI_COLL_FUNC_EXIT(a)			MPIR_FUNC_EXIT(a)
#define MPIDI_RMA_FUNC_ENTER(a)			MPIR_FUNC_ENTER(a)
#define MPIDI_RMA_FUNC_EXIT(a)			MPIR_FUNC_EXIT(a)
#define MPIDI_INIT_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPIDI_INIT_FUNC_EXIT(a)			MPIR_FUNC_EXIT(a)
#define MPIDI_FINALIZE_FUNC_ENTER(a)		MPIR_FUNC_ENTER(a)
#define MPIDI_FINALIZE_FUNC_EXIT(a)		MPIR_FUNC_EXIT(a)

/* evaporate the timing macros since timing is not selected */
#define MPIU_Timer_init(rank, size)
#define MPIU_Timer_finalize()

#endif /* defined(MPIDU_FUNC_NESTING_H_INCLUDED) */
