#include "so_scheduler.h"
#include "queue.h"
#include "windows.h"
#include <stdlib.h>
#include <stdio.h>

#define WAITING 2
#define TERMINATED 1
#define NOT_TERMINATED 0


typedef struct scheduler {
	unsigned int time_quantum;
	unsigned int io;
	DWORD current_thread_id;
	HANDLE current_thread_handle;
	int current_thread_priority;
	int current_thread_time;
	HANDLE current_thread_mutex;
	HANDLE main_thread_mutex;
	Node * blocked_nodes[SO_MAX_NUM_EVENTS];
} scheduler_t;

typedef struct params {
	unsigned int priority;
	unsigned int time_quantum;
	HANDLE thread_mutex;
	HANDLE thread_handle;
	so_handler *func;
	BOOL finished;
} params_t;

scheduler_t *my_scheduler;

Node *my_pq;
Node *finished_threads;

int so_init(unsigned int time_quantum, unsigned int io)
{
	/* check if parameters are valid or scheduler has been allocated */
	if (time_quantum == 0 || io > SO_MAX_NUM_EVENTS ||
		my_scheduler != NULL)
		return -1;
	my_scheduler = malloc(sizeof(scheduler_t));
	if (my_scheduler == NULL) {
		perror("Calloc error on init");
		return -1;
	}
	my_scheduler->io = io;
	my_scheduler->current_thread_handle = INVALID_HANDLE_VALUE;
	my_scheduler->current_thread_id = -1;
	my_scheduler->current_thread_priority = -1;
	my_scheduler->current_thread_time = time_quantum;
	my_scheduler->main_thread_mutex = INVALID_HANDLE_VALUE;
	finished_threads = NULL;
	my_scheduler->time_quantum = time_quantum;
	return 0;
}

DWORD WINAPI start_thread(void *args)
{
	/* thread function */
	params_t *thread_args = (params_t *) args;

	WaitForSingleObject(thread_args->thread_handle, INFINITE);
	thread_args->func(thread_args->priority);
	ReleaseMutex(thread_args->thread_mutex);
	thread_args->finished = true;
	CloseHandle(thread_args->thread_mutex);
	free(thread_args->thread_mutex);
	/* add thread to finished_threads queue */
	if (finished_threads == NULL)
		finished_threads = newNode(GetCurrentThreadId(),
		thread_args->priority,
		thread_args->thread_mutex, thread_args->thread_handle);
	else
		push(&finished_threads, GetCurrentThreadId(),
		thread_args->priority,
		thread_args->thread_mutex, thread_args->thread_handle);

	so_schedule_thread(TERMINATED);
	free(args);
	return 0;
}

DWORD so_fork(so_handler *func, unsigned int priority)
{
	DWORD thread_id;
	HANDLE thread_mutex;
	HANDLE thread_handle;
	params_t *args;

	/* check for invalid parameters */
	if (func == NULL || priority > SO_MAX_PRIO)
		return INVALID_TID;

	/* subtract time needed for so_fork */
	if (my_scheduler->current_thread_id != -1)
		my_scheduler->current_thread_time--;

	/* lock the main thread until so_end */
	if (my_scheduler->main_thread_mutex == INVALID_HANDLE_VALUE) {
		my_scheduler->main_thread_mutex = CreateMutex(NULL,
		FALSE, NULL);
		WaitForSingleObject(my_scheduler->main_thread_mutex, INFINITE);
	}

	thread_mutex  = CreateMutex(NULL, FALSE, NULL);
	WaitForSingleObject(thread_mutex, INFINITE);
	args = malloc(sizeof(params_t));
	args->priority = priority;
	args->thread_mutex = thread_mutex;
	args->time_quantum = my_scheduler->time_quantum;
	args->func = func;

	args->finished = false;
	/* start thread and add it to pq */
	thread_handle = CreateThread(NULL, 0,
	(LPTHREAD_START_ROUTINE) start_thread,
	(void *) args, 0, &thread_id);

	WaitForSingleObject(thread_handle, INFINITE);
	args->thread_handle = thread_handle;

	if (my_pq == NULL)
		my_pq = newNode(thread_id, priority,
		thread_mutex, thread_handle);
	else
		push(&my_pq, thread_id, priority,
		thread_mutex, thread_handle);

	if (my_scheduler->current_thread_id == -1)
		so_schedule_thread(NOT_TERMINATED);
	return thread_id;
}

