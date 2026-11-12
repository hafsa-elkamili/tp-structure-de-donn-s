#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "expr.h"
#include "stack.h"

const char *expr_strerror(expr_status st)
{
    switch (st) {
    case EXPR_OK:            return "ok";
    case EXPR_ERR_PAREN:     return "parenthese non appariee";
    case EXPR_ERR_TOKEN:     return "jeton invalide";
    case EXPR_ERR_SYNTAX:    return "erreur de syntaxe";
    case EXPR_ERR_OPERAND:   return "operande manquante";
    case EXPR_ERR_MALFORMED: return "expression mal formee";
    case EXPR_ERR_DIVZERO:   return "division par zero";
    case EXPR_ERR_DOMAIN:    return "resultat hors domaine";
    case EXPR_ERR_VAR:       return "variable sans valeur";
    case EXPR_ERR_MEM:       return "memoire insuffisante";
    }
    return "erreur inconnue";
}

static void set_pos(int *p, int v) { if (p) *p = v; }

/* ------------------------------------------------------------------ */
/* Etape 3 : parenthesage                                              */
/* ------------------------------------------------------------------ */

static char closer_of(char open)
{
    return open == '(' ? ')' : open == '[' ? ']' : '}';
}

expr_status expr_check_brackets(const char *s, int *errpos)
{
    expr_status r = EXPR_OK;
    stack *st = stack_new();
    token t;
    int pos = -1;

    if (!st) { set_pos(errpos, -1); return EXPR_ERR_MEM; }

    for (int i = 0; s[i] && r == EXPR_OK; i++) {
        char c = s[i];
        if (c == '(' || c == '[' || c == '{') {
            memset(&t, 0, sizeof t);
            t.type = T_LP;
            t.op = c;
            t.pos = i;
            if (stack_push(st, t) != 0) r = EXPR_ERR_MEM;
        } else if (c == ')' || c == ']' || c == '}') {
            /* pile vide (fermante orpheline) ou mauvais type de parenthese */
            if (stack_pop(st, &t) != 0 || closer_of(t.op) != c) {
                r = EXPR_ERR_PAREN;
                pos = i;
            }
        }
    }
    if (r == EXPR_OK && !stack_empty(st)) {
        /* ouvrantes jamais fermees : on signale la plus a gauche */
        while (stack_pop(st, &t) == 0) { }
        r = EXPR_ERR_PAREN;
        pos = t.pos;
    }
    stack_free(st);
    set_pos(errpos, pos);
    return r;
}

/* ------------------------------------------------------------------ */
/* Etape 4 : analyse lexicale                                          */
/* ------------------------------------------------------------------ */

static int is_func(const char *n)
{
    static const char *names[] = { "sin", "cos", "sqrt", "abs", "max", "min" };
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++)
        if (strcmp(n, names[i]) == 0) return 1;
    return 0;
}

static int arity(const char *n)
{
    return (strcmp(n, "max") == 0 || strcmp(n, "min") == 0) ? 2 : 1;
}

expr_status expr_tokenize(const char *s, queue **out, int *errpos)
{
    expr_status st = EXPR_OK;
    int pos = -1, i = 0;
    int prev_operand = 0;   /* 1 si le jeton precedent est NUM, VAR ou ')' */
    queue *q = queue_new();
    token t;

    if (!q) { set_pos(errpos, -1); return EXPR_ERR_MEM; }

    while (s[i] && st == EXPR_OK) {
        unsigned char c = (unsigned char)s[i];
        if (isspace(c)) { i++; continue; }

        memset(&t, 0, sizeof t);
        t.pos = i;

        if (isdigit(c) || c == '.') {
            char buf[64];
            int j = i, dots = 0, digits = 0;
            while (isdigit((unsigned char)s[j]) || s[j] == '.') {
                if (s[j] == '.') dots++; else digits++;
                j++;
            }
            if (dots > 1 || digits == 0 || (size_t)(j - i) >= sizeof buf) {
                st = EXPR_ERR_TOKEN; pos = i; break;
            }
            memcpy(buf, s + i, (size_t)(j - i));
            buf[j - i] = '\0';
            t.type = T_NUM;
            t.val = strtod(buf, NULL);
            prev_operand = 1;
            i = j;
        } else if (isalpha(c)) {
            int j = i, len;
            while (isalpha((unsigned char)s[j])) j++;
            len = j - i;
            if ((size_t)len >= sizeof t.name) { st = EXPR_ERR_TOKEN; pos = i; break; }
            for (int k = 0; k < len; k++)
                t.name[k] = (char)tolower((unsigned char)s[i + k]);
            t.name[len] = '\0';
            if (len == 1 && s[j] != '(') {
                t.type = T_VAR;
                t.op = t.name[0];
                prev_operand = 1;
            } else if (is_func(t.name)) {
                t.type = T_FUNC;
                prev_operand = 0;
            } else {
                st = EXPR_ERR_TOKEN; pos = i; break;
            }
            i = j;
        } else {
            switch (c) {
            case '(': t.type = T_LP; prev_operand = 0; break;
            case ')': t.type = T_RP; prev_operand = 1; break;
            case ',': t.type = T_COMMA; prev_operand = 0; break;
            case '+': case '-': case '*': case '/': case '^':
                if (!prev_operand && c == '+') { i++; continue; }  /* + unaire : ignore */
                t.type = T_OP;
                t.op = (!prev_operand && c == '-') ? 'u' : (char)c;
                prev_operand = 0;
                break;
            default:
                st = EXPR_ERR_TOKEN; pos = i; break;
            }
            if (st != EXPR_OK) break;
            i++;
        }
        if (queue_enqueue(q, t) != 0) st = EXPR_ERR_MEM;
    }

    set_pos(errpos, pos);
    if (st != EXPR_OK) { queue_free(q); return st; }
    *out = q;
    return EXPR_OK;
}

