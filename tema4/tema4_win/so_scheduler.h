#ifndef SO_SCHEDULER_H_
#define SO_SCHEDULER_H_

/* OS dependent stuff */
#ifdef __linux__
#include <pthread.h>

#define DECL_PREFIX

typedef pthread_t tid_t;
#elif defined(_WIN32)
#include <windows.h>

#ifdef DLL_IMPORTS
#define DECL_PREFIX __declspec(dllimport)
#else
#define DECL_PREFIX __declspec(dllexport)
#endif

typedef DWORD tid_t;
#else
#error "Unknown platform"
#endif

/* the maximum priority that can be assigned to a thread */
#define SO_MAX_PRIO 5
/* the maximum number of events */
#define SO_MAX_NUM_EVENTS 256

/* return value of failed tasks */
#define INVALID_TID ((tid_t)0)

#ifdef __cplusplus
extern "C" {
#endif

/* handler prototype */
typedef void (so_handler)(unsigned int);


DECL_PREFIX int so_init(unsigned int time_quantum, unsigned int io);

DECL_PREFIX tid_t so_fork(so_handler *func, unsigned int priority);

DECL_PREFIX int so_wait(unsigned int io);

DECL_PREFIX int so_signal(unsigned int io);

DECL_PREFIX void so_exec(void);
void so_schedule_thread(int caller);

DECL_PREFIX void so_end(void);

#ifdef __cplusplus
}
#endif

#endif /* SO_SCHEDULER_H_ */