int so_wait(unsigned int io)
{
	/* check for invalid io */
	if ((int) io >= my_scheduler->io)
		return -1;
	return 0;
}

int so_signal(unsigned int io)
{
	/* check for invalid io */
	int num_threads = 0;

	if ((int) io >= my_scheduler->io)
		return -1;
	return num_threads;
}

void so_exec(void)
{
	/* check if running thread is listed */
	if (my_scheduler->current_thread_id == -1)
		perror("not running a thread\n");

	my_scheduler->current_thread_time--;
	so_schedule_thread(WAITING);
}

void so_end(void)
{
	Node *head;
	DWORD rc;

	if (my_scheduler == NULL)
		return;

	/* block main thread until all forked threads finished */
	if (my_scheduler->main_thread_mutex != INVALID_HANDLE_VALUE) {
		my_scheduler->main_thread_mutex = CreateMutex(NULL,
		FALSE, NULL);
		rc = WaitForSingleObject(my_scheduler->main_thread_mutex,
		INFINITE);
		if (!isEmpty(&my_pq))
			exit(EXIT_FAILURE);
	}

	/* join threads */
	while (!isEmpty(&finished_threads)) {
		head = peek(&finished_threads);
		WaitForSingleObject(head->thread_handle, INFINITE);
		CloseHandle(head->thread_handle);
		pop(&finished_threads);
	}

	/* unlock main thread */
	if (my_scheduler->main_thread_mutex) {
		ReleaseMutex(my_scheduler->main_thread_mutex);
		CloseHandle(my_scheduler->main_thread_mutex);
	}
	freeQueue(&my_pq);
	freeQueue(&finished_threads);
	free(my_scheduler);
	my_scheduler = NULL;
}

void so_schedule_thread(int caller)
{
	/* if is the first call from the main thread */
	if (caller == NOT_TERMINATED)
		if (my_scheduler->current_thread_id == -1)
			if (!isEmpty(&my_pq)) {
				Node *head = my_pq;

				/* choose the element with highest priority */
				my_scheduler->current_thread_id =
				head->thread_id;
				my_scheduler->current_thread_priority =
				head->priority;
				my_scheduler->current_thread_mutex =
				head->mutex;
				my_scheduler->current_thread_handle =
				head->thread_handle;
				my_scheduler->current_thread_time =
				my_scheduler->time_quantum;
				pop(&my_pq);
				ReleaseMutex(
					my_scheduler->current_thread_mutex);
			}
/* if caller has finished its work, the next best thread should be scheduled */
	else if (caller == TERMINATED) {
		if (isEmpty(&my_pq)) {
			ReleaseMutex(my_scheduler->main_thread_mutex);
			my_scheduler->current_thread_id = -1;
		} else {
			Node *head = my_pq;

/* choose the element with the highest priority from queue*/
			my_scheduler->current_thread_id =
			head->thread_id;
			my_scheduler->current_thread_priority =
			head->priority;
			my_scheduler->current_thread_mutex =
			head->mutex;
			my_scheduler->current_thread_time =
			my_scheduler->time_quantum;
			my_scheduler->current_thread_handle =
			head->thread_handle;
			pop(&my_pq);
			ReleaseMutex(my_scheduler->current_thread_mutex);
		}
	}
	/* if quantum is finished, reset it */
	else if (caller == WAITING)
		if (my_scheduler->current_thread_time == 0)
			if (isEmpty(&my_pq))
				my_scheduler->current_thread_time =
				my_scheduler->time_quantum;
}
