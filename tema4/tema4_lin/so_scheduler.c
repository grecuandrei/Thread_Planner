#include "so_scheduler.h"
#include "semaphore.h"
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

#define WAITING 2
#define TERMINATED 1
#define NOT_TERMINATED 0


typedef struct scheduler {
	unsigned int time_quantum;
	unsigned int io;
	pthread_t current_thread_id;
	int current_thread_priority;
	int current_thread_time;
	pthread_mutex_t *current_thread_mutex;
	pthread_mutex_t *main_thread_mutex;
} scheduler_t;

typedef struct params {
	unsigned int priority;
	unsigned int time_quantum;
	pthread_mutex_t *thread_mutex;
	so_handler *func;
	bool finished;
} params_t;

scheduler_t *my_scheduler;

Node *my_pq;
Node *finished_threads;

int so_init(unsigned int time_quantum, unsigned int io)
{
	/* check if parameters are valid or scheduler has been allocated */
	if (time_quantum == 0 || io > SO_MAX_NUM_EVENTS || my_scheduler != NULL)
		return -1;
	my_scheduler = malloc(sizeof(scheduler_t));
	if (my_scheduler == NULL) {
		perror("Calloc error on init");
		return -1;
	}
	my_scheduler->io = io;
	my_scheduler->current_thread_id = -1;
	my_scheduler->current_thread_time = time_quantum;
	my_scheduler->main_thread_mutex = NULL;
	finished_threads = NULL;
	my_scheduler->time_quantum = time_quantum;
	return 0;
}

void *start_thread(void *args)
{
	/* thread function */
	params_t *thread_args = args;

	pthread_mutex_lock(thread_args->thread_mutex);
	thread_args->func(thread_args->priority);
	pthread_mutex_unlock(thread_args->thread_mutex);
	thread_args->finished = true;
	pthread_mutex_destroy(thread_args->thread_mutex);
	free(thread_args->thread_mutex);
	/* add thread to finished_threads queue */
	if (finished_threads == NULL)
		finished_threads =
		newNode(pthread_self(),
		thread_args->priority,
		thread_args->thread_mutex);
	else
		push(&finished_threads,
			pthread_self(),
			thread_args->priority,
			thread_args->thread_mutex);

	so_schedule_thread(TERMINATED);
	pthread_exit(args);
}

tid_t so_fork(so_handler *func, unsigned int priority)
{
	/* check for invalid parameters */
	if (func == NULL || priority > SO_MAX_PRIO)
		return INVALID_TID;

	/* subtract time needed for so_fork */
	if (my_scheduler->current_thread_id != -1)
		my_scheduler->current_thread_time--;

	/* lock the main thread until so_end */
	if (my_scheduler->main_thread_mutex == NULL) {
		my_scheduler->main_thread_mutex =
				malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(my_scheduler->main_thread_mutex, NULL);
		pthread_mutex_lock(my_scheduler->main_thread_mutex);
	}

	pthread_t thread_id;
	params_t *args;
	pthread_mutex_t *thread_mutex = malloc(sizeof(*thread_mutex));

	if (thread_mutex == NULL)
		return INVALID_TID;

	/* block new thread until it will be scheduled */
	pthread_mutex_init(thread_mutex, NULL);
	pthread_mutex_lock(thread_mutex);
	args = malloc(sizeof(params_t));
	if (args == NULL)
		return INVALID_TID;

	args->priority = priority;
	args->thread_mutex = thread_mutex;
	args->time_quantum = my_scheduler->time_quantum;
	args->func = func;
	args->finished = false;
	/* start thread and add it to pq */
	pthread_create(&thread_id, NULL, start_thread, args);

	if (my_pq == NULL)
		my_pq = newNode(thread_id, priority, thread_mutex);
	else
		push(&my_pq, thread_id, priority, thread_mutex);

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
	if ((int) io >= my_scheduler->io)
		return -1;

	return 0;
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

	if (my_scheduler == NULL)
		return;

	/* block main thread until all forked threads finished */
	if (my_scheduler->main_thread_mutex) {
		pthread_mutex_lock(my_scheduler->main_thread_mutex);
		if (!isEmpty(&my_pq))
			exit(EXIT_FAILURE);
	}

	params_t *args;

	/* join threads */
	while (!isEmpty(&finished_threads)) {
		head = peek(&finished_threads);
		pthread_join(head->thread_id, (void **) &args);
		free(args);
		pop(&finished_threads);
	}

	/* unlock main thread */
	if (my_scheduler->main_thread_mutex) {
		pthread_mutex_unlock(my_scheduler->main_thread_mutex);
		pthread_mutex_destroy(my_scheduler->main_thread_mutex);
		free(my_scheduler->main_thread_mutex);
	}
	freeQueue(&my_pq);
	freeQueue(&finished_threads);
	free(my_scheduler);
	my_scheduler = NULL;
}

void so_schedule_thread(int caller)
{
	if (caller == NOT_TERMINATED) {
		/* if is the first call from the main thread */
		if (my_scheduler->current_thread_id == -1)
			if (!isEmpty(&my_pq)) {
				Node *head = my_pq;

				/* choose the element with the
				 * highest priority from queue
				 */
				my_scheduler->current_thread_id =
						head->thread_id;
				my_scheduler->current_thread_priority =
						head->priority;
				my_scheduler->current_thread_mutex =
						head->mutex;
				my_scheduler->current_thread_time =
						my_scheduler->time_quantum;
				pop(&my_pq);
				pthread_mutex_unlock(
					my_scheduler->current_thread_mutex);
			}
	}
	/* if caller has finished its work,
	 * the next best thread should be scheduled
	 */
	else if (caller == TERMINATED)
		if (isEmpty(&my_pq)) {
			pthread_mutex_unlock(my_scheduler->main_thread_mutex);
			my_scheduler->current_thread_id = -1;
		} else {
			Node *head = my_pq;

			/* choose the element with the
			 * highest priority from queue
			 */
			my_scheduler->current_thread_id = head->thread_id;
			my_scheduler->current_thread_priority = head->priority;
			my_scheduler->current_thread_mutex = head->mutex;
			my_scheduler->current_thread_time =
						my_scheduler->time_quantum;
			pop(&my_pq);
			pthread_mutex_unlock(
					my_scheduler->current_thread_mutex);
		}
	/* if quantum is finished, reset it */
	else if (caller == WAITING)
		if (my_scheduler->current_thread_time == 0)
			if (isEmpty(&my_pq))
				my_scheduler->current_thread_time =
					my_scheduler->time_quantum;
}
