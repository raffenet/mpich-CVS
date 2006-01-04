#ifndef MPID_NEM_DEBUG_H
#define MPID_NEM_DEBUG_H
#include <pm.h>

/*#define NDEBUG 1 */
#include <assert.h>

/*#define MY_DEBUG*/
#ifdef MY_DEBUG
#define printf_d printf
#else
#define printf_d(x...) do { } while (0)
#endif

/*#define YIELD_IN_SKIP*/
#ifdef YIELD_IN_SKIP
#define SKIP sched_yield()
#warning "SKIP is sched_yield"
#else /* YIELD_IN_SKIP */
#define SKIP do{}while(0)
/*#warning "SKIP is do ...while" */
#endif /* YIELD_IN_SKIP */

/* #define HOLD_ON_ERROR */
#ifdef HOLD_ON_ERROR
#define MAYBE_HOLD_ON_ERROR(x) while(1)
#else /* HOLD_ON_ERROR */
#define MAYBE_HOLD_ON_ERROR(x) x
#endif /* HOLD_ON_ERROR */

#define MAX_ERR_STR_LEN 256
extern char MPID_nem_err_str[MAX_ERR_STR_LEN];

#define ERROR_RET(ret, err...) do {								\
    snprintf (MPID_nem_err_str, MAX_ERR_STR_LEN, err);						\
    printf ("%s ERROR (%s:%d) %s\n", MPID_nem_hostname, __FILE__, __LINE__, MPID_nem_err_str);	\
    MAYBE_HOLD_ON_ERROR (return ret);								\
} while (0)

#define PERROR_RET(ret, err...) do {			\
    snprintf (MPID_nem_err_str, MAX_ERR_STR_LEN, err);	\
    perror(MPID_nem_err_str);				\
    MAYBE_HOLD_ON_ERROR (return ret);			\
} while (0)

#define ERROR_GOTO(label, err...) do {								\
    snprintf (MPID_nem_err_str, MAX_ERR_STR_LEN, err);						\
    printf ("%s ERROR (%s:%d) %s\n", MPID_nem_hostname, __FILE__, __LINE__, MPID_nem_err_str);	\
    MAYBE_HOLD_ON_ERROR (goto label);								\
} while (0)

#define PERROR_GOTO(label, err...) do {			\
    snprintf (MPID_nem_err_str, MAX_ERR_STR_LEN, err);	\
    perror(MPID_nem_err_str);				\
    MAYBE_HOLD_ON_ERROR (goto label);			\
} while (0)

#define FATAL_ERROR(err...) do {									\
    snprintf (MPID_nem_err_str, MAX_ERR_STR_LEN, err);							\
    printf ("%s FATAL ERROR (%s:%d) %s\n", MPID_nem_hostname, __FILE__, __LINE__, MPID_nem_err_str);	\
    MAYBE_HOLD_ON_ERROR (exit (-1));									\
} while (0)

#define FATAL_PERROR(err...) do {			\
    snprintf (MPID_nem_err_str, MAX_ERR_STR_LEN, err);	\
    perror(MPID_nem_err_str);				\
    MAYBE_HOLD_ON_ERROR (exit (-1));			\
} while (0)

#endif /* MPID_NEM_DEBUG_H */
