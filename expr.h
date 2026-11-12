#ifndef EXPR_H
#define EXPR_H
#include <stddef.h>
#include "queue.h"

typedef enum {
    EXPR_OK = 0,
    EXPR_ERR_PAREN,      /* parenthese non appariee                   */
    EXPR_ERR_TOKEN,      /* caractere / identifiant / nombre invalide */
    EXPR_ERR_SYNTAX,     /* virgule ou jeton mal place                */
    EXPR_ERR_OPERAND,    /* operandes insuffisantes pour un operateur */
    EXPR_ERR_MALFORMED,  /* expression vide ou operandes en trop      */
    EXPR_ERR_DIVZERO,    /* division par zero                         */
    EXPR_ERR_DOMAIN,     /* sqrt(-1), depassement, ...                */
    EXPR_ERR_VAR,        /* variable utilisee sans table de valeurs   */
    EXPR_ERR_MEM
} expr_status;

const char *expr_strerror(expr_status st);

/* Les fonctions ci-dessous renvoient un statut ; *errpos (peut etre NULL)
   recoit l'indice dans la chaine de la premiere erreur, ou -1. */

/* Etape 3 : parenthesage ( ) [ ] { } avec une pile. */
expr_status expr_check_brackets(const char *s, int *errpos);

/* Etape 4 : analyse lexicale -> file de jetons infixes. */
expr_status expr_tokenize(const char *s, queue **out, int *errpos);

/* Etape 5 : infixe -> postfixe (shunting-yard). La file `infix` est videe. */
expr_status expr_to_postfix(queue *infix, queue **out, int *errpos);

/* Etape 6 : evaluation avec une pile. La file `postfix` est videe.
   vars : tableau de 26 valeurs pour a..z (peut etre NULL si pas de variable). */
expr_status expr_eval_postfix(queue *postfix, const double vars[26],
                              double *result, int *errpos);

/* Tout enchaine : parenthesage, lexer, postfixe, evaluation. */
expr_status expr_eval(const char *s, const double vars[26],
                      double *result, int *errpos);

/* Utilitaire : ecrit la notation postfixe ("3 4 2 * +") dans buf. */
expr_status expr_to_postfix_string(const char *s, char *buf, size_t n,
                                   int *errpos);

#endif
