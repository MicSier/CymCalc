#ifndef CYMCALC_H
#define CYMCALC_H

#include <gmp.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------
// Symbolic Expression Types
//-----------------------------------------------

typedef enum {
    EXPR_NUMBER,
    EXPR_SYMBOL,
    EXPR_ADD,
    EXPR_MUL,
    EXPR_POW,
    EXPR_FUNC,
    EXPR_DIFF,
    EXPR_INT
} ExprType;

// Supported functions
typedef enum {
    FUNC_SIN,
    FUNC_COS,
    FUNC_EXP,
    FUNC_LOG
} FuncType;

typedef size_t ExprIndex;

typedef struct {
    ExprType type;

    union {
        // EXPR_NUMBER
        mpq_t value;

        // EXPR_SYMBOL
        char* name;

        struct {
            ExprIndex left;
            ExprIndex right;
        } binop;

        struct {
            FuncType func;
            ExprIndex arg;
        } func;

        struct {
            ExprIndex inner;
            char* var;
        } diff;

        struct {
            ExprIndex inner;
            char* var;
        } integral;

    } data;

    // For arena bookkeeping
    bool used;

} Expr;

#define MAX_EXPR_COUNT 10000
#define INVALID_INDEX ((size_t)-1)

typedef struct {
    Expr pool[MAX_EXPR_COUNT];
    int free_list[MAX_EXPR_COUNT]; // indices of free slots
    int free_count;                // number of free slots available
} ExprArena;


//-----------------------------------------------
// Constructors
//-----------------------------------------------

ExprIndex expr_number(ExprArena* arena, char* num_str);
ExprIndex expr_number_mpq(ExprArena* arena, const mpq_t value);
ExprIndex expr_symbol(ExprArena* arena, char* name);
ExprIndex expr_add(ExprArena* arena, ExprIndex left, ExprIndex right);
ExprIndex expr_mul(ExprArena* arena, ExprIndex left, ExprIndex right);
ExprIndex expr_pow(ExprArena* arena, ExprIndex base, ExprIndex exponent) ;
ExprIndex expr_func(ExprArena* arena,FuncType f, ExprIndex arg);

ExprIndex expr_div(ExprArena* arena, ExprIndex numerator, ExprIndex denominator);
ExprIndex expr_sub(const Expr* a, const Expr* b);

// Calculus operators
ExprIndex expr_diff(ExprArena* arena, ExprIndex e, const char* symbol_name);
ExprIndex expr_int(ExprArena* arena, ExprIndex e, const char* symbol_name);

// Unary operators
Expr* expr_neg(const Expr* a);

// Compere Expresions
int expr_equal(const ExprArena* arena, ExprIndex a_idx, ExprIndex b_idx);

//-----------------------------------------------
// Memory Management
//-----------------------------------------------

ExprIndex expr_arena_alloc(ExprArena* arena);

void expr_arena_free(ExprArena* arena, ExprIndex e);
void expr_arena_init(ExprArena* arena);


Expr* expr_copy(const Expr* e);

//-----------------------------------------------
// Core Operations
//-----------------------------------------------


ExprIndex expr_differentiate(ExprArena* arena, const ExprIndex e, const char* symbol_name);
ExprIndex expr_integrate(ExprArena* arena, const ExprIndex e, const char* var);
ExprIndex expr_simplify(ExprArena* arena, const ExprIndex e);

// Evaluate expression substituting symbol → number.
// Returns new expression with substituted values.
ExprIndex expr_substitute(ExprArena* arena, ExprIndex e, const char* symbol, const char* value_str);

double expr_eval_numeric(ExprArena* arena, ExprIndex e);

Expr* expr_eval(const Expr* e,
                const char* symbol_name,
                const char* value_str);


//-----------------------------------------------
// Printing
//-----------------------------------------------

// Print to string in infix notation.
// The returned string is heap-allocated. Caller must free it.
void expr_print(ExprArena* arena, ExprIndex idx);
char* expr_to_string(const Expr* e);

#ifdef __cplusplus
}
#endif

#endif // CYMCALC_H

#ifdef CYMCALC_IMPLEMENTATION

void expr_arena_init(ExprArena* arena) {
    arena->free_count = MAX_EXPR_COUNT;
    for (size_t i = 0; i < MAX_EXPR_COUNT; i++) {
        arena->pool[i].used = false;
        arena->free_list[i] = MAX_EXPR_COUNT - 1 - i;  // Reverse order free list for better locality
    }
}

ExprIndex expr_arena_alloc(ExprArena* arena) {
    if (arena->free_count == 0) {
        fprintf(stderr, "ExprArena out of memory!\n");
        exit(1);
    }
    size_t index = arena->free_list[--arena->free_count];
    Expr* e = &arena->pool[index];
    e->used = true;
    // Clear or init union fields if needed (especially mpq_t)
    memset(&e->data, 0, sizeof(e->data));
    return index;
}

void expr_arena_free(ExprArena* arena, ExprIndex index) {
    if (index == INVALID_INDEX || !arena->pool[index].used) return;
    Expr* e = &arena->pool[index];
    // Free internal allocated data depending on type:
    switch (e->type) {
        case EXPR_NUMBER:
            mpq_clear(e->data.value);
            break;
        case EXPR_SYMBOL:
            free(e->data.name);
            break;
        case EXPR_DIFF:
            free(e->data.diff.var);
            break;
        case EXPR_INT:
            free(e->data.integral.var);
            break;
        default:
            break;
    }
    e->used = false;
    arena->free_list[arena->free_count++] = index;
}

void expr_arena_clear(ExprArena* arena) {
    for (size_t i = 0; i < MAX_EXPR_COUNT; i++) {
        if (arena->pool[i].used) {
            expr_arena_free(arena, i);
        }
    }
}

Expr* expr_at(ExprArena* arena, ExprIndex index) {
    if (index == INVALID_INDEX || index >= MAX_EXPR_COUNT || !arena->pool[index].used) {
        printf("error out of range expr index %d\n",index);
        exit(1);
        //return NULL;
    }
    return &arena->pool[index];
}