/* ------------------------------------------------------------------ */
/* Etape 5 : infixe -> postfixe (algorithme de Dijkstra)               */
/* ------------------------------------------------------------------ */

static int prec(char op)
{
    switch (op) {
    case '+': case '-': return 1;
    case '*': case '/': return 2;
    case 'u':           return 3;   /* moins unaire : -2^2 = -(2^2) */
    case '^':           return 4;
    }
    return 0;
}

static int right_assoc(char op) { return op == '^' || op == 'u'; }

expr_status expr_to_postfix(queue *in, queue **out, int *errpos)
{
    expr_status st = EXPR_OK;
    int pos = -1;
    stack *ops = stack_new();
    queue *res = queue_new();
    token t, top;

    if (!ops || !res) { st = EXPR_ERR_MEM; goto done; }

    while (st == EXPR_OK && queue_dequeue(in, &t) == 0) {
        switch (t.type) {
        case T_NUM:
        case T_VAR:
            if (queue_enqueue(res, t) != 0) st = EXPR_ERR_MEM;
            break;

        case T_FUNC:
            if (stack_push(ops, t) != 0) st = EXPR_ERR_MEM;
            break;

        case T_COMMA:
            while (stack_top(ops, &top) == 0 && top.type != T_LP) {
                stack_pop(ops, NULL);
                if (queue_enqueue(res, top) != 0) { st = EXPR_ERR_MEM; break; }
            }
            if (st == EXPR_OK && stack_empty(ops)) { st = EXPR_ERR_SYNTAX; pos = t.pos; }
            break;

        case T_OP:
            /* un operateur prefixe (moins unaire) ne depile jamais rien */
            if (t.op != 'u') {
                while (st == EXPR_OK && stack_top(ops, &top) == 0 && top.type == T_OP &&
                       (prec(top.op) > prec(t.op) ||
                        (prec(top.op) == prec(t.op) && !right_assoc(t.op)))) {
                    stack_pop(ops, NULL);
                    if (queue_enqueue(res, top) != 0) st = EXPR_ERR_MEM;
                }
            }
            if (st == EXPR_OK && stack_push(ops, t) != 0) st = EXPR_ERR_MEM;
            break;

        case T_LP:
            if (stack_push(ops, t) != 0) st = EXPR_ERR_MEM;
            break;

        case T_RP:
            while (stack_top(ops, &top) == 0 && top.type != T_LP) {
                stack_pop(ops, NULL);
                if (queue_enqueue(res, top) != 0) { st = EXPR_ERR_MEM; break; }
            }
            if (st != EXPR_OK) break;
            if (stack_empty(ops)) { st = EXPR_ERR_PAREN; pos = t.pos; break; }
            stack_pop(ops, NULL);                       /* depile '(' */
            if (stack_top(ops, &top) == 0 && top.type == T_FUNC) {
                stack_pop(ops, NULL);
                if (queue_enqueue(res, top) != 0) st = EXPR_ERR_MEM;
            }
            break;
        }
    }

    while (st == EXPR_OK && stack_pop(ops, &top) == 0) {
        if (top.type == T_LP) { st = EXPR_ERR_PAREN; pos = top.pos; }
        else if (queue_enqueue(res, top) != 0) st = EXPR_ERR_MEM;
    }

done:
    stack_free(ops);
    if (st == EXPR_OK) *out = res; else queue_free(res);
    set_pos(errpos, pos);
    return st;
}

/* ------------------------------------------------------------------ */
/* Etape 6 : evaluation postfixe                                       */
/* ------------------------------------------------------------------ */

