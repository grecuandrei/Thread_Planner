#include "queue.h"

Node *newNode(DWORD id, int p, HANDLE mutex, HANDLE thread_handle)
{
	/* allocate node */
	Node *temp = (Node *) malloc(sizeof(Node));

	temp->thread_id = id;
	temp->mutex = mutex;
	temp->priority = p;
	temp->thread_handle = thread_handle;
	temp->next = NULL;

	return temp;
}

Node *peek(Node **head)
{
	return (*head);
}

void pop(Node **head)
{
	Node *temp = *head;
	(*head) = (*head)->next;
	free(temp);
}


void push(Node **head, DWORD id, int p, HANDLE mutex, HANDLE thread_handle)
{
	/* insert element to queue */
	Node *start = (*head);
	Node *temp = newNode(id, p, mutex, thread_handle);

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

int isEmpty(Node **head)
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
