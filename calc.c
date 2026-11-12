/* Mini-analyseur interactif : ./calc "3+4*2"  ou  ./calc  (lit stdin).
   Variables : x=..., y=... via la ligne "let x 3". */
#include <stdio.h>
#include <string.h>
#include "expr.h"

static void run(const char *line, double vars[26])
{
    char pf[256];
    double v;
    int pos = -1;
    expr_status st = expr_to_postfix_string(line, pf, sizeof pf, &pos);
    if (st == EXPR_OK) st = expr_eval(line, vars, &v, &pos);
    if (st == EXPR_OK) {
        printf("postfixe : %s\nresultat : %g\n", pf, v);
    } else {
        printf("erreur : %s\n", expr_strerror(st));
        if (pos >= 0) printf("  %s\n  %*s^\n", line, pos, "");
    }
}

int main(int argc, char **argv)
{
    double vars[26] = { 0 };
    if (argc > 1) { run(argv[1], vars); return 0; }

    char line[512];
    printf("> ");
    while (fgets(line, sizeof line, stdin)) {
        line[strcspn(line, "\n")] = '\0';
        char name; double val;
        if (sscanf(line, "let %c %lf", &name, &val) == 2 && name >= 'a' && name <= 'z')
            vars[name - 'a'] = val;
        else if (line[0])
            run(line, vars);
        printf("> ");
    }
    printf("\n");
    return 0;
}