ExprIndex expr_number(ExprArena* arena, char* num_str) {
    ExprIndex idx = expr_arena_alloc(arena);
    Expr* num_expr = expr_at(arena,idx);
    num_expr->type = EXPR_NUMBER;

    mpq_init(num_expr->data.value);
    mpq_t num;
    mpq_init(num);

    if (mpq_set_str(num, num_str, 10) != 0) {
        fprintf(stderr, "Invalid rational string: %s\n", num_str);
        exit(1);
    }
    mpq_canonicalize(num);
    mpq_set(num_expr->data.value, num);
    mpq_clear(num);

    return idx;
}

ExprIndex expr_symbol(ExprArena* arena, char* name) {
    ExprIndex idx = expr_arena_alloc(arena);
    Expr* sym_expr = expr_at(arena,idx);
    sym_expr->type = EXPR_SYMBOL;
    sym_expr->data.name = strdup(name);
    if (!sym_expr->data.name) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return idx;
}

ExprIndex expr_add(ExprArena* arena, ExprIndex left, ExprIndex right) {
    ExprIndex idx = expr_arena_alloc(arena);
    Expr* add_expr = expr_at(arena,idx);
    add_expr->type = EXPR_ADD;

    add_expr->data.binop.left  = left;
    add_expr->data.binop.right = right;

    return idx;
}

ExprIndex expr_mul(ExprArena* arena, ExprIndex left, ExprIndex right) {
    ExprIndex idx = expr_arena_alloc(arena);
    Expr* mul_expr= expr_at(arena,idx);

    mul_expr->type = EXPR_MUL;

    mul_expr->data.binop.left  = left;
    mul_expr->data.binop.right = right;

    return idx;
}

ExprIndex expr_pow(ExprArena* arena, ExprIndex base, ExprIndex exponent) {
    ExprIndex idx = expr_arena_alloc(arena);
    Expr* pow_expr= expr_at(arena,idx);

    pow_expr->type = EXPR_POW;

    pow_expr->data.binop.left  = base;
    pow_expr->data.binop.right = exponent;

    return idx;
}

ExprIndex expr_func(ExprArena* arena,FuncType f, ExprIndex arg) {
    ExprIndex idx = expr_arena_alloc(arena);
    Expr* fun_expr = expr_at(arena,idx);

    fun_expr->type = EXPR_FUNC;

    switch (f) {
        case FUNC_SIN: break;
        case FUNC_COS: break;
        case FUNC_EXP: break;
        case FUNC_LOG: break;
        default:
            fprintf(stderr, "Unknown function type: %d\n", f);
            exit(1);
    }
    fun_expr->data.func.func = f;
    fun_expr->data.func.arg  = arg;

    return idx;
}

ExprIndex expr_diff(ExprArena* arena, ExprIndex f, const char* var) {
    //if (!(false) || !var) return NULL;
    ExprIndex idx = expr_arena_alloc(arena);
    Expr* e = expr_at(arena,idx);
    e->type = EXPR_DIFF;
    e->data.diff.inner = f;
    e->data.diff.var = strdup(var);
    return idx;
}

ExprIndex expr_int(ExprArena* arena, ExprIndex f, const char* var) {
    //if (!(false) || !var) return NULL;
    ExprIndex idx = expr_arena_alloc(arena);
    Expr* e = expr_at(arena,idx);
    e->type = EXPR_INT;
    e->data.diff.inner = f;
    e->data.diff.var = strdup(var);
    return idx;
}

void expr_print(ExprArena* arena, ExprIndex idx) {
    Expr* e = expr_at(arena, idx);
    if (!e) {
        printf("<null>");
        return;
    }
    switch (e->type) {
        case EXPR_NUMBER:
            gmp_printf("%Qd", e->data.value);
            break;
        case EXPR_SYMBOL:
            printf("%s", e->data.name);
            break;
        case EXPR_ADD:
            printf("(");
            expr_print(arena, e->data.binop.left);
            printf(" + ");
            expr_print(arena, e->data.binop.right);
            printf(")");
            break;
        case EXPR_MUL:
            printf("(");
            expr_print(arena, e->data.binop.left);
            printf(" * ");
            expr_print(arena, e->data.binop.right);
            printf(")");
            break;
        case EXPR_POW:
            printf("(");
            expr_print(arena, e->data.binop.left);
            printf(" ^ ");
            expr_print(arena, e->data.binop.right);
            printf(")");
            break;
        case EXPR_FUNC:
            switch (e->data.func.func) {
                case FUNC_SIN: printf("sin"); break;
                case FUNC_COS: printf("cos"); break;
                case FUNC_EXP: printf("exp"); break;
                case FUNC_LOG: printf("log"); break;
                default:
                    fprintf(stderr, "Unknown function type: %d\n", e->data.func.func);
                    exit(1);
            }
            printf("(");
            expr_print(arena, e->data.func.arg);
            printf(")");
            break;
        case EXPR_DIFF:
        {
            const char* var = e->data.diff.var;

            printf("d/d%s", var);
            printf("(");
            expr_print(arena, e->data.diff.inner);
            printf(")");
            break;
        }
        case EXPR_INT:
        {
            const char* var = e->data.integral.var;
            printf( "∫");
            printf("(");
            expr_print(arena, e->data.integral.inner);
            printf(")");
            printf( "d%s",var);
            break;
        }
        default:
            printf("<expr_type_%d>", e->type);
    }
}

