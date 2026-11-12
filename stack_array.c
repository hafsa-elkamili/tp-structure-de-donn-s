/* Pile : tableau dynamique (capacite doublee quand il est plein). */
#include <stdlib.h>
#include "stack.h"

#define INIT_CAP 8

struct stack {
    token *data;
    size_t size, cap;
};

stack *stack_new(void)
{
    stack *s = malloc(sizeof *s);
    if (!s) return NULL;
    s->data = malloc(INIT_CAP * sizeof *s->data);
    if (!s->data) { free(s); return NULL; }
    s->size = 0;
    s->cap = INIT_CAP;
    return s;
}

void stack_free(stack *s)
{
    if (!s) return;
    free(s->data);
    free(s);
}

int stack_push(stack *s, token t)
{
    if (s->size == s->cap) {
        token *p = realloc(s->data, s->cap * 2 * sizeof *p);
        if (!p) return -1;          /* l'ancien tableau reste valide */
        s->data = p;
        s->cap *= 2;
    }
    s->data[s->size++] = t;
    return 0;
}

int stack_pop(stack *s, token *out)
{
    if (s->size == 0) return -1;
    token t = s->data[--s->size];
    if (out) *out = t;
    return 0;
}

int stack_top(const stack *s, token *out)
{
    if (s->size == 0) return -1;
    *out = s->data[s->size - 1];
    return 0;
}

size_t stack_size(const stack *s)  { return s->size; }
int    stack_empty(const stack *s) { return s->size == 0; }
const char *stack_impl(void)       { return "tableau dynamique"; }
