#include "cutesim/queue.h"

#include <stdlib.h>

void queue_init(Queue *q) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

void queue_destroy(Queue *q) {
    QueueNode *cur = q->head;
    while (cur) {
        QueueNode *next = cur->next;
        free(cur);
        cur = next;
    }
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

int queue_enqueue(Queue *q, void *data) {
    QueueNode *node = malloc(sizeof(QueueNode));
    if (!node) {
        return -1;
    }
    node->data = data;
    node->next = NULL;
    if (q->tail) {
        q->tail->next = node;
    } else {
        q->head = node;
    }
    q->tail = node;
    q->size++;
    return 0;
}

void *queue_dequeue(Queue *q) {
    if (!q->head) {
        return NULL;
    }
    QueueNode *node = q->head;
    void      *data = node->data;
    q->head = node->next;
    if (!q->head) {
        q->tail = NULL;
    }
    free(node);
    q->size--;
    return data;
}

void *queue_peek(const Queue *q) {
    if (!q->head) {
        return NULL;
    }
    return q->head->data;
}

int queue_is_empty(const Queue *q) {
    return q->size == 0;
}

int queue_size(const Queue *q) {
    return q->size;
}