static inline char* concat(char* s1, const char* s2) {
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    char* result = malloc(len1 + len2 + 1);
    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1);
    free(s1);
    return result;
}
/*
char* expr_to_string_impl(char* string, const Expr* e) {
    const size_t len = strlen(string);
    switch(e->type)
    {
        case EXPR_NUMBER: {
            char* num_str = mpq_get_str(NULL, 10, e->data.value);
            string = concat(string, num_str);
            free(num_str);
            break;
        }

        case EXPR_SYMBOL: {
            string = concat(string, e->data.name);
            break;
        }

        case EXPR_ADD: {
            string = concat(string, "(");
            string = expr_to_string_impl(string, e->data.binop.left);
            string = concat(string, " + ");
            string = expr_to_string_impl(string, e->data.binop.right);
            string = concat(string, ")");
            break;
        }

        case EXPR_MUL: {
            string = concat(string, "(");
            string = expr_to_string_impl(string, e->data.binop.left);
            string = concat(string, " * ");
            string = expr_to_string_impl(string, e->data.binop.right);
            string = concat(string, ")");
            break;
        }

        case EXPR_POW: {
            string = concat(string, "(");
            string = expr_to_string_impl(string, e->data.binop.left);
            string = concat(string, " ^ ");
            string = expr_to_string_impl(string, e->data.binop.right);
            string = concat(string, ")");
            break;
        }

        case EXPR_FUNC: {
            const char* fname = NULL;
            switch (e->data.func.func) {
                case FUNC_SIN: fname = "sin"; break;
                case FUNC_COS: fname = "cos"; break;
                case FUNC_EXP: fname = "exp"; break;
                case FUNC_LOG: fname = "log"; break;
                default:
                    fprintf(stderr, "Unknown function type: %d\n", e->data.func.func);
                    exit(1);
            }
            string = concat(string, fname);
            string = concat(string, "(");
            string = expr_to_string_impl(string, e->data.func.arg);
            string = concat(string, ")");
            break;
        }
        
        case EXPR_DIFF: {
            char* inner_str = expr_to_string(e->data.diff.inner);
            const char* var = e->data.diff.var;

            char* result = malloc(strlen(inner_str) + strlen(var) + 32);
            sprintf(result, "d/d%s(%s)", var, inner_str);

            free(inner_str);
            return result;
        }

        case EXPR_INT: {
            char* inner_str = expr_to_string(e->data.integral.inner);
            const char* var = e->data.integral.var;
            char* result = malloc(strlen(inner_str) + strlen(var) + 16);
            sprintf(result, "∫ %s d%s", inner_str, var);
            free(inner_str);
            return result;
            break;
        }

        default:
            fprintf(stderr, "Unknown expression type in conversion to string: %d.\n",e->type);
            exit(1);
    }
    return string;
}


char* expr_to_string(const Expr* e) {
    char* s=malloc(1);
    s[0] = '\0';
    return expr_to_string_impl(a, s, e);
}
*/

ExprType expr_type(ExprArena* arena, ExprIndex idx) {
    return (expr_at(arena,idx))->type;
}

FuncType expr_ftype(ExprArena* arena, ExprIndex idx) {
    Expr* e = expr_at(arena, idx);
    if (!e || e->type != EXPR_FUNC) {
        fprintf(stderr, "expr_value: not a function\n");
        exit(1);
    }
    return e->data.func.func;
}

ExprIndex expr_arg(ExprArena* arena, ExprIndex idx) {
    Expr* e = expr_at(arena, idx);
    if (!e || e->type != EXPR_FUNC) {
        fprintf(stderr, "expr_value: not a function\n");
        exit(1);
    }
    return e->data.func.arg;
}

mpq_t* expr_value(ExprArena* arena, ExprIndex idx) {
    Expr* e = expr_at(arena, idx);
    if (!e || e->type != EXPR_NUMBER) {
        fprintf(stderr, "expr_value: not a number\n");
        exit(1);
    }
    return &e->data.value;
}

char* expr_name(ExprArena* arena, ExprIndex idx) {
    Expr* e = expr_at(arena, idx);
    if (!e || e->type != EXPR_SYMBOL) {
        fprintf(stderr, "expr_value: not a number\n");
        exit(1);
    }
    return e->data.name;
}

ExprIndex expr_left(ExprArena* arena, ExprIndex idx) {
    Expr* e = expr_at(arena, idx);
    if (!e || (e->type != EXPR_MUL && e->type != EXPR_ADD && e->type != EXPR_POW)) {
        fprintf(stderr, "expr_value: not a binop %d \n", e->type);
        exit(1);
    }
    return e->data.binop.left;
}

ExprIndex expr_right(ExprArena* arena, ExprIndex idx) {
    Expr* e = expr_at(arena, idx);
    if (!e || e->type != EXPR_MUL && e->type != EXPR_ADD && e->type != EXPR_POW) {
        fprintf(stderr, "expr_value: not a binop %d \n", e->type);
        exit(1);
    }
    return e->data.binop.right;
}

ExprIndex expr_add_numbers(ExprArena* arena, ExprIndex a_idx, ExprIndex b_idx) {
    Expr* a = expr_at(arena, a_idx);
    Expr* b = expr_at(arena, b_idx);
    if (!a || !b || a->type != EXPR_NUMBER || b->type != EXPR_NUMBER) {
        fprintf(stderr, "expr_add_numbers: operands must be EXPR_NUMBER\n");
        exit(1);
    }

    ExprIndex result_idx = expr_arena_alloc(arena);
    Expr* result = expr_at(arena, result_idx);
    result->type = EXPR_NUMBER;
    mpq_init(result->data.value);
    mpq_add(result->data.value, a->data.value, b->data.value);
    return result_idx;
}

ExprIndex expr_mul_numbers(ExprArena* arena, ExprIndex a_idx, ExprIndex b_idx) {
    Expr* a = expr_at(arena, a_idx);
    Expr* b = expr_at(arena, b_idx);
    if (!a || !b || a->type != EXPR_NUMBER || b->type != EXPR_NUMBER) {
        fprintf(stderr, "expr_mul_numbers: operands must be EXPR_NUMBER\n");
        exit(1);
    }

    ExprIndex result_idx = expr_arena_alloc(arena);
    Expr* result = expr_at(arena, result_idx);
    result->type = EXPR_NUMBER;
    mpq_init(result->data.value);
    mpq_mul(result->data.value, a->data.value, b->data.value);
    return result_idx;
}

/*Expr* expr_pow_numbers(const Expr* a, const Expr* b) {
    Expr* result = malloc(sizeof(Expr));
    result->type = EXPR_NUMBER;
    result->refcount = 1;
    mpq_init(result->data.value);
    mpq_pow(result->data.value, a->data.value, b->data.value);
    return result;
}*/

const char* func_type_name(FuncType f) {
    switch (f) {
        case FUNC_SIN: return "sin";
        case FUNC_COS: return "cos";
        case FUNC_EXP: return "exp";
        case FUNC_LOG: return "log";
        default:       return "unknown_func";
    }
}

void expr_print_tree_impl(ExprArena* a, ExprIndex idx, int indent, const char* prefix) {
    Expr *e = expr_at(a, idx);
    if (!e) {
        printf("%*s%s(NULL)\n", indent, "", prefix);
        return;
    }

    char buf[256];
    const char* label = "???";

    switch (e->type) {
        case EXPR_NUMBER:
            gmp_snprintf(buf, sizeof(buf), "NUMBER: %Qd", e->data.value);
            label = buf;
            break;

        case EXPR_SYMBOL:
            snprintf(buf, sizeof(buf), "SYMBOL: %s", e->data.name);
            label = buf;
            break;

        case EXPR_ADD:
            label = "ADD";
            break;

        case EXPR_MUL:
            label = "MUL";
            break;

        case EXPR_POW:
            label = "POW";
            break;

        case EXPR_FUNC:
            snprintf(buf, sizeof(buf), "FUNC: %s", func_type_name(e->data.func.func));
            label = buf;
            break;

        case EXPR_DIFF:
            snprintf(buf, sizeof(buf), "DIFF w.r.t. %s", e->data.diff.var);
            label = buf;
            break;

        case EXPR_INT:
            snprintf(buf, sizeof(buf), "INTEGRAL w.r.t. %s", e->data.integral.var);
            label = buf;
            break;
    }

    printf("%*s%s%s\n", indent, "", prefix, label);

    // Recurse on children
    switch (e->type) {
        case EXPR_ADD:
        case EXPR_MUL:
        case EXPR_POW:
            expr_print_tree_impl(a, e->data.binop.left, indent + 4, "├── ");
            expr_print_tree_impl(a, e->data.binop.right, indent + 4, "└── ");
            break;

        case EXPR_FUNC:
            expr_print_tree_impl(a, e->data.func.arg, indent + 4, "└── ");
            break;

        case EXPR_DIFF:
            expr_print_tree_impl(a, e->data.diff.inner, indent + 4, "└── ");
            break;

        case EXPR_INT:
            expr_print_tree_impl(a, e->data.integral.inner, indent + 4, "└── ");
            break;

        default:
            break;  // Nothing to recurse on
    }
}

void expr_print_tree(ExprArena* a, ExprIndex e) {
    expr_print_tree_impl(a, e, 0, "");
}


ExprIndex expr_simplify(ExprArena* a, ExprIndex idx) {
    //if (!e) return NULL;
    Expr* e = expr_at(a,idx);
    switch (e->type) {

        case EXPR_NUMBER:
        case EXPR_SYMBOL:
            return idx;

        case EXPR_ADD: {
            ExprIndex left = expr_simplify(a, e->data.binop.left);
            ExprIndex right = expr_simplify(a, e->data.binop.right);

            // number + number → number
            if (expr_type(a,left) == EXPR_NUMBER && expr_type(a,right) == EXPR_NUMBER) {
                ExprIndex result = expr_add_numbers(a, left, right);
                //expr_release(left);
                //expr_release(right);
                return result;
            }

            if (expr_type(a,left) == EXPR_NUMBER &&
                mpq_cmp_ui(*expr_value(a, left), 0, 1) == 0) {
                //expr_release(e);
                return expr_simplify(a,right);
            }

            if (expr_type(a,right) == EXPR_NUMBER &&
                mpq_cmp_ui(*expr_value(a,right), 0, 1) == 0) {
                //expr_release(eright);
                return expr_simplify(a,left);
            }

            // reorder if left is symbol/function and right is number
            if ((expr_type(a,left) == EXPR_SYMBOL || expr_type(a,left) == EXPR_FUNC ||
                 expr_type(a,left) == EXPR_MUL    || expr_type(a,left) == EXPR_ADD) &&
                expr_type(a,right) == EXPR_NUMBER) {
                
                ExprIndex swapped = expr_add(a,right, left);
                //expr_release(left);
                //expr_release(right);
                return expr_simplify(a,swapped);
            }

            // a*x + b*x → (a+b)*x
            if (expr_type(a, left) == EXPR_MUL &&
                expr_type(a, right) == EXPR_MUL) {
                
                Expr* lmul = expr_at(a, left);
                Expr* rmul = expr_at(a, right);
                
                ExprIndex c = lmul->data.binop.left;
                ExprIndex x1 = lmul->data.binop.right;
                
                ExprIndex b = rmul->data.binop.left;
                ExprIndex x2 = rmul->data.binop.right;
                
                if (expr_equal(a, x1, x2)) {
                    ExprIndex sum = expr_simplify(a, expr_add(a, c, b));
                    ExprIndex result = expr_mul(a, sum, x1);
                    return result;
                }
            }

            // keep as ADD
            ExprIndex result = expr_add(a, left, right);
            return result;
        }

        case EXPR_MUL: {
            ExprIndex left = expr_simplify(a, e->data.binop.left);
            ExprIndex right = expr_simplify(a, e->data.binop.right);
            
            // number * number → number
            if (expr_type(a,left) == EXPR_NUMBER && expr_type(a,right) == EXPR_NUMBER) {
                ExprIndex result = expr_mul_numbers(a, left, right);
                //expr_release(left);
                //expr_release(right);
                return result;
            }
 
            if (expr_type(a,left) == EXPR_NUMBER) {
                if (mpq_cmp_ui(*expr_value(a,left), 1, 1) == 0) {
                // 1 * right → right
                //expr_release(left);
                return expr_simplify(a, right);
            }
            if (mpq_cmp_ui(*expr_value(a,left), 0, 1) == 0) {
                // 0 * anything → 0
                //expr_release(right);
                return expr_number(a,"0");
            }
            }

            // reorder if left is symbol/function and right is number
            if ((expr_type(a,left) == EXPR_SYMBOL || expr_type(a,left) == EXPR_FUNC ||
                 expr_type(a,left) == EXPR_MUL    || expr_type(a,left) == EXPR_ADD) &&
                expr_type(a,right) == EXPR_NUMBER) {
                
                ExprIndex swapped = expr_mul(a,right, left);
                //expr_release(left);
                //expr_release(right);
                return expr_simplify(a,swapped);
            }


            // number * (number * expr) → (number * number) * expr
            if (expr_type(a,left) == EXPR_NUMBER &&
                (expr_type(a,right) == EXPR_MUL )) {
                // multiply constants
                ExprIndex rightleft = expr_left(a, right);
                ExprIndex merged;
                if( expr_type(a,rightleft) == EXPR_NUMBER)                
                    merged = expr_mul_numbers(a,left, rightleft);
                else 
                    merged = expr_mul(a,left,rightleft);
                
                ExprIndex rightright = expr_right(a, right);
                ExprIndex newmul = expr_mul(a, merged, rightright);
                //expr_release(left);
                //expr_release(right);
                return expr_simplify(a,newmul);
            }
            // number * (number + expr) → (number * number) + (number * expr)
            if (expr_type(a,left) == EXPR_NUMBER &&
                (expr_type(a,right) == EXPR_ADD )) {
                // multiply constants
                ExprIndex rightleft = expr_left(a, right);
                ExprIndex merged;
                if( expr_type(a,rightleft) == EXPR_NUMBER)                
                    merged = expr_mul_numbers(a,left, rightleft);
                else 
                    merged = expr_mul(a,left,rightleft);
                
                ExprIndex rightright = expr_mul(a,left,expr_right(a, right));
                ExprIndex newmul = expr_add(a, merged, rightright);
                //expr_release(left);
                //expr_release(right);
                return expr_simplify(a,newmul);
            }
            
            // x * x -> x^2
            if (expr_equal(a, left, right)) {
                ExprIndex exponent = expr_number(a,"2");
                ExprIndex result = expr_pow(a, left, exponent);
                //expr_release(right);
                return expr_simplify(a,result);
            }
            
            // x^a * x^b → x^(a+b)
            if (expr_type(a,left) == EXPR_POW &&
                expr_type(a,right) == EXPR_POW) {

                ExprIndex base1 = expr_left(a,left);
                ExprIndex base2 = expr_left(a,right);

                if (expr_equal(a, base1, base2)) {
                    ExprIndex exp1 = expr_right(a,left);
                    ExprIndex exp2 = expr_right(a,right);

                    ExprIndex exp_sum = expr_simplify(a,expr_add(a,exp1, exp2));
                    ExprIndex result = expr_pow(a, base1, exp_sum); //expr_copy(base1)

                    //expr_release(left);
                    //expr_release(right);
                    return result;
                }
            }

            // keep as MUL
            ExprIndex result = expr_mul(a, left, right);
            return result;
        }

        case EXPR_POW: {
            ExprIndex base = expr_simplify(a,e->data.binop.left);
            ExprIndex exponent = expr_simplify(a,e->data.binop.right);
            
            if (expr_type(a, exponent) == EXPR_NUMBER) {
                if (mpq_cmp_ui(*expr_value(a, exponent), 0, 1) == 0) {
                    // x^0 = 1
                    //expr_release(base);
                    //expr_release(exponent);
                    return expr_number(a, "1");
                }
                if (mpq_cmp_ui(*expr_value(a, exponent), 1, 1) == 0) {
                    // x^1 = x
                    //expr_release(exponent);
                    return expr_simplify(a, base);
                }
            }
            
            if (expr_type(a,base) == EXPR_NUMBER) {
                if (mpq_cmp_ui(*expr_value(a,base), 0, 1) == 0) {
                    // 0^x = 0
                    //expr_release(exponent);
                    return expr_number(a,"0");
                }
                if (mpq_cmp_ui(*expr_value(a,base), 1, 1) == 0) {
                    // 1^x = 1
                    //expr_release(exponent);
                    return expr_number(a,"1");
                }
            }

            //if(base->type==EXPR_NUMBER && exponent->type==EXPR_NUMBER) {
            //    Expr* result = expr_mul_numbers(base, exponent);
            //    expr_release(base);
            //    expr_release(exponent);
            //    return result;
            //}
            
            return expr_pow(a, base, exponent);
        }
        
        case EXPR_FUNC: {
            ExprIndex arg = expr_simplify(a, e->data.func.arg);
            return expr_func(a, e->data.func.func, arg);
        }

        case EXPR_DIFF: {
            ExprIndex inner = e->data.diff.inner;
            const char* var = e->data.diff.var;
            
            ExprIndex simplified_inner = expr_simplify(a, inner);

            // Try to fully evaluate the derivative using existing engine
            ExprIndex attempted = expr_differentiate(a, simplified_inner, var);
            if (attempted != INVALID_INDEX) return expr_simplify(a,attempted);
               
            // Return symbolic diff since it couldn’t be evaluated
            return expr_diff(a, simplified_inner, var);
        }

        case EXPR_INT: {
            ExprIndex inner = e->data.integral.inner;
            const char* var = e->data.integral.var;

            ExprIndex simp_inner = expr_simplify(a, inner);

            // Try to evaluate with existing integration engine
            ExprIndex attempted = expr_integrate(a, simp_inner, var);
            if (attempted != INVALID_INDEX) return expr_simplify(a, attempted);
            return expr_int(a, simp_inner, var);
            
        }

        default:
            fprintf(stderr, "Unknown expression type in expr_simplify.\n");
            exit(1);
    }
}
/*
Expr* expr_copy(const Expr* e) {
    if (!e) return NULL;

    switch (e->type) {

        case EXPR_NUMBER: {
            // allocate new node
            Expr* copy = malloc(sizeof(Expr));
            if (!copy) {
                fprintf(stderr, "Out of memory\n");
                exit(1);
            }
            copy->type = EXPR_NUMBER;
            copy->refcount = 1;
            mpq_init(copy->data.value);
            mpq_set(copy->data.value, e->data.value);
            return copy;
        }

        case EXPR_SYMBOL: {
            Expr* copy = malloc(sizeof(Expr));
            if (!copy) {
                fprintf(stderr, "Out of memory\n");
                exit(1);
            }
            copy->type = EXPR_SYMBOL;
            copy->refcount = 1;
            copy->data.name = strdup(e->data.name);
            if (!copy->data.name) {
                fprintf(stderr, "Out of memory\n");
                exit(1);
            }
            return copy;
        }

        case EXPR_ADD:
        case EXPR_MUL:
        case EXPR_POW: {
            Expr* left_copy = expr_copy(e->data.binop.left);
            Expr* right_copy = expr_copy(e->data.binop.right);
            Expr* copy = malloc(sizeof(Expr));
            if (!copy) {
                fprintf(stderr, "Out of memory\n");
                exit(1);
            }
            copy->type = e->type;
            copy->refcount = 1;
            copy->data.binop.left = left_copy;
            copy->data.binop.right = right_copy;
            return copy;
        }

        case EXPR_FUNC: {
            Expr* arg_copy = expr_copy(e->data.func.arg);
            Expr* copy = malloc(sizeof(Expr));
            if (!copy) {
                fprintf(stderr, "Out of memory\n");
                exit(1);
            }
            copy->type = EXPR_FUNC;
            copy->refcount = 1;
            copy->data.func.func = e->data.func.func;
            copy->data.func.arg = arg_copy;
            return copy;
        }

        default:
            fprintf(stderr, "Unknown expression type in copy: %d\n", e->type);
            exit(1);
    }
}
*/
int expr_equal(const ExprArena* arena, ExprIndex a_idx, ExprIndex b_idx) {
    if (a_idx == b_idx) return 1;
    if (a_idx < 0 || b_idx < 0) return 0;

    const Expr* a = expr_at(arena, a_idx);
    const Expr* b = expr_at(arena, b_idx);
    if (!a || !b) return 0;

    if (a->type != b->type) return 0;

    switch (a->type) {
        case EXPR_NUMBER:
            return mpq_cmp(a->data.value, b->data.value) == 0;

        case EXPR_SYMBOL:
            return strcmp(a->data.name, b->data.name) == 0;

        case EXPR_ADD:
        case EXPR_MUL:
        case EXPR_POW:
            return expr_equal(arena, a->data.binop.left, b->data.binop.left) &&
                   expr_equal(arena, a->data.binop.right, b->data.binop.right);

        case EXPR_FUNC:
            return a->data.func.func == b->data.func.func &&
                   expr_equal(arena, a->data.func.arg, b->data.func.arg);

        case EXPR_DIFF:
            return strcmp(a->data.diff.var, b->data.diff.var) == 0 &&
                   expr_equal(arena, a->data.diff.inner, b->data.diff.inner);

        case EXPR_INT:
            return strcmp(a->data.integral.var, b->data.integral.var) == 0 &&
                   expr_equal(arena, a->data.integral.inner, b->data.integral.inner);

        default:
            fprintf(stderr, "Unknown expression type in expr_equal: %d\n", a->type);
            exit(1);
    }
}


ExprIndex expr_substitute(ExprArena* a, ExprIndex idx, const char* symbol_name, const char* value_str){
    Expr* e = expr_at(a,idx);
    if (!e) return INVALID_INDEX;

    switch (e->type) {
      
        case EXPR_NUMBER:
            return idx;
  
        case EXPR_SYMBOL:
            if (strcmp(e->data.name, symbol_name) == 0) {
                return expr_number(a,value_str);
            } else {
                return idx;
            }

        case EXPR_ADD: {
            ExprIndex left  = expr_substitute(a, expr_left(a,idx) , symbol_name, value_str);
            ExprIndex right = expr_substitute(a, expr_right(a,idx), symbol_name, value_str);
            return expr_add(a, left, right);
        }

        case EXPR_MUL: {
            ExprIndex left  = expr_substitute(a, expr_left(a,idx) , symbol_name, value_str);
            ExprIndex right = expr_substitute(a, expr_right(a,idx), symbol_name, value_str);
            return expr_mul(a, left, right);
        }

        case EXPR_POW: {
            ExprIndex base = expr_substitute(a,expr_left(a,idx), symbol_name, value_str);
            ExprIndex exponent = expr_substitute(a,expr_right(a,idx), symbol_name, value_str);
            return expr_pow(a, base, exponent);
        }

        case EXPR_FUNC: {
            ExprIndex arg = expr_substitute(a, expr_arg(a,idx), symbol_name, value_str);
            return expr_func(a, expr_ftype(a,idx), arg);
        }
 
        default:
            fprintf(stderr, "Unknown expression type in expr_substitute.\n");
            exit(1);
    }
           
}
/*
Expr* expr_eval(const Expr* e, 
                const char* symbol_name, 
                const char* value_str) {
    if (!e) return NULL;

    switch (e->type) {
        case EXPR_NUMBER:
            return expr_copy(e);

        case EXPR_SYMBOL:
            if (strcmp(e->data.name, symbol_name) == 0) {
                // Replace symbol with number
                return expr_number(value_str);
            } else {
                return expr_copy(e);
            }

        case EXPR_ADD: {
            Expr* left = expr_eval(e->data.binop.left, symbol_name, value_str);
            Expr* right = expr_eval(e->data.binop.right, symbol_name, value_str);

            if (left->type == EXPR_NUMBER && right->type == EXPR_NUMBER) {
                Expr* result = expr_add_numbers(left, right);
                expr_release(left);
                expr_release(right);
                return result;
            } else {
                Expr* result = expr_add(left, right);
                return result;
            }
        }

        case EXPR_MUL: {
            Expr* left = expr_eval(e->data.binop.left, symbol_name, value_str);
            Expr* right = expr_eval(e->data.binop.right, symbol_name, value_str);

            if (left->type == EXPR_NUMBER && right->type == EXPR_NUMBER) {
                Expr* result = malloc(sizeof(Expr));
                result->type = EXPR_NUMBER;
                result->refcount = 1;
                mpq_init(result->data.value);
                mpq_mul(result->data.value, left->data.value, right->data.value);

                expr_release(left);
                expr_release(right);
                return result;
            } else {
                Expr* result = expr_mul(left, right);
                return result;
            }
        }

        case EXPR_POW: {
            Expr* base = expr_eval(e->data.binop.left, symbol_name, value_str);
            Expr* exponent = expr_eval(e->data.binop.right, symbol_name, value_str);

            // For now, we keep powers symbolic
            Expr* result = expr_pow(base, exponent);
            return result;
        }

        case EXPR_FUNC: {
            Expr* arg = expr_eval(e->data.func.arg, symbol_name, value_str);

        if (arg->type == EXPR_NUMBER) {
            double x = mpq_get_d(arg->data.value);
            double result_val;

            switch (e->data.func.func) {
                case FUNC_SIN: result_val = sin(x); break;
                case FUNC_COS: result_val = cos(x); break;
                case FUNC_EXP: result_val = exp(x); break;
                case FUNC_LOG: result_val = log(x); break;
                default:
                    fprintf(stderr, "Unsupported function type: %d\n", e->data.func.func);
                    exit(1);
            }

            Expr* result = malloc(sizeof(Expr));
            result->type = EXPR_NUMBER;
            result->refcount = 1;
            mpq_init(result->data.value);
            mpq_set_d(result->data.value, result_val); // approximate
            expr_release(arg);
            return result;
        }

            Expr* result = expr_func(e->data.func.func, arg);
            return result;
        }

        default:
            fprintf(stderr, "Unknown expression type in expr_eval.\n");
            exit(1);
    }
}
*/
double expr_eval_numeric(ExprArena* a, ExprIndex idx) {
    Expr* e = expr_at(a,idx);
    if (!e) {
        fprintf(stderr, "expr_eval_numeric called with NULL expression.\n");
        exit(1);
    }
    

    switch (e->type) {
        
        case EXPR_NUMBER:
            return mpq_get_d(e->data.value);

        case EXPR_SYMBOL:
            fprintf(stderr, "Cannot evaluate expression with free symbol: %s\n", e->data.name);
            exit(1);

        case EXPR_ADD: {
            double left  = expr_eval_numeric(a, expr_left(a,idx) );
            double right = expr_eval_numeric(a, expr_right(a,idx));
            return left + right;
        }

        case EXPR_MUL: {
            double left  = expr_eval_numeric(a, expr_left(a,idx) );
            double right = expr_eval_numeric(a, expr_right(a,idx));
            return left * right;
        }

        case EXPR_POW: {
            double base     = expr_eval_numeric(a, expr_left(a,idx) );
            double exponent = expr_eval_numeric(a, expr_right(a,idx));
            return pow(base, exponent);
        }

        case EXPR_FUNC: {
            double arg_val = expr_eval_numeric(a, expr_arg(a,idx));
            switch (e->data.func.func) {
                case FUNC_SIN: return sin(arg_val);
                case FUNC_COS: return cos(arg_val);
                case FUNC_EXP: return exp(arg_val);
                case FUNC_LOG: return log(arg_val);
                default:
                    fprintf(stderr, "Unknown function in expr_eval_numeric.\n");
                    exit(1);
            }
        }

        default:
            fprintf(stderr, "Unknown expression type in expr_eval_numeric.\n");
            exit(1);
    }
            
}

ExprIndex expr_differentiate(ExprArena* a, ExprIndex idx, const char* var_name) {
    Expr* e = expr_at(a, idx);
    if (!e) return INVALID_INDEX;

    switch (e->type) {
        case EXPR_NUMBER:
            return expr_number(a, "0");

        case EXPR_SYMBOL:
            if (strcmp(e->data.name, var_name) == 0) {
                return expr_number(a,"1");
            } else {
                return expr_number(a,"0");
            }

        case EXPR_ADD: {
            ExprIndex left = expr_differentiate(a,e->data.binop.left, var_name);
            ExprIndex right = expr_differentiate(a,e->data.binop.right, var_name);
            return expr_add(a,left, right);
        }

        case EXPR_MUL: {
            ExprIndex f = e->data.binop.left;
            ExprIndex g = e->data.binop.right;

            ExprIndex df = expr_differentiate(a,f, var_name);
            ExprIndex dg = expr_differentiate(a,g, var_name);

            ExprIndex left_term = expr_mul(a,df, g);
            ExprIndex right_term = expr_mul(a,f, dg);

            return expr_add(a, left_term, right_term);
        }


        case EXPR_POW: {
            ExprIndex base = e->data.binop.left;
            ExprIndex exponent = e->data.binop.right;

            if (expr_type(a,exponent) == EXPR_NUMBER &&
                expr_type(a,base) == EXPR_SYMBOL &&
                strcmp(expr_name(a,base), var_name) == 0) {
                // d/dx x^n = n * x^(n-1)
                mpq_t new_exp;
                mpq_init(new_exp);

                mpq_t one;
                mpq_init(one);
                mpq_set_ui(one, 1, 1);

                mpq_sub(new_exp, *expr_value(a,exponent), one);

                ExprIndex new_exp_idx = expr_number_mpq(a, new_exp);
                mpq_clear(new_exp);

                ExprIndex x_pow_n_minus_1 = expr_pow(a, base, new_exp_idx);
                ExprIndex coeff_n = expr_number_mpq(a, *expr_value(a,exponent));

                return expr_mul(a, coeff_n, x_pow_n_minus_1);


            } else {
                fprintf(stderr, "Power rule for general expressions not implemented yet.\n");
                return INVALID_INDEX;
            }
        }

        case EXPR_FUNC: {
            ExprIndex u = e->data.func.arg;
            ExprIndex du = expr_differentiate(a,u, var_name);

            switch (e->data.func.func) {
                case FUNC_SIN: {
                    ExprIndex cos_u = expr_func(a, FUNC_COS, u);
                    return expr_mul(a, cos_u, du);
                }
                case FUNC_COS: {
                    ExprIndex sin_u = expr_func(a, FUNC_SIN, u);
                    ExprIndex neg_sin_u = expr_mul(a,expr_number(a,"-1"), sin_u);
                    return expr_mul(a,neg_sin_u, du);
                }
                case FUNC_EXP: {
                    ExprIndex exp_u = expr_func(a,FUNC_EXP, u);
                    return expr_mul(a,exp_u, du);
                }
                case FUNC_LOG: {
                    ExprIndex reiprocal = expr_pow(a,u, expr_number(a,"-1"));
                    return expr_mul(a,reiprocal, du);
                }
                default:
                    fprintf(stderr, "Unknown function in differentiation.\n");
                    exit(1);
            }
        }
        default:
            fprintf(stderr, "Unknown expression type in differentiation.\n");
            expr_print_tree(a,idx);
            exit(1);
    }
}

ExprIndex expr_number_mpq(ExprArena* a, const mpq_t value) {
    ExprIndex idx = expr_arena_alloc(a);
    Expr* e = expr_at(a,idx);
    e->type = EXPR_NUMBER;
    mpq_init(e->data.value);
    mpq_set(e->data.value, value);
    return idx;
}
/*
void mpq_add_ui(mpq_t rop, const mpq_t op1, unsigned long int op2) {
    mpq_t temp;
    mpq_init(temp);
    mpq_set_ui(temp, op2, 1);
    mpq_add(rop, op1, temp);
    mpq_clear(temp);
}
*/
ExprIndex expr_integrate(ExprArena* a, const ExprIndex idx, const char* var_name){
    Expr* e = expr_at(a,idx);
    if (!e) return INVALID_INDEX;

    switch (e->type) {
        
        case EXPR_NUMBER:
            return expr_mul(a,idx,expr_symbol(a, var_name));

        case EXPR_SYMBOL:
            if (strcmp(e->data.name, var_name) == 0) {
                ExprIndex res = expr_mul(a,expr_number(a,"1/2"),expr_pow(a,expr_symbol(a,var_name), expr_number(a,"2")));
                expr_print_tree(a,res);
                return res;
            } else {
                return expr_mul(a,idx,expr_symbol(a,var_name));
            }

        case EXPR_ADD: {
            ExprIndex left = expr_integrate(a,e->data.binop.left, var_name);
            ExprIndex right = expr_integrate(a,e->data.binop.right, var_name);
            return expr_add(a,left, right);
        }

        case EXPR_MUL: {
            ExprIndex f = e->data.binop.left;
            ExprIndex g = e->data.binop.right;

           /* if (expr_type(a,g) == EXPR_NUMBER) {
                ExprIndex inner = expr_integrate(a, f, var_name);
                return expr_mul(a,g, inner);
            }*/

            if (expr_type(a,f) == EXPR_NUMBER) {
                ExprIndex inner = expr_integrate(a, g, var_name);
                return expr_mul(a,f, inner);
            }

            break; // product rule not implemented

        }

        case EXPR_POW: {
            ExprIndex base = e->data.binop.left;
            ExprIndex exponent = e->data.binop.right;
        
            if (expr_type(a, exponent) == EXPR_NUMBER &&
                expr_type(a, base) == EXPR_SYMBOL &&
                strcmp(expr_name(a, base), var_name) == 0) {
                
                // Create one: 1/1
                ExprIndex one = expr_number(a, "1");
                
                // new_exp = exponent + 1
                ExprIndex new_exp = expr_add(a, exponent, one);
                new_exp = expr_simplify(a, new_exp);
                
                // x^(n+1)
                ExprIndex pow_expr = expr_pow(a, base, new_exp);
                
                // 1 / (n+1)
                ExprIndex coeff = expr_div(a, one, new_exp);
                
                //expr_release(a, one);
                //expr_release(a, new_exp);
                
                return expr_mul(a, coeff, pow_expr);
            } else {
                fprintf(stderr, "Power rule for general expressions not implemented yet.\n");
                return INVALID_INDEX;
            }
        }


        case EXPR_FUNC: {
            ExprIndex u = e->data.func.arg;
            //Expr* du = expr_diff(u, var_name);
            if (expr_type(a,u) == EXPR_SYMBOL && strcmp(expr_name(a,u), var_name) == 0) {
                switch (e->data.func.func) {
                    case FUNC_SIN: {
                        ExprIndex cos_u = expr_func(a,FUNC_COS, u);
                        ExprIndex neg_cos_u = expr_mul(a,expr_number(a,"-1"), cos_u);
                        return neg_cos_u;
                    }
                    case FUNC_COS: {
                        ExprIndex sin_u = expr_func(a,FUNC_SIN, u);
                        return sin_u;
                    }
                    case FUNC_EXP: {
                        ExprIndex exp_u = expr_func(a,FUNC_EXP, u);
                        return exp_u;
                    }
                    case FUNC_LOG: {
                        ExprIndex reiprocal = expr_pow(a,u, expr_number(a,"-1"));
                        return reiprocal;
                    }
                    default:
                        fprintf(stderr, "Unknown function in Integration.\n");
                        exit(1);
                }
            }

        }

        default:
            fprintf(stderr, "Unknown expression type in Integration.\n");
            expr_print_tree(a,idx);
            exit(1);
    }

    fprintf(stderr, "Couldn't resolve integration for expression: ");
    expr_print(a,idx);
    fprintf(stderr, "\n");
    return INVALID_INDEX;
}
/*
Expr* expr_neg(const Expr* a) {
    if (!a) return NULL;

    if (a->type == EXPR_NUMBER) {
        mpq_t neg_value;
        mpq_init(neg_value);
        mpq_neg(neg_value, a->data.value);
        Expr* neg = expr_number_mpq(neg_value);
        mpq_clear(neg_value);
        return neg;
    }

    Expr* minus1 = expr_number("-1");
    Expr* result = expr_mul(minus1, expr_retain(a));
    expr_release(minus1);
    return expr_simplify(result);
}

Expr* expr_sub(const Expr* a, const Expr* b) {
    if (!a || !b) return NULL;

    Expr* neg_b = expr_neg(b);
    Expr* result = expr_add(expr_retain(a), neg_b);
    expr_release(neg_b);
    return expr_simplify(result);
}
*/
ExprIndex expr_div(ExprArena* a, ExprIndex num, ExprIndex den) {
    if (num == INVALID_INDEX || den == INVALID_INDEX)
        return INVALID_INDEX;

    Expr* en = expr_at(a, num);
    Expr* ed = expr_at(a, den);

    // a / 1 = a
    if (expr_type(a, den) == EXPR_NUMBER &&
        mpq_cmp_ui(*expr_value(a, den), 1, 1) == 0) {
        return num;
    }

    // 0 / b = 0 (as long as b ≠ 0)
    if (expr_type(a, num) == EXPR_NUMBER &&
        mpq_sgn(*expr_value(a, num)) == 0) {
        return expr_number(a, "0");
    }

    // a / a = 1
    if (expr_equal(a, num, den)) {
        return expr_number(a, "1");
    }

    // both numeric → directly divide
    if (expr_type(a, num) == EXPR_NUMBER &&
        expr_type(a, den) == EXPR_NUMBER) {
        mpq_t q;
        mpq_init(q);
        mpq_div(q, *expr_value(a, num), *expr_value(a, den));
        ExprIndex result = expr_number_mpq(a, q);
        mpq_clear(q);
        return result;
    }

    // a / b → a * b^(-1)
    ExprIndex minus_one = expr_number(a, "-1");
    ExprIndex reciprocal = expr_pow(a, den, minus_one);
    ExprIndex result = expr_mul(a, num, reciprocal);
    //expr_release(a, reciprocal);
    return expr_simplify(a, result);
}

#endif // CYMCALC_IMPLEMENTATION