#include <stdio.h>
#include <stdlib.h>
#define CYMCALC_IMPLEMENTATION
#include "cymcalc.h"

#ifdef _WIN32
#include <windows.h>
#endif

void setup_utf8_console() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
}

int main() {

    setup_utf8_console();

    printf("----------------------------------------------------\n");
    printf(" Example 1: Symbolic differentiation and integration\n");
    printf("----------------------------------------------------\n");

    Expr* x = expr_symbol("x");
    Expr* x3 = expr_pow(expr_retain(x), expr_number("3"));
    Expr* sinx = expr_func(FUNC_SIN, expr_retain(x));
    Expr* f = expr_add(x3, sinx);

    char* f_str = expr_to_string(f);
    printf("f(x) = %s\n", f_str);

    Expr* df = expr_diff(f, "x");  
    Expr* df_sim = expr_simplify(df);       

    char* df_str = expr_to_string(df);
    char* dfs_str = expr_to_string(df_sim);
    printf("%s = %s\n", df_str, dfs_str);
    Expr* Sf = expr_int(f, "x"); 
    Expr* Sf_sim = expr_simplify(Sf);        

    char* Sf_str = expr_to_string(Sf);
    char* Sfs_str = expr_to_string(Sf_sim);

    printf("%s = %s\n", Sf_str, Sfs_str);
    Expr* Sdf = expr_int(df, "x"); 
    Expr* Sdf_sim = expr_simplify(expr_int(df_sim,"x"));   
    char* Sdf_str = expr_to_string(Sdf);
    char* Sdfs_str = expr_to_string(Sdf_sim);
    char* comp_str = (expr_equal(f,Sdf_sim))?"TRUE":"FALSE";
    printf("%s = %s, so we have f(x)==Sf'(x)dx being %s\n",Sdf_str, Sdfs_str, comp_str);
    free(df_str);

    expr_release(df);
    expr_release(f);
    expr_release(Sf);
    expr_release(x);

    printf("--------------------------------------------\n");
    printf(" Example 2: Evaluation\n");
    printf("--------------------------------------------\n");

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

    printf("--------------------------------------------\n");
    printf(" Example 3: Nested expressions\n");
    printf("--------------------------------------------\n");

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

    Expr* Sh = expr_simplify(expr_int(h, "x"));         

    dh_str = expr_to_string(Sh);
    printf("Sh(x) = %s\n", dh_str);
    free(dh_str);   

    expr_release(dh);
    expr_release(h);
    expr_release(x);
    expr_release(x2);
    expr_release(exp_x2);
    expr_release(sinx);

    return 0;
}
