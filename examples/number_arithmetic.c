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
    printf(" Example: Number arithmetic\n");
    printf("----------------------------------------------------\n");
    {
        ExprIndex r = expr_add(&a, expr_number(&a,"3"), expr_number(&a,"5"));
        expr_print(&a, r);
        printf(" = ");
        ExprIndex rs = expr_simplify(&a, r);
        expr_print(&a, rs);
        printf("\n");
    }
    {
        ExprIndex t = expr_mul(&a, expr_add(&a, expr_number(&a,"3"), expr_number(&a,"-7/20")), expr_number(&a,"5"));
        expr_print(&a, t);
        printf(" = ");
        ExprIndex ts = expr_simplify(&a, t);
        expr_print(&a, ts);
        printf("\n");
    }
    {
        ExprIndex t = expr_mul(&a, expr_mul(&a, expr_number(&a,"3"), expr_number(&a,"-7/20")), expr_number(&a,"5"));
        expr_print(&a, t);
        printf(" = ");
        ExprIndex ts = expr_simplify(&a, t);
        expr_print(&a, ts);
        printf("\n");
    }

    return 0;
}
