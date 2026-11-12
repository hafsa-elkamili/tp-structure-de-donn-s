#ifndef TOKEN_H
#define TOKEN_H

/* Jeton de l'analyseur. Meme type d'element pour les piles et les files. */
typedef enum { T_NUM, T_VAR, T_OP, T_FUNC, T_LP, T_RP, T_COMMA } token_type;

typedef struct {
    token_type type;
    double val;      /* T_NUM : valeur                                   */
    char op;         /* T_OP : + - * / ^ ou 'u' (moins unaire)
                        T_VAR : lettre de la variable (a-z)               */
    char name[8];    /* T_FUNC : sin cos sqrt abs max min                */
    int pos;         /* position dans la chaine source (messages d'erreur) */
} token;

#endif
