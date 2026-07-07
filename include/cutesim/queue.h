#pragma once

typedef struct QueueNode {
    void *data;
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *head;
    QueueNode *tail;
    int size;
} Queue;

/* Initialise a Queue to the empty state.
   Must be called before any other queue function. */
void queue_init(Queue *q);

/* Release all internal nodes.
   Does not free the data pointers — caller owns the data. */
void queue_destroy(Queue *q);

/* Append data to the back of the queue.
   Returns 0 on success, -1 on allocation failure. */
int queue_enqueue(Queue *q, void *data);

/* Remove and return the front element.
   Returns NULL if the queue is empty. */
void *queue_dequeue(Queue *q);

/* Return the front element without removing it.
   Returns NULL if the queue is empty. */
void *queue_peek(const Queue *q);

/* Return 1 if the queue is empty, 0 otherwise. */
int queue_is_empty(const Queue *q);

/* Return the number of elements currently in the queue. */
int queue_size(const Queue *q);
