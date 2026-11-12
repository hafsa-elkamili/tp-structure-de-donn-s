#include <assert.h>
#include <stdio.h>
#include "queue.h"

static token mk(int v) { token t = { T_NUM, (double)v, 0, "", v }; return t; }

int main(void)
{
    token t;
    queue *q = queue_new();
    assert(q);

    /* file vide */
    assert(queue_empty(q) && queue_size(q) == 0);
    assert(queue_dequeue(q, &t) == -1);
    assert(queue_front(q, &t) == -1);

    /* FIFO simple */
    queue_enqueue(q, mk(1));
    queue_enqueue(q, mk(2));
    queue_enqueue(q, mk(3));
    assert(queue_front(q, &t) == 0 && t.val == 1);
    assert(queue_dequeue(q, &t) == 0 && t.val == 1);
    assert(queue_dequeue(q, &t) == 0 && t.val == 2);
    assert(queue_dequeue(q, &t) == 0 && t.val == 3);
    assert(queue_empty(q));
    assert(queue_dequeue(q, &t) == -1);

    /* enroulement du tampon circulaire : on decale head sans jamais depasser 5 elements */
    int next_in = 0, next_out = 0;
    for (int round = 0; round < 1000; round++) {
        for (int k = 0; k < 5; k++) queue_enqueue(q, mk(next_in++));
        for (int k = 0; k < 5; k++) {
            assert(queue_dequeue(q, &t) == 0);
            assert((int)t.val == next_out++);
        }
    }
    assert(queue_empty(q));

    /* croissance alors que head != 0 (cas delicat du tableau circulaire) */
    for (int k = 0; k < 6; k++) queue_enqueue(q, mk(next_in++));
    for (int k = 0; k < 4; k++) { queue_dequeue(q, &t); assert((int)t.val == next_out++); }
    for (int k = 0; k < 500; k++) queue_enqueue(q, mk(next_in++));   /* force plusieurs agrandissements */
    assert(queue_size(q) == 502);
    while (queue_dequeue(q, &t) == 0) assert((int)t.val == next_out++);
    assert(next_out == next_in);

    /* reutilisation apres vidage (tail doit etre remis a NULL dans la version chainee) */
    queue_enqueue(q, mk(42));
    assert(queue_dequeue(q, &t) == 0 && t.val == 42);
    queue_enqueue(q, mk(43));
    assert(queue_front(q, &t) == 0 && t.val == 43);

    /* destruction d'une file non vide */
    for (int k = 0; k < 150; k++) queue_enqueue(q, mk(k));
    queue_free(q);
    queue_free(NULL);

    printf("test_queue (%s) : OK\n", queue_impl());
    return 0;
}
