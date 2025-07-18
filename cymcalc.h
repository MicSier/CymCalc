#ifndef CYMCALC_H
#define CYMCALC_H

#include <gmp.h> //"C:\vcpkg\installed\x64-windows-static\include\gmp.h"
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
    EXPR_FUNC
} ExprType;

// Supported functions
typedef enum {
    FUNC_SIN,
    FUNC_COS,
    FUNC_EXP,
    FUNC_LOG
} FuncType;

// Forward declaration
typedef struct Expr Expr;

// Expression node
struct Expr {
    ExprType type;
    int refcount;

    union {
        // For EXPR_NUMBER
        mpq_t value;

        // For EXPR_SYMBOL
        char* name;

        // For binary operators (Add, Mul, Pow)
        struct {
            Expr* left;
            Expr* right;
        } binop;

        // For functions (e.g. sin(x))
        struct {
            FuncType func;
            Expr* arg;
        } func;
    } data;
};

//-----------------------------------------------
// Constructors
//-----------------------------------------------

// Create number (rational constant)
Expr* expr_number(char* num_str); // e.g. "3/4"

// Create symbol
Expr* expr_symbol(char* name);

// Binary operators
Expr* expr_add(Expr* a, Expr* b);
Expr* expr_mul(Expr* a, Expr* b);
Expr* expr_pow(Expr* base, Expr* exponent);

// Function application
Expr* expr_func(FuncType f, Expr* arg);

// Compere Expresions
int expr_equal(const Expr* a, const Expr* b);

//-----------------------------------------------
// Memory Management
//-----------------------------------------------

Expr* expr_retain(Expr* e);
void expr_release(Expr* e);
Expr* expr_copy(const Expr* e);

//-----------------------------------------------
// Core Operations
//-----------------------------------------------

// Symbolic differentiation: d(expr)/d(symbol_name)
Expr* expr_diff(const Expr* e, const char* symbol_name);

// Simplification (dummy, can be implemented as no-op initially)
Expr* expr_simplify(const Expr* e);

// Evaluate expression substituting symbol → number.
// Returns new expression with substituted values.
Expr* expr_substitute(const Expr* e, const char* symbol, const char* value_str);
double expr_eval_numeric(const Expr* e);

Expr* expr_eval(const Expr* e,
                const char* symbol_name,
                const char* value_str);

//-----------------------------------------------
// Printing
//-----------------------------------------------

// Print to string in infix notation.
// The returned string is heap-allocated. Caller must free it.
char* expr_to_string(const Expr* e);

#ifdef __cplusplus
}
#endif

#endif // CYMCALC_H

#ifdef CYMCALC_IMPLEMENTATION

Expr* expr_retain(Expr* e) {
    if (e) {
        e->refcount++;
    }
    return e;
}

Expr* expr_number(char* num_str) {
    Expr* num_expr;
    num_expr = (Expr*) malloc(sizeof(Expr));
    if (!num_expr) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    num_expr->type = EXPR_NUMBER;
    num_expr->refcount = 1;

    mpq_init(num_expr->data.value);

    mpq_t num;
    mpq_init(num);

    if (mpq_set_str(num, num_str, 10) != 0) {
        fprintf(stderr, "Invalid rational string: %s\n", num_str);
        exit(1);
    }
    mpq_canonicalize(num);
    //gmp_printf("Fraction = %Qd\n", num);
    mpq_set(num_expr->data.value, num);

    mpq_clear(num);

    return num_expr;
}

Expr* expr_symbol(char* name) {
    Expr* sym_expr;
    sym_expr = (Expr*) malloc(sizeof(Expr));
    if (!sym_expr) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    sym_expr->type = EXPR_SYMBOL;
    sym_expr->refcount = 1;

    sym_expr->data.name = strdup(name);;

    return sym_expr;
}

Expr* expr_add(Expr* a, Expr* b) {
    Expr* add_expr;
    add_expr = (Expr*) malloc(sizeof(Expr));
    if (!add_expr) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    add_expr->type = EXPR_ADD;
    add_expr->refcount = 1;

    add_expr->data.binop.left  = expr_retain(a);
    add_expr->data.binop.right = expr_retain(b);

    return add_expr;
}

Expr* expr_mul(Expr* a, Expr* b) {
    Expr* mul_expr;
    mul_expr = (Expr*) malloc(sizeof(Expr));
    if (!mul_expr) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    mul_expr->type = EXPR_MUL;
    mul_expr->refcount = 1;

    mul_expr->data.binop.left  = expr_retain(a);
    mul_expr->data.binop.right = expr_retain(b);

    return mul_expr;
}

Expr* expr_pow(Expr* base, Expr* exponent) {
    Expr* pow_expr;
    pow_expr = (Expr*) malloc(sizeof(Expr));
    if (!pow_expr) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    pow_expr->type = EXPR_POW;
    pow_expr->refcount = 1;

    pow_expr->data.binop.left  = expr_retain(base);
    pow_expr->data.binop.right = expr_retain(exponent);

    return pow_expr;
}

Expr* expr_func(FuncType f, Expr* arg) {
    Expr* fun_expr;
    fun_expr = (Expr*) malloc(sizeof(Expr));
    if (!fun_expr) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    fun_expr->type = EXPR_FUNC;
    fun_expr->refcount = 1;
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
    fun_expr->data.func.arg  = expr_retain(arg);

    return fun_expr;
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
        default:
            fprintf(stderr, "Unknown expression type in conversion to string: %d.\n",e->type);
            exit(1);
    }
    return string;
}

char* expr_to_string(const Expr* e) {
    char* s=malloc(1);
    s[0] = '\0';
    return expr_to_string_impl(s, e);
}

void expr_release(Expr* e) {
    if (!e) return;

    e->refcount--;
    if (e->refcount > 0) return;

    switch (e->type) {
        case EXPR_NUMBER:
            mpq_clear(e->data.value);
            break;
        case EXPR_SYMBOL:
            free(e->data.name);
            break;
        case EXPR_ADD:
        case EXPR_MUL:
        case EXPR_POW:
            expr_release(e->data.binop.left);
            expr_release(e->data.binop.right);
            break;
        case EXPR_FUNC:
            expr_release(e->data.func.arg);
            break;
        default:
            fprintf(stderr, "Unknown expression type in release: %d\n", e->type);
            exit(1);
    }
    free(e);
}

Expr* expr_add_numbers(const Expr* a, const Expr* b) {
    Expr* result = malloc(sizeof(Expr));
    result->type = EXPR_NUMBER;
    result->refcount = 1;
    mpq_init(result->data.value);
    mpq_add(result->data.value, a->data.value, b->data.value);
    return result;
}

Expr* expr_mul_numbers(const Expr* a, const Expr* b) {
    Expr* result = malloc(sizeof(Expr));
    result->type = EXPR_NUMBER;
    result->refcount = 1;
    mpq_init(result->data.value);
    mpq_mul(result->data.value, a->data.value, b->data.value);
    return result;
}

/*Expr* expr_pow_numbers(const Expr* a, const Expr* b) {
    Expr* result = malloc(sizeof(Expr));
    result->type = EXPR_NUMBER;
    result->refcount = 1;
    mpq_init(result->data.value);
    mpq_pow(result->data.value, a->data.value, b->data.value);
    return result;
}*/

/*Expr* expr_simplify(const Expr* e) {
    if (!e) return NULL;
    Expr* ec = expr_copy(e);
    switch (e->type) {

        case EXPR_NUMBER:
        case EXPR_SYMBOL:
            return expr_retain(ec);

        case EXPR_ADD: {
            Expr* left = expr_simplify(ec->data.binop.left);
            Expr* right = expr_simplify(ec->data.binop.right);

            // number + number -> number
            if (left->type == EXPR_NUMBER && right->type == EXPR_NUMBER) {
                /*mpq_t num;
                mpq_init(num);
                mpq_add(num, left->data.value, right->data.value);
                ec->type=EXPR_NUMBER;
                mpq_init(ec->data.value);
                mpq_set(ec->data.value, num);
                mpq_clear(num);
                return expr_retain(ec);*//*
                Expr* result = expr_add_numbers(left, right);
                expr_release(left);
                expr_release(right);
                return result;
            }

            // a*x + b*x -> (a+b) * x
            if (left->type == EXPR_MUL &&
                right->type == EXPR_MUL) {
                if (expr_equal(left->data.binop.right, right->data.binop.right)) {
                    Expr* a = left->data.binop.left;
                    Expr* b = right->data.binop.left;
                    Expr* sum = expr_simplify(expr_add(a, b));
                    Expr* result = expr_mul(sum, expr_retain(left->data.binop.right));
                    expr_release(left);
                    expr_release(right);
                    return result;
                }
            }

            // else return ADD(left, right)
            return expr_add(left, right);
        }

        case EXPR_MUL: {
            Expr* left = expr_simplify(ec->data.binop.left);
            Expr* right = expr_simplify(ec->data.binop.right);
            
            // number * number -> number
            if (left->type == EXPR_NUMBER && right->type == EXPR_NUMBER) {
                mpq_t num;
                mpq_init(num);
                mpq_mul(num, left->data.value, right->data.value);
                ec->type=EXPR_NUMBER;
                mpq_init(ec->data.value);
                mpq_set(ec->data.value, num);
                mpq_clear(num);
                return expr_retain(ec);
            }

            // Rule 2: swap number left
            if (left->type == EXPR_SYMBOL || left->type == EXPR_FUNC) {
                if (right->type == EXPR_NUMBER) {
                    Expr* swapped = expr_mul(right, left);
                    expr_release(left);
                    expr_release(right);
                    return expr_simplify(swapped);
                }
            }

            // Rule 1: x^a * x^b
            if (left->type == EXPR_POW &&
                right->type == EXPR_POW) {
                // check if bases match
            }

            return expr_mul(left, right);
        }

        case EXPR_POW: {
            Expr* base = expr_simplify(ec->data.binop.left);
            Expr* exponent = expr_simplify(ec->data.binop.right);
            return expr_pow(base, exponent);
        }

        case EXPR_FUNC: {
            Expr* arg = expr_simplify(ec->data.func.arg);
            return expr_func(ec->data.func.func, arg);
        }

        default:
            fprintf(stderr, "Unknown expression type in simplification.\n");
            exit(1);
    }
}*/

Expr* expr_simplify(const Expr* ec) {
    if (!ec) return NULL;

    switch (ec->type) {

        case EXPR_NUMBER:
        case EXPR_SYMBOL:
            return expr_copy(ec);

        case EXPR_ADD: {
            Expr* left = expr_simplify(ec->data.binop.left);
            Expr* right = expr_simplify(ec->data.binop.right);

            // number + number → number
            if (left->type == EXPR_NUMBER && right->type == EXPR_NUMBER) {
                Expr* result = expr_add_numbers(left, right);
                expr_release(left);
                expr_release(right);
                return result;
            }

            if (left->type == EXPR_NUMBER &&
                mpq_cmp_ui(left->data.value, 0, 1) == 0) {
                expr_release(left);
                return expr_simplify(right);
            }

            if (right->type == EXPR_NUMBER &&
                mpq_cmp_ui(right->data.value, 0, 1) == 0) {
                expr_release(right);
                return expr_simplify(left);
            }

            // a*x + b*x → (a+b)*x
            if (left->type == EXPR_MUL &&
                right->type == EXPR_MUL) {

                Expr* x1 = left->data.binop.right;
                Expr* x2 = right->data.binop.right;

                if (expr_equal(x1, x2)) {
                    Expr* a = left->data.binop.left;
                    Expr* b = right->data.binop.left;

                    Expr* sum = expr_simplify(expr_add(a, b));
                    Expr* result = expr_mul(sum, expr_copy(x1));

                    expr_release(left);
                    expr_release(right);
                    return result;
                }
            }

            // keep as ADD
            Expr* result = expr_add(left, right);
            return result;
        }

        case EXPR_MUL: {
            Expr* left = expr_simplify(ec->data.binop.left);
            Expr* right = expr_simplify(ec->data.binop.right);
            
            // number * number → number
            if (left->type == EXPR_NUMBER && right->type == EXPR_NUMBER) {
                Expr* result = expr_mul_numbers(left, right);
                expr_release(left);
                expr_release(right);
                return result;
            }

            if (left->type == EXPR_NUMBER) {
                if (mpq_cmp_ui(left->data.value, 1, 1) == 0) {
                // 1 * right → right
                expr_release(left);
                return expr_simplify(right);
            }
            if (mpq_cmp_ui(left->data.value, 0, 1) == 0) {
                // 0 * anything → 0
                expr_release(right);
                return expr_number("0");
            }
            }

            // reorder if left is symbol/function and right is number
            if ((left->type == EXPR_SYMBOL || left->type == EXPR_FUNC) &&
                right->type == EXPR_NUMBER) {
                
                Expr* swapped = expr_mul(right, left);
                expr_release(left);
                expr_release(right);
                return expr_simplify(swapped);
            }
            // x * x -> x^2
            if (expr_equal(left, right)) {
                Expr* exponent = expr_number("2");
                Expr* result = expr_pow(left, exponent);
                expr_release(right);
                return expr_simplify(result);
            }

            // x^a * x^b → x^(a+b)
            if (left->type == EXPR_POW &&
                right->type == EXPR_POW) {

                Expr* base1 = left->data.binop.left;
                Expr* base2 = right->data.binop.left;

                if (expr_equal(base1, base2)) {
                    Expr* exp1 = left->data.binop.right;
                    Expr* exp2 = right->data.binop.right;

                    Expr* exp_sum = expr_simplify(expr_add(exp1, exp2));
                    Expr* result = expr_pow(expr_copy(base1), exp_sum);

                    expr_release(left);
                    expr_release(right);
                    return result;
                }
            }

            // keep as MUL
            Expr* result = expr_mul(left, right);
            return result;
        }

        case EXPR_POW: {
            Expr* base = expr_simplify(ec->data.binop.left);
            Expr* exponent = expr_simplify(ec->data.binop.right);
            
            if (exponent->type == EXPR_NUMBER) {
                if (mpq_cmp_ui(exponent->data.value, 0, 1) == 0) {
                    // x^0 = 1
                    expr_release(base);
                    expr_release(exponent);
                    return expr_number("1");
                }
                if (mpq_cmp_ui(exponent->data.value, 1, 1) == 0) {
                    // x^1 = x
                    expr_release(exponent);
                    return expr_simplify(base);
                }
            }
            
            if (base->type == EXPR_NUMBER) {
                if (mpq_cmp_ui(base->data.value, 0, 1) == 0) {
                    // 0^x = 0
                    expr_release(exponent);
                    return expr_number("0");
                }
                if (mpq_cmp_ui(base->data.value, 1, 1) == 0) {
                    // 1^x = 1
                    expr_release(exponent);
                    return expr_number("1");
                }
            }

            /*if(base->type==EXPR_NUMBER && exponent->type==EXPR_NUMBER) {
                Expr* result = expr_mul_numbers(base, exponent);
                expr_release(base);
                expr_release(exponent);
                return result;
            }*/
            return expr_pow(base, exponent);
        }

        case EXPR_FUNC: {
            Expr* arg = expr_simplify(ec->data.func.arg);
            return expr_func(ec->data.func.func, arg);
        }

        default:
            fprintf(stderr, "Unknown expression type in expr_simplify.\n");
            exit(1);
    }
}

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

