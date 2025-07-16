#include <stdio.h>
#include <stdlib.h>
#define CYMCALC_IMPLEMENTATION
#include "cymcalc.h"

int main() {
    //--------------------------------------------
    // Example 1: Symbolic differentiation
    //--------------------------------------------

    // Expression: f(x) = x^3 + sin(x)

    Expr* x = expr_symbol("x");
    Expr* x3 = expr_pow(expr_retain(x), expr_number("3"));
    Expr* sinx = expr_func(FUNC_SIN, expr_retain(x));
    Expr* f = expr_add(x3, sinx);

    char* f_str = expr_to_string(f);
    printf("f(x) = %s\n", f_str);
    free(f_str);

    // Compute derivative: f'(x)
    Expr* df = expr_simplify(expr_diff(f, "x"));         

    char* df_str = expr_to_string(df);
    printf("f'(x) = %s\n", df_str);
    free(df_str);

    expr_release(df);
    expr_release(f);
    expr_release(x);

    //--------------------------------------------
    // Example 2: Evaluation
    //--------------------------------------------

    // Expression: g(y) = (3/2) * y + log(y)
    Expr* y = expr_symbol("y");
    Expr* three_half = expr_number("3/2");
    Expr* term1 = expr_mul(three_half, expr_retain(y));
    Expr* logy = expr_func(FUNC_LOG, expr_retain(y));
    Expr* g = expr_add(term1, logy);

    char* g_str = expr_to_string(g);
    printf("g(y) = %s\n", g_str);
    free(g_str);

    // Evaluate g(y) at y = 4
    Expr* g_val = expr_simplify(expr_substitute(g, "y", "4"));
    char* g_val_str = expr_to_string(g_val);
    printf("g(4) = %s = %f \n", g_val_str, expr_eval_numeric(g_val));
    free(g_val_str);

    expr_release(g_val);
    expr_release(g);
    expr_release(y);

    //--------------------------------------------
    // Example 3: Nested expressions
    //--------------------------------------------

    // Expression: h(x) = sin(x) * exp(x^2)
    x = expr_symbol("x");
    Expr* x2 = expr_pow(expr_retain(x), expr_number("2"));
    Expr* exp_x2 = expr_func(FUNC_EXP, x2);
    sinx = expr_func(FUNC_SIN, expr_retain(x));
    Expr* h = expr_mul(sinx, exp_x2);

    char* h_str = expr_to_string(h);
    printf("h(x) = %s\n", h_str);
    free(h_str);

    // Differentiate h(x)
    Expr* dh = expr_simplify(expr_diff(h, "x"));
    char* dh_str = expr_to_string(dh);
    printf("h'(x) = %s\n", dh_str);
    free(dh_str);

    expr_release(dh);
    expr_release(h);
    expr_release(x);

    return 0;
}
