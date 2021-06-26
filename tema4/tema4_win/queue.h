#include <stdio.h>
#include <stdlib.h>
#include "windows.h"

#define true 1
#define false 0

typedef struct node {
	DWORD thread_id;
	int priority;
	HANDLE mutex;
	HANDLE thread_handle;
	struct node *next;
} Node;

Node *newNode(DWORD id, int p, HANDLE m, HANDLE thread_handle);

Node *peek(Node **head);

void pop(Node **head);

void push(Node **head, DWORD id, int p, HANDLE m, HANDLE thread_handle);

int isEmpty(Node **head);

void freeQueue(Node **head);
