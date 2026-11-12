#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "expr.h"
#include "stack.h"

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { printf("ECHEC : "); printf(__VA_ARGS__); \
                              printf("\n"); failures++; } } while (0)

static void brackets(const char *s, expr_status exp_st, int exp_pos)
{
    int pos;
    expr_status st = expr_check_brackets(s, &pos);
    CHECK(st == exp_st && pos == exp_pos, "brackets(\"%s\") -> %d pos %d (attendu %d pos %d)",
          s, st, pos, exp_st, exp_pos);
}

static void postfix(const char *s, const char *expected)
{
    char buf[128];
    int pos;
    expr_status st = expr_to_postfix_string(s, buf, sizeof buf, &pos);
    CHECK(st == EXPR_OK && strcmp(buf, expected) == 0,
          "postfix(\"%s\") = \"%s\" (attendu \"%s\")", s, st == EXPR_OK ? buf : "<erreur>", expected);
}

static void value(const char *s, const double *vars, double expected)
{
    double v = 0;
    int pos;
    expr_status st = expr_eval(s, vars, &v, &pos);
    CHECK(st == EXPR_OK && fabs(v - expected) < 1e-9,
          "eval(\"%s\") = %g, statut %d (attendu %g)", s, v, st, expected);
}

static void error(const char *s, expr_status exp_st, int exp_pos)
{
    double v;
    int pos;
    expr_status st = expr_eval(s, NULL, &v, &pos);
    CHECK(st == exp_st && pos == exp_pos, "eval(\"%s\") -> %s pos %d (attendu %s pos %d)",
          s, expr_strerror(st), pos, expr_strerror(exp_st), exp_pos);
}

int main(void)
{
    /* --- etape 3 : parenthesage --- */
    brackets("", EXPR_OK, -1);
    brackets("(1+2)*[3-{4}]", EXPR_OK, -1);
    brackets("(1+2", EXPR_ERR_PAREN, 0);        /* ouvrante jamais fermee */
    brackets("1+2)", EXPR_ERR_PAREN, 3);        /* fermante orpheline */
    brackets("(]", EXPR_ERR_PAREN, 1);          /* mauvais type */
    brackets("((1)", EXPR_ERR_PAREN, 0);
    brackets(")(", EXPR_ERR_PAREN, 0);

    /* --- etape 5 : conversion postfixe, priorites et associativite --- */
    postfix("3+4*2", "3 4 2 * +");
    postfix("(1+2)*3", "1 2 + 3 *");
    postfix("8-3-2", "8 3 - 2 -");              /* associativite a gauche */
    postfix("2^3^2", "2 3 2 ^ ^");              /* associativite a droite */
    postfix("-3+4", "3 neg 4 +");
    postfix("2^-1", "2 1 neg ^");
    postfix("max(3,4)", "3 4 max");
    postfix("max(1+2, 4*5)", "1 2 + 4 5 * max");
    postfix("x^2+1", "x 2 ^ 1 +");

    /* --- etape 6 : evaluation --- */
    double vars[26] = { 0 };
    vars['x' - 'a'] = 3;
    vars['y' - 'a'] = 0.5;
    value("3+4*2", NULL, 11);
    value("(1+2)*3", NULL, 9);
    value("10/4", NULL, 2.5);
    value("2^3^2", NULL, 512);
    value("-3+4", NULL, 1);
    value("-2^2", NULL, -4);
    value("2^-1", NULL, 0.5);
    value("2*(-(1+1))", NULL, -4);
    value("2*(-1+1)", NULL, 0);
    value("  1.5 + .5 ", NULL, 2);
    value("x^2+1", vars, 10);
    value("x*y + 2*x", vars, 7.5);
    value("max(3,4)", NULL, 4);
    value("min(max(1,5),3)", NULL, 3);
    value("sqrt(16)+abs(-2)", NULL, 6);
    value("+3", NULL, 3);

    /* --- etape 6 : erreurs --- */
    error("1/0", EXPR_ERR_DIVZERO, 1);
    error("1/(2-2)", EXPR_ERR_DIVZERO, 1);
    error("(1+2", EXPR_ERR_PAREN, 0);
    error("1+2)", EXPR_ERR_PAREN, 3);
    error("3 $ 4", EXPR_ERR_TOKEN, 2);
    error("1.2.3", EXPR_ERR_TOKEN, 0);
    error("foo(1)", EXPR_ERR_TOKEN, 0);
    error("3 +", EXPR_ERR_OPERAND, 2);
    error("* 3", EXPR_ERR_OPERAND, 0);
    error("3 4", EXPR_ERR_MALFORMED, 2);
    error("", EXPR_ERR_MALFORMED, 0);
    error("   ", EXPR_ERR_MALFORMED, 0);
    error("sqrt(-1)", EXPR_ERR_DOMAIN, 0);
    error("x+1", EXPR_ERR_VAR, 0);
    error("max(3)", EXPR_ERR_OPERAND, 0);
    error("max(3,4,5)", EXPR_ERR_MALFORMED, 0);

    if (failures) { printf("test_expr (%s) : %d echec(s)\n", stack_impl(), failures); return 1; }
    printf("test_expr (%s) : OK\n", stack_impl());
    return 0;
}
