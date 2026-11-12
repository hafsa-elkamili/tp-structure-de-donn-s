#include <assert.h>
#include <stdio.h>
#include "stack.h"

static token mk(int v) { token t = { T_NUM, (double)v, 0, "", v }; return t; }

int main(void)
{
    token t;
    stack *s = stack_new();
    assert(s);

    /* pile vide */
    assert(stack_empty(s) && stack_size(s) == 0);
    assert(stack_pop(s, &t) == -1);
    assert(stack_top(s, &t) == -1);

    /* push / top / pop simples (LIFO) */
    assert(stack_push(s, mk(1)) == 0);
    assert(stack_push(s, mk(2)) == 0);
    assert(stack_size(s) == 2);
    assert(stack_top(s, &t) == 0 && t.val == 2);
    assert(stack_size(s) == 2);                 /* top ne retire rien */
    assert(stack_pop(s, &t) == 0 && t.val == 2);
    assert(stack_pop(s, &t) == 0 && t.val == 1);
    assert(stack_empty(s));
    assert(stack_pop(s, &t) == -1);             /* pop a vide apres usage */

    /* croissance : plus de 100 elements, voire beaucoup plus */
    for (int i = 0; i < 5000; i++) assert(stack_push(s, mk(i)) == 0);
    assert(stack_size(s) == 5000);
    for (int i = 4999; i >= 0; i--) {
        assert(stack_pop(s, &t) == 0);
        assert((int)t.val == i);
    }
    assert(stack_empty(s));

    /* pop avec out == NULL */
    stack_push(s, mk(7));
    assert(stack_pop(s, NULL) == 0 && stack_empty(s));

    /* destruction d'une pile non vide (verifie par AddressSanitizer/LeakSanitizer) */
    for (int i = 0; i < 150; i++) stack_push(s, mk(i));
    stack_free(s);
    stack_free(NULL);

    printf("test_stack (%s) : OK\n", stack_impl());
    return 0;
}