expr_status expr_eval_postfix(queue *pf, const double vars[26],
                              double *result, int *errpos)
{
    expr_status st = EXPR_OK;
    int pos = -1;
    stack *vs = stack_new();
    token t, a, b, r;

    if (!vs) { set_pos(errpos, -1); return EXPR_ERR_MEM; }
    memset(&a, 0, sizeof a);

    while (st == EXPR_OK && queue_dequeue(pf, &t) == 0) {
        memset(&r, 0, sizeof r);
        r.type = T_NUM;
        r.pos = t.pos;

        switch (t.type) {
        case T_NUM:
            r.val = t.val;
            break;
        case T_VAR:
            if (!vars) { st = EXPR_ERR_VAR; pos = t.pos; }
            else r.val = vars[t.op - 'a'];
            break;
        case T_OP:
            if (t.op == 'u') {
                if (stack_pop(vs, &a) != 0) { st = EXPR_ERR_OPERAND; pos = t.pos; }
                else r.val = -a.val;
            } else if (stack_pop(vs, &b) != 0 || stack_pop(vs, &a) != 0) {
                st = EXPR_ERR_OPERAND; pos = t.pos;
            } else {
                switch (t.op) {
                case '+': r.val = a.val + b.val; break;
                case '-': r.val = a.val - b.val; break;
                case '*': r.val = a.val * b.val; break;
                case '/':
                    if (b.val == 0.0) { st = EXPR_ERR_DIVZERO; pos = t.pos; }
                    else r.val = a.val / b.val;
                    break;
                case '^': r.val = pow(a.val, b.val); break;
                default:  st = EXPR_ERR_SYNTAX; pos = t.pos; break;
                }
            }
            break;
        case T_FUNC:
            if (arity(t.name) == 2) {
                if (stack_pop(vs, &b) != 0 || stack_pop(vs, &a) != 0) {
                    st = EXPR_ERR_OPERAND; pos = t.pos; break;
                }
                r.val = (strcmp(t.name, "max") == 0)
                        ? (a.val > b.val ? a.val : b.val)
                        : (a.val < b.val ? a.val : b.val);
            } else {
                if (stack_pop(vs, &a) != 0) { st = EXPR_ERR_OPERAND; pos = t.pos; break; }
                if (strcmp(t.name, "sin") == 0)       r.val = sin(a.val);
                else if (strcmp(t.name, "cos") == 0)  r.val = cos(a.val);
                else if (strcmp(t.name, "abs") == 0)  r.val = fabs(a.val);
                else if (a.val < 0) { st = EXPR_ERR_DOMAIN; pos = t.pos; }
                else r.val = sqrt(a.val);
            }
            break;
        case T_LP: case T_RP: case T_COMMA:
            st = EXPR_ERR_SYNTAX; pos = t.pos;
            break;
        }

        if (st == EXPR_OK && !isfinite(r.val)) { st = EXPR_ERR_DOMAIN; pos = t.pos; }
        if (st == EXPR_OK && stack_push(vs, r) != 0) st = EXPR_ERR_MEM;
    }

    if (st == EXPR_OK) {
        if (stack_pop(vs, &a) != 0) { st = EXPR_ERR_MALFORMED; pos = 0; }
        else if (!stack_empty(vs))  { st = EXPR_ERR_MALFORMED; pos = a.pos; }
        else *result = a.val;
    }
    stack_free(vs);
    set_pos(errpos, pos);
    return st;
}

/* ------------------------------------------------------------------ */
/* Enchainement complet                                                */
/* ------------------------------------------------------------------ */

expr_status expr_eval(const char *s, const double vars[26],
                      double *result, int *errpos)
{
    queue *inf = NULL, *pf = NULL;
    expr_status st = expr_check_brackets(s, errpos);
    if (st != EXPR_OK) return st;
    st = expr_tokenize(s, &inf, errpos);
    if (st != EXPR_OK) return st;
    st = expr_to_postfix(inf, &pf, errpos);
    queue_free(inf);
    if (st != EXPR_OK) return st;
    st = expr_eval_postfix(pf, vars, result, errpos);
    queue_free(pf);
    return st;
}

expr_status expr_to_postfix_string(const char *s, char *buf, size_t n, int *errpos)
{
    queue *inf = NULL, *pf = NULL;
    token t;
    size_t len = 0;
    expr_status st = expr_tokenize(s, &inf, errpos);
    if (st != EXPR_OK) return st;
    st = expr_to_postfix(inf, &pf, errpos);
    queue_free(inf);
    if (st != EXPR_OK) return st;

    if (n == 0) { queue_free(pf); return EXPR_ERR_MEM; }
    buf[0] = '\0';
    while (queue_dequeue(pf, &t) == 0 && st == EXPR_OK) {
        char piece[40];
        switch (t.type) {
        case T_NUM:  snprintf(piece, sizeof piece, "%g", t.val); break;
        case T_VAR:  snprintf(piece, sizeof piece, "%c", t.op); break;
        case T_FUNC: snprintf(piece, sizeof piece, "%s", t.name); break;
        case T_OP:   snprintf(piece, sizeof piece, "%s",
                              t.op == 'u' ? "neg" : (char[]){ t.op, '\0' }); break;
        default:     snprintf(piece, sizeof piece, "?"); break;
        }
        size_t need = strlen(piece) + (len ? 1 : 0);
        if (len + need + 1 > n) { st = EXPR_ERR_MEM; break; }   /* buffer trop petit */
        if (len) buf[len++] = ' ';
        memcpy(buf + len, piece, strlen(piece) + 1);
        len += strlen(piece);
    }
    queue_free(pf);
    return st;
}
