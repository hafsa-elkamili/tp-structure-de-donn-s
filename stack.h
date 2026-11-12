#ifndef STACK_H
#define STACK_H
#include <stddef.h>
#include "token.h"

/* Interface TAD Pile (identique pour les deux implementations).
   Valeurs de retour : 0 = succes, -1 = echec (pile vide ou memoire). */
typedef struct stack stack;

stack      *stack_new(void);
void        stack_free(stack *s);                  /* accepte NULL */
int         stack_push(stack *s, token t);
int         stack_pop(stack *s, token *out);       /* out peut etre NULL */
int         stack_top(const stack *s, token *out);
size_t      stack_size(const stack *s);
int         stack_empty(const stack *s);
const char *stack_impl(void);                      /* nom de l'implementation */

#endif
