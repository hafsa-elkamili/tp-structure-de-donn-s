#ifndef QUEUE_H
#define QUEUE_H
#include <stddef.h>
#include "token.h"

/* Interface TAD File (identique pour les deux implementations).
   Valeurs de retour : 0 = succes, -1 = echec (file vide ou memoire). */
typedef struct queue queue;

queue      *queue_new(void);
void        queue_free(queue *q);                  /* accepte NULL */
int         queue_enqueue(queue *q, token t);
int         queue_dequeue(queue *q, token *out);   /* out peut etre NULL */
int         queue_front(const queue *q, token *out);
size_t      queue_size(const queue *q);
int         queue_empty(const queue *q);
const char *queue_impl(void);

#endif