int expr_equal(const Expr* a, const Expr* b) {
    if (a == b) return 1;
    if (!a || !b) return 0;
    if (a->type != b->type) return 0;

    switch (a->type) {
        case EXPR_NUMBER:
            return (mpq_cmp(a->data.value, b->data.value) == 0);

        case EXPR_SYMBOL:
            return strcmp(a->data.name, b->data.name) == 0;

        case EXPR_ADD:
        case EXPR_MUL:
        case EXPR_POW:
            return expr_equal(a->data.binop.left, b->data.binop.left) &&
                   expr_equal(a->data.binop.right, b->data.binop.right);

        case EXPR_FUNC:
            return (a->data.func.func == b->data.func.func) &&
                   expr_equal(a->data.func.arg, b->data.func.arg);

        default:
            fprintf(stderr, "Unknown expression type in expr_equal.\n");
            exit(1);
    }
}

Expr* expr_substitute(const Expr* e, const char* symbol_name, const char* value_str) {
    if (!e) return NULL;

    switch (e->type) {
        case EXPR_NUMBER:
            return expr_copy(e);

        case EXPR_SYMBOL:
            if (strcmp(e->data.name, symbol_name) == 0) {
                return expr_number(value_str);
            } else {
                return expr_copy(e);
            }

        case EXPR_ADD: {
            Expr* left = expr_substitute(e->data.binop.left, symbol_name, value_str);
            Expr* right = expr_substitute(e->data.binop.right, symbol_name, value_str);
            return expr_add(left, right);
        }

        case EXPR_MUL: {
            Expr* left = expr_substitute(e->data.binop.left, symbol_name, value_str);
            Expr* right = expr_substitute(e->data.binop.right, symbol_name, value_str);
            return expr_mul(left, right);
        }

        case EXPR_POW: {
            Expr* base = expr_substitute(e->data.binop.left, symbol_name, value_str);
            Expr* exponent = expr_substitute(e->data.binop.right, symbol_name, value_str);
            return expr_pow(base, exponent);
        }

        case EXPR_FUNC: {
            Expr* arg = expr_substitute(e->data.func.arg, symbol_name, value_str);
            return expr_func(e->data.func.func, arg);
        }

        default:
            fprintf(stderr, "Unknown expression type in expr_substitute.\n");
            exit(1);
    }
}

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

