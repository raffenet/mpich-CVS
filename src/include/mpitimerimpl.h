#ifndef MPITIMERIMPL_H
#define MPITIMERIMPL_H

/* Routine tracing (see --enable-timing for control of this) */
#ifdef HAVE_TIMING

/* Possible values for timing */
#define MPID_TIMING_KIND_OFF 0
#define MPID_TIMING_KIND_TIME 1
#define MPID_TIMING_KIND_LOG 2
#define MPID_TIMING_KIND_ALL 3
#define MPID_TIMING_KIND_RUNTIME 4

/* These next two include files contain the static state definitions */
#include "mpistates.h"
#include "mpisysstates.h"

/* Possible values for USE_LOGGING */
#define MPID_LOGGING_RLOG 1
#define MPID_LOGGING_DLOG 2

/* Include the macros specific to the selected logging library */
#if (USE_LOGGING == MPID_LOGGING_RLOG)
#include "rlog_macros.h"
#elif (USE_LOGGING == MPID_LOGGING_DLOG)
#include "dlog_macros.h"
#else
#error You must select a logging library if timing is enabled
#endif

/* MPI layer definitions */
#define MPID_MPI_STATE_DECL                MPIDU_STATE_DECL
#define MPID_MPI_INIT_STATE_DECL           MPIDU_INIT_STATE_DECL
#define MPID_MPI_FINALIZE_STATE_DECL       MPIDU_FINALIZE_STATE_DECL

#define MPID_MPI_FUNC_ENTER                MPIDU_FUNC_ENTER
#define MPID_MPI_FUNC_EXIT                 MPIDU_FUNC_EXIT
#define MPID_MPI_PT2PT_FUNC_ENTER          MPIDU_PT2PT_FUNC_ENTER
#define MPID_MPI_PT2PT_FUNC_EXIT           MPIDU_PT2PT_FUNC_EXIT
#define MPID_MPI_COLL_FUNC_ENTER           MPIDU_COLL_FUNC_ENTER
#define MPID_MPI_COLL_FUNC_EXIT            MPIDU_COLL_FUNC_EXIT
#define MPID_MPI_INIT_FUNC_ENTER           MPIDU_INIT_FUNC_ENTER
#define MPID_MPI_INIT_FUNC_EXIT            MPIDU_INIT_FUNC_EXIT
#define MPID_MPI_FINALIZE_FUNC_ENTER       MPIDU_FINALIZE_FUNC_ENTER
#define MPID_MPI_FINALIZE_FUNC_EXIT        MPIDU_FINALIZE_FUNC_EXIT

#define LOG_ARROWS
#ifdef LOG_ARROWS
#define MPID_MPI_PT2PT_FUNC_ENTER_FRONT    MPIDU_PT2PT_FUNC_ENTER_FRONT
#define MPID_MPI_PT2PT_FUNC_ENTER_BACK     MPIDU_PT2PT_FUNC_ENTER_BACK
#define MPID_MPI_PT2PT_FUNC_ENTER_BOTH     MPIDU_PT2PT_FUNC_ENTER_BOTH
#else
#define MPID_MPI_PT2PT_FUNC_ENTER_FRONT    MPIDU_PT2PT_FUNC_ENTER
#define MPID_MPI_PT2PT_FUNC_ENTER_BACK     MPIDU_PT2PT_FUNC_ENTER
#define MPID_MPI_PT2PT_FUNC_ENTER_BOTH     MPIDU_PT2PT_FUNC_ENTER
#endif

/* device layer definitions */
#define MPIDI_STATE_DECL                MPIDU_STATE_DECL
#define MPIDI_INIT_STATE_DECL           MPIDU_INIT_STATE_DECL
#define MPIDI_FINALIZE_STATE_DECL       MPIDU_FINALIZE_STATE_DECL

