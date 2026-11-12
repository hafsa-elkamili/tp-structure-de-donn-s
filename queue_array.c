/* File : tableau circulaire redimensionnable.
   head = indice du premier element, les elements occupent
   data[head], data[(head+1)%cap], ... (size elements). */
#include <stdlib.h>
#include "queue.h"

#define INIT_CAP 8

struct queue {
    token *data;
    size_t head, size, cap;
};

queue *queue_new(void)
{
    queue *q = malloc(sizeof *q);
    if (!q) return NULL;
    q->data = malloc(INIT_CAP * sizeof *q->data);
    if (!q->data) { free(q); return NULL; }
    q->head = q->size = 0;
    q->cap = INIT_CAP;
    return q;
}

void queue_free(queue *q)
{
    if (!q) return;
    free(q->data);
    free(q);
}

/* Agrandissement : on "deroule" le tampon circulaire dans un nouveau tableau. */
static int grow(queue *q)
{
    token *p = malloc(q->cap * 2 * sizeof *p);
    if (!p) return -1;
    for (size_t i = 0; i < q->size; i++)
        p[i] = q->data[(q->head + i) % q->cap];
    free(q->data);
    q->data = p;
    q->head = 0;
    q->cap *= 2;
    return 0;
}

int queue_enqueue(queue *q, token t)
{
    if (q->size == q->cap && grow(q) != 0) return -1;
    q->data[(q->head + q->size) % q->cap] = t;
    q->size++;
    return 0;
}

int queue_dequeue(queue *q, token *out)
{
    if (q->size == 0) return -1;
    if (out) *out = q->data[q->head];
    q->head = (q->head + 1) % q->cap;
    q->size--;
    return 0;
}

int queue_front(const queue *q, token *out)
{
    if (q->size == 0) return -1;
    *out = q->data[q->head];
    return 0;
}

size_t queue_size(const queue *q)  { return q->size; }
int    queue_empty(const queue *q) { return q->size == 0; }
const char *queue_impl(void)       { return "tableau circulaire"; }
