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

/* Include the macros specific to the selected logging library */
#ifdef USE_LOGGING_RLOG
#include "rlog_macros.h"
#elif defined(USE_LOGGING_DLOG)
#include "dlog_macros.h"
#else
#error You must select a logging library if you enable timing
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
#define MPID_Timer_init(rank, size)
#define MPID_Timer_finalize()
#define MPID_STATE_DECLS
#define MPID_MPI_STATE_DECLS
#define MPID_FUNC_ENTER(a)
#define MPID_FUNC_EXIT(a)
#define MPID_MPI_FINALIZE_STATE_DECLS
#define MPID_MPI_INIT_STATE_DECLS
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

#endif /* HAVE_TIMING */

#endif