#define MPIDI_FUNC_ENTER                MPIDU_FUNC_ENTER
#define MPIDI_FUNC_EXIT                 MPIDU_FUNC_EXIT
#define MPIDI_PT2PT_FUNC_ENTER          MPIDU_PT2PT_FUNC_ENTER
#define MPIDI_PT2PT_FUNC_EXIT           MPIDU_PT2PT_FUNC_EXIT
#define MPIDI_COLL_FUNC_ENTER           MPIDU_COLL_FUNC_ENTER
#define MPIDI_COLL_FUNC_EXIT            MPIDU_COLL_FUNC_EXIT
#define MPIDI_INIT_FUNC_ENTER           MPIDU_INIT_FUNC_ENTER
#define MPIDI_INIT_FUNC_EXIT            MPIDU_INIT_FUNC_EXIT
#define MPIDI_FINALIZE_FUNC_ENTER       MPIDU_FINALIZE_FUNC_ENTER
#define MPIDI_FINALIZE_FUNC_EXIT        MPIDU_FINALIZE_FUNC_EXIT

#define LOG_MPID_ARROWS
#ifdef LOG_MPID_ARROWS
#define MPIDI_PT2PT_FUNC_ENTER_FRONT    MPIDU_PT2PT_FUNC_ENTER_FRONT
#define MPIDI_PT2PT_FUNC_ENTER_BACK     MPIDU_PT2PT_FUNC_ENTER_BACK
#define MPIDI_PT2PT_FUNC_ENTER_BOTH     MPIDU_PT2PT_FUNC_ENTER_BOTH
#else
#define MPIDI_PT2PT_FUNC_ENTER_FRONT    MPIDU_PT2PT_FUNC_ENTER
#define MPIDI_PT2PT_FUNC_ENTER_BACK     MPIDU_PT2PT_FUNC_ENTER
#define MPIDI_PT2PT_FUNC_ENTER_BOTH     MPIDU_PT2PT_FUNC_ENTER
#endif

/* prototype the initialization/finalization functions */
int MPID_Timer_init(int rank, int size);
int MPID_Timer_finalize();

/* Statistics macros aren't defined yet */
/* All uses of these are protected by the symbol COLLECT_STATS, so they
   do not need to be defined in the non-HAVE_TIMING branch. */
#define MPID_STAT_BEGIN
#define MPID_STAT_END
#define MPID_STAT_ACC(statid,val)
#define MPID_STAT_ACC_RANGE(statid,rng)
#define MPID_STAT_ACC_SIMPLE(statid,val)
#define MPID_STAT_MISC(a) a

#else /* HAVE_TIMING */

/* evaporate all the timing macros if timing is not selected */
/* MPI layer */
#define MPID_Timer_init(rank, size)
#define MPID_Timer_finalize()
#define MPID_MPI_STATE_DECL(a)
#define MPID_MPI_INIT_STATE_DECL(a)
#define MPID_MPI_FINALIZE_STATE_DECL(a)
#define MPID_MPI_FUNC_EXIT(a)
#define MPID_MPI_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_FRONT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BACK(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BOTH(a)
#define MPID_MPI_PT2PT_FUNC_EXIT(a)
#define MPID_MPI_COLL_FUNC_ENTER(a)
#define MPID_MPI_COLL_FUNC_EXIT(a)
#define MPID_MPI_INIT_FUNC_ENTER(a)
#define MPID_MPI_INIT_FUNC_EXIT(a)
#define MPID_MPI_FINALIZE_FUNC_ENTER(a)
#define MPID_MPI_FINALIZE_FUNC_EXIT(a)
/* device layer */
#define MPIDI_STATE_DECL(a)
#define MPIDI_FUNC_ENTER(a)
#define MPIDI_FUNC_EXIT(a)
#define MPIDI_PT2PT_FUNC_ENTER(a)
#define MPIDI_PT2PT_FUNC_ENTER_FRONT(a)
#define MPIDI_PT2PT_FUNC_ENTER_BACK(a)
#define MPIDI_PT2PT_FUNC_ENTER_BOTH(a)
#define MPIDI_PT2PT_FUNC_EXIT(a)
#define MPIDI_COLL_FUNC_ENTER(a)
#define MPIDI_COLL_FUNC_EXIT(a)
#define MPIDI_INIT_FUNC_ENTER(a)
#define MPIDI_INIT_FUNC_EXIT(a)
#define MPIDI_FINALIZE_FUNC_ENTER(a)
#define MPIDI_FINALIZE_FUNC_EXIT(a)

#endif /* HAVE_TIMING */

#endif
