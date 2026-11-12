/* Pile : liste simplement chainee (insertion/suppression en tete). */
#include <stdlib.h>
#include "stack.h"

typedef struct snode {
    token t;
    struct snode *next;
} snode;

struct stack {
    snode *head;
    size_t size;
};

stack *stack_new(void)
{
    stack *s = malloc(sizeof *s);
    if (!s) return NULL;
    s->head = NULL;
    s->size = 0;
    return s;
}

void stack_free(stack *s)
{
    if (!s) return;
    while (s->head) {
        snode *n = s->head;
        s->head = n->next;
        free(n);
    }
    free(s);
}

int stack_push(stack *s, token t)
{
    snode *n = malloc(sizeof *n);
    if (!n) return -1;
    n->t = t;
    n->next = s->head;
    s->head = n;
    s->size++;
    return 0;
}

int stack_pop(stack *s, token *out)
{
    if (!s->head) return -1;
    snode *n = s->head;
    if (out) *out = n->t;
    s->head = n->next;
    free(n);
    s->size--;
    return 0;
}

int stack_top(const stack *s, token *out)
{
    if (!s->head) return -1;
    *out = s->head->t;
    return 0;
}

size_t stack_size(const stack *s)  { return s->size; }
int    stack_empty(const stack *s) { return s->size == 0; }
const char *stack_impl(void)       { return "liste chainee"; }
