#include "queue.h"

Node *newNode(pthread_t id, int p, pthread_mutex_t *mutex)
{
	/* allocate node */
	Node *temp = (Node *) malloc(sizeof(Node));

	temp->thread_id = id;
	temp->mutex = mutex;
	temp->priority = p;
	temp->next = NULL;

	return temp;
}

int peek_priority(Node **head)
{
	return (*head)->priority;
}

pthread_t peek_id(Node **head)
{
	return (*head)->thread_id;
}

pthread_mutex_t *peek_mutex(Node **head)
{
	return (*head)->mutex;
}

Node *peek(Node **head)
{
	return *head;
}

void pop(Node **head)
{
	Node *temp = *head;

	(*head) = (*head)->next;
	free(temp);
}


void push(Node **head, pthread_t id, int p, pthread_mutex_t *mutex)
{
	/* insert element to queue */
	Node *start = (*head);
	Node *temp = newNode(id, p, mutex);

	if ((*head)->priority > p) {
		temp->next = *head;
		(*head) = temp;
	} else {
		while (start->next != NULL &&
			start->next->priority < p) {
			start = start->next;
		}
		temp->next = start->next;
		start->next = temp;
	}
}

bool isEmpty(Node **head)
{
	return (*head) == NULL;
}

void freeQueue(Node **head)
{
	/* free queue */
	while (!isEmpty(head)) {
		Node *start = *head;

		(*head) = (*head)->next;
		start->next = NULL;
		start->priority = 0;
		free(start);
	}
}
