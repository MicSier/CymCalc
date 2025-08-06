#include <stdio.h>
#include <stdlib.h>
#define CYMCALC_IMPLEMENTATION
#include "..\cymcalc.h"

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

    ExprArena a;
    expr_arena_init(&a);
    printf("----------------------------------------------------\n");
    printf(" Example: Symbolic differentiation and integration\n");
    printf("----------------------------------------------------\n");
    {
        ExprIndex x = expr_symbol(&a,"x");
        ExprIndex x3 = expr_pow(&a, x, expr_number(&a,"3"));
        ExprIndex sinx = expr_func(&a, FUNC_SIN, x);
        ExprIndex f = expr_add(&a, x3, sinx);

        printf("f(x) = ");
        expr_print(&a, f);
        printf("\n");
        ExprIndex df = expr_diff(&a, f, "x");  
        ExprIndex df_sim = expr_simplify(&a, df); 

        printf("f'(x) = ");
        expr_print(&a, df);
        printf(" = ");
        expr_print(&a, df_sim);
        printf("\n");
        
        
        ExprIndex Sf = expr_int(&a,f, "x"); 
        ExprIndex Sf_sim = expr_simplify(&a,Sf); 
        
        printf("∫f(x)dx = ");
        expr_print(&a, Sf);
        printf(" = ");
        expr_print(&a, Sf_sim);
        printf("\n");

        ExprIndex Sdf = expr_int(&a,df, "x"); 
        ExprIndex Sdf_sim = expr_simplify(&a,Sdf);   
        expr_print(&a,Sdf);
        printf(" = ");
        expr_print(&a,Sdf_sim);
        char* comp_str = (expr_equal(&a,f,Sdf_sim))?"TRUE":"FALSE";
        printf(", so we have f(x)==∫f'(x)dx being %s\n", comp_str);
    }

    printf("--------------------------------------------\n");
    printf(" Nested expressions\n");
    printf("--------------------------------------------\n");
    {
        ExprIndex x = expr_symbol(&a,"x");
        ExprIndex x2 = expr_pow(&a, x, expr_number(&a,"2"));
        ExprIndex exp_x2 = expr_func(&a, FUNC_EXP, x2);
        ExprIndex sinx = expr_func(&a, FUNC_SIN, x);
        ExprIndex h = expr_mul(&a, sinx, exp_x2);

        printf("h(x) = ");
        expr_print(&a, h);
        printf("\n");

        ExprIndex dh = expr_simplify(&a,expr_diff(&a,h, "x"));
        
        printf("h'(x) = ");
        expr_print(&a, dh);
        printf("\n");

        ExprIndex Sh = expr_simplify(&a,expr_int(&a,h, "x"));         

        printf("∫h(x)dx = ");
        expr_print(&a, Sh);
        printf("\n");
        


    }

    return 0;
}
