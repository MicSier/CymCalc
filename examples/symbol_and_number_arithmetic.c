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
    printf(" Example: Number and symbol arithmetic\n");
    printf("----------------------------------------------------\n");
    {
        ExprIndex m = expr_mul(&a, expr_add(&a, expr_symbol(&a,"x"), expr_number(&a,"-7/20")), expr_number(&a,"5"));
        expr_print(&a, m);
        printf(" = ");

        ExprIndex ms = expr_simplify(&a, m);
        expr_print(&a, ms);
        printf("\n");
    }
    {
        ExprIndex m = expr_mul(&a, expr_mul(&a, expr_symbol(&a,"x"), expr_number(&a,"-7/20")), expr_number(&a,"5"));
        expr_print(&a, m);
        printf(" = ");
        ExprIndex ms = expr_simplify(&a, m);
        expr_print(&a, ms);
        printf("\n");
    }

    return 0;
}