double expr_eval_numeric(const Expr* e) {
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
            double left = expr_eval_numeric(e->data.binop.left);
            double right = expr_eval_numeric(e->data.binop.right);
            return left + right;
        }

        case EXPR_MUL: {
            double left = expr_eval_numeric(e->data.binop.left);
            double right = expr_eval_numeric(e->data.binop.right);
            return left * right;
        }

        case EXPR_POW: {
            double base = expr_eval_numeric(e->data.binop.left);
            double exponent = expr_eval_numeric(e->data.binop.right);
            return pow(base, exponent);
        }

        case EXPR_FUNC: {
            double arg_val = expr_eval_numeric(e->data.func.arg);
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

Expr* expr_diff(const Expr* e, const char* var_name) {
    if (!e) return NULL;

    switch (e->type) {
        case EXPR_NUMBER:
            return expr_number("0");

        case EXPR_SYMBOL:
            if (strcmp(e->data.name, var_name) == 0) {
                return expr_number("1");
            } else {
                return expr_number("0");
            }

        case EXPR_ADD: {
            Expr* left = expr_diff(e->data.binop.left, var_name);
            Expr* right = expr_diff(e->data.binop.right, var_name);
            return expr_add(left, right);
        }

        case EXPR_MUL: {
            Expr* f = e->data.binop.left;
            Expr* g = e->data.binop.right;

            Expr* df = expr_diff(f, var_name);
            Expr* dg = expr_diff(g, var_name);

            Expr* left_term = expr_mul(df, expr_copy(g));
            Expr* right_term = expr_mul(expr_copy(f), dg);

            return expr_add(left_term, right_term);
        }

        case EXPR_POW: {
            Expr* base = e->data.binop.left;
            Expr* exponent = e->data.binop.right;

            if (exponent->type == EXPR_NUMBER &&
                base->type == EXPR_SYMBOL &&
                strcmp(base->data.name, var_name) == 0) {
                // d/dx x^n = n * x^(n-1)
                mpq_t new_exp;
                mpq_init(new_exp);

                mpq_t one;
                mpq_init(one);
                mpq_set_ui(one, 1, 1);

                mpq_sub(new_exp, exponent->data.value, one);

                mpq_clear(one);

                Expr* new_exp_expr = malloc(sizeof(Expr));
                new_exp_expr->type = EXPR_NUMBER;
                new_exp_expr->refcount = 1;
                mpq_init(new_exp_expr->data.value);
                mpq_set(new_exp_expr->data.value, new_exp);

                Expr* pow_expr = expr_pow(expr_copy(base), new_exp_expr);

                Expr* coeff = malloc(sizeof(Expr));
                coeff->type = EXPR_NUMBER;
                coeff->refcount = 1;
                mpq_init(coeff->data.value);
                mpq_set(coeff->data.value, exponent->data.value);

                mpq_clear(new_exp);

                return expr_mul(coeff, pow_expr);
            } else {
                fprintf(stderr, "Power rule for general expressions not implemented yet.\n");
                exit(1);
            }
        }

        case EXPR_FUNC: {
            Expr* u = e->data.func.arg;
            Expr* du = expr_diff(u, var_name);

            switch (e->data.func.func) {
                case FUNC_SIN: {
                    Expr* cos_u = expr_func(FUNC_COS, expr_copy(u));
                    return expr_mul(cos_u, du);
                }
                case FUNC_COS: {
                    Expr* sin_u = expr_func(FUNC_SIN, expr_copy(u));
                    Expr* neg_sin_u = expr_mul(expr_number("-1"), sin_u);
                    return expr_mul(neg_sin_u, du);
                }
                case FUNC_EXP: {
                    Expr* exp_u = expr_func(FUNC_EXP, expr_copy(u));
                    return expr_mul(exp_u, du);
                }
                case FUNC_LOG: {
                    Expr* reciprocal = expr_pow(expr_copy(u), expr_number("-1"));
                    return expr_mul(reciprocal, du);
                }
                default:
                    fprintf(stderr, "Unknown function in differentiation.\n");
                    exit(1);
            }
        }

        default:
            fprintf(stderr, "Unknown expression type in differentiation.\n");
            exit(1);
    }
}

#endif // CYMCALC_IMPLEMENTATION