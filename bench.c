/* Mesures etape 7 : meme programme, lie a l'une ou l'autre implementation. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "stack.h"
#include "queue.h"

static double now(void) { return (double)clock() / CLOCKS_PER_SEC; }

int main(int argc, char **argv)
{
    size_t n = argc > 1 ? (size_t)strtoul(argv[1], NULL, 10) : 5000000u;
    token t = { T_NUM, 1.0, 0, "", 0 };
    double t0;

    printf("== %s (n = %zu)\n", stack_impl(), n);

    stack *s = stack_new();
    t0 = now();
    for (size_t i = 0; i < n; i++) stack_push(s, t);
    printf("pile : %zu push        %.3f s\n", n, now() - t0);
    t0 = now();
    for (size_t i = 0; i < n; i++) stack_pop(s, NULL);
    printf("pile : %zu pop         %.3f s\n", n, now() - t0);
    t0 = now();
    for (size_t i = 0; i < 3 * n; i++) { stack_push(s, t); stack_push(s, t); stack_pop(s, NULL); stack_pop(s, NULL); }
    printf("pile : usage stable (petite taille) %.3f s\n", now() - t0);
    stack_free(s);

    queue *q = queue_new();
    t0 = now();
    for (size_t i = 0; i < n; i++) queue_enqueue(q, t);
    printf("file : %zu enqueue     %.3f s\n", n, now() - t0);
    t0 = now();
    for (size_t i = 0; i < n; i++) queue_dequeue(q, NULL);
    printf("file : %zu dequeue     %.3f s\n", n, now() - t0);
    t0 = now();
    for (size_t i = 0; i < 3 * n; i++) { queue_enqueue(q, t); queue_enqueue(q, t); queue_dequeue(q, NULL); queue_dequeue(q, NULL); }
    printf("file : usage stable (petite taille) %.3f s\n", now() - t0);
    queue_free(q);
    return 0;
}
