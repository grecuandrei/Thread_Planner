#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

typedef struct node {
	pthread_t thread_id;
	int priority;
	pthread_mutex_t* mutex;
	struct node* next;
} Node;

Node *newNode(pthread_t id, int p, pthread_mutex_t *m);

int peek_priority(Node **head);

pthread_t peek_id(Node **head);

pthread_mutex_t *peek_mutex(Node **head);

Node *peek(Node **head);

void pop(Node **head);

void push(Node **head, pthread_t id, int p, pthread_mutex_t *m);

bool isEmpty(Node **head);

void freeQueue(Node **head);
