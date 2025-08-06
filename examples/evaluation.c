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
    printf("--------------------------------------------\n");
    printf(" Example: Evaluation\n");
    printf("--------------------------------------------\n");

    ExprIndex y = expr_symbol(&a,"y");
    ExprIndex three_half = expr_number(&a,"3/2");
    ExprIndex term1 = expr_mul(&a,three_half, y);
    ExprIndex logy = expr_func(&a,FUNC_LOG, y);
    ExprIndex g = expr_add(&a,term1, logy);

    printf("g(y) = ");
    expr_print(&a,g);
    printf("\n");

    // Evaluate g(y) at y = 4
    ExprIndex g_val = expr_simplify(&a,expr_substitute(&a, g, "y", "4"));
    printf("g(4) = ");
    expr_print(&a,g_val);
    printf("= %f \n", expr_eval_numeric(&a,g_val));

    return 0;
}
