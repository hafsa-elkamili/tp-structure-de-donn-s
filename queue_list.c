/* File : liste simplement chainee avec pointeurs tete (sortie) et queue (entree). */
#include <stdlib.h>
#include "queue.h"

typedef struct qnode {
    token t;
    struct qnode *next;
} qnode;

struct queue {
    qnode *head, *tail;
    size_t size;
};

queue *queue_new(void)
{
    queue *q = malloc(sizeof *q);
    if (!q) return NULL;
    q->head = q->tail = NULL;
    q->size = 0;
    return q;
}

void queue_free(queue *q)
{
    if (!q) return;
    while (q->head) {
        qnode *n = q->head;
        q->head = n->next;
        free(n);
    }
    free(q);
}

int queue_enqueue(queue *q, token t)
{
    qnode *n = malloc(sizeof *n);
    if (!n) return -1;
    n->t = t;
    n->next = NULL;
    if (q->tail) q->tail->next = n; else q->head = n;
    q->tail = n;
    q->size++;
    return 0;
}

int queue_dequeue(queue *q, token *out)
{
    if (!q->head) return -1;
    qnode *n = q->head;
    if (out) *out = n->t;
    q->head = n->next;
    if (!q->head) q->tail = NULL;   /* file devenue vide : ne pas laisser tail pendant */
    free(n);
    q->size--;
    return 0;
}

int queue_front(const queue *q, token *out)
{
    if (!q->head) return -1;
    *out = q->head->t;
    return 0;
}

size_t queue_size(const queue *q)  { return q->size; }
int    queue_empty(const queue *q) { return q->size == 0; }
const char *queue_impl(void)       { return "liste chainee"; }
