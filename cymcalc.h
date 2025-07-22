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

        struct {
            Expr* inner;   // Expression being differentiated
            char* var;     // Variable name
        } diff;

        struct {
            Expr* inner;   // Expression being integrated
            char* var;     // Variable of integration
        } integral;

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
Expr* expr_div(const Expr* numerator, const Expr* denominator);
Expr* expr_sub(const Expr* a, const Expr* b);

// Calculus operators
Expr* expr_diff(Expr* e, const char* symbol_name);
Expr* expr_int(Expr* e, const char* var);

// Unary operators
Expr* expr_neg(const Expr* a);

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


Expr* expr_differentiate(const Expr* e, const char* symbol_name);
Expr* expr_integrate(const Expr* e, const char* var);
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

Expr* expr_diff(Expr* f, const char* var) {
    if (!f || !var) return NULL;
    Expr* e = malloc(sizeof(Expr));
    e->type = EXPR_DIFF;
    e->refcount = 1;
    e->data.diff.inner = expr_retain(f);
    e->data.diff.var = strdup(var);
    return e;
}

Expr* expr_int(Expr* f, const char* var) {
    if (!f || !var) return NULL;
    Expr* e = malloc(sizeof(Expr));
    e->type = EXPR_INT;
    e->refcount = 1;
    e->data.integral.inner = expr_retain(f);
    e->data.integral.var = strdup(var);
    return e;
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
        case EXPR_DIFF:
            expr_release(e->data.diff.inner);
        break;
        case EXPR_INT:
            expr_release(e->data.integral.inner);
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
    Expr* e = expr_copy(e);
    switch (e->type) {

        case EXPR_NUMBER:
        case EXPR_SYMBOL:
            return expr_retain(e);

        case EXPR_ADD: {
            Expr* left = expr_simplify(e->data.binop.left);
            Expr* right = expr_simplify(e->data.binop.right);

            // number + number -> number
            if (left->type == EXPR_NUMBER && right->type == EXPR_NUMBER) {
                /*mpq_t num;
                mpq_init(num);
                mpq_add(num, left->data.value, right->data.value);
                e->type=EXPR_NUMBER;
                mpq_init(e->data.value);
                mpq_set(e->data.value, num);
                mpq_clear(num);
                return expr_retain(e);*//*
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
            Expr* left = expr_simplify(e->data.binop.left);
            Expr* right = expr_simplify(e->data.binop.right);
            
            // number * number -> number
            if (left->type == EXPR_NUMBER && right->type == EXPR_NUMBER) {
                mpq_t num;
                mpq_init(num);
                mpq_mul(num, left->data.value, right->data.value);
                e->type=EXPR_NUMBER;
                mpq_init(e->data.value);
                mpq_set(e->data.value, num);
                mpq_clear(num);
                return expr_retain(e);
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
                // chek if bases match
            }

            return expr_mul(left, right);
        }

        case EXPR_POW: {
            Expr* base = expr_simplify(e->data.binop.left);
            Expr* exponent = expr_simplify(e->data.binop.right);
            return expr_pow(base, exponent);
        }

        case EXPR_FUNC: {
            Expr* arg = expr_simplify(e->data.func.arg);
            return expr_func(e->data.func.func, arg);
        }

        default:
            fprintf(stderr, "Unknown expression type in simplification.\n");
            exit(1);
    }
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

void expr_print_tree_impl(const Expr* e, int indent, const char* prefix) {
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
            expr_print_tree_impl(e->data.binop.left, indent + 4, "├── ");
            expr_print_tree_impl(e->data.binop.right, indent + 4, "└── ");
            break;

        case EXPR_FUNC:
            expr_print_tree_impl(e->data.func.arg, indent + 4, "└── ");
            break;

        case EXPR_DIFF:
            expr_print_tree_impl(e->data.diff.inner, indent + 4, "└── ");
            break;

        case EXPR_INT:
            expr_print_tree_impl(e->data.integral.inner, indent + 4, "└── ");
            break;

        default:
            break;  // Nothing to recurse on
    }
}

void expr_print_tree(const Expr* e) {
    expr_print_tree_impl(e, 0, "");
}


Expr* expr_simplify(const Expr* e) {
    if (!e) return NULL;

    switch (e->type) {

        case EXPR_NUMBER:
        case EXPR_SYMBOL:
            return expr_copy(e);

        case EXPR_ADD: {
            Expr* left = expr_simplify(e->data.binop.left);
            Expr* right = expr_simplify(e->data.binop.right);

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
            Expr* left = expr_simplify(e->data.binop.left);
            Expr* right = expr_simplify(e->data.binop.right);
            
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

            // a * b * x -> c * x
            // number * (number * expr) → (number * number) * expr
            if (left->type == EXPR_NUMBER &&
                right->type == EXPR_MUL &&
                right->data.binop.left->type == EXPR_NUMBER) {
                // multiply constants
                Expr* merged = expr_mul_numbers(left, right->data.binop.left);
                Expr* newmul = expr_mul(merged, expr_retain(right->data.binop.right));
                expr_release(left);
                expr_release(right);
                return expr_simplify(newmul);
            }

            // a * x * b -> c * x
            if (right->type == EXPR_NUMBER &&
                left->type == EXPR_MUL &&
                left->data.binop.right->type == EXPR_NUMBER) {
                
                // multiply constants
                Expr* merged = expr_mul_numbers(right, left->data.binop.right);
                Expr* newmul = expr_mul(merged, expr_retain(left->data.binop.left));
                
                expr_release(left);
                expr_release(right);
                return expr_simplify(newmul);
            }

            // keep as MUL
            Expr* result = expr_mul(left, right);
            return result;
        }

        case EXPR_POW: {
            Expr* base = expr_simplify(e->data.binop.left);
            Expr* exponent = expr_simplify(e->data.binop.right);
            
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
            Expr* arg = expr_simplify(e->data.func.arg);
            return expr_func(e->data.func.func, arg);
        }

        case EXPR_DIFF: {
            const Expr* inner = e->data.diff.inner;
            const char* var = e->data.diff.var;

            Expr* simplified_inner = expr_simplify(inner);

            // Try to fully evaluate the derivative using existing engine
            Expr* attempted = expr_differentiate(simplified_inner, var);
            if (attempted) return attempted;
                
            // Return symbolic diff since it couldn’t be evaluated
            return expr_diff(simplified_inner, var);
        }

        case EXPR_INT: {
            const Expr* inner = e->data.integral.inner;
            const char* var = e->data.integral.var;

            Expr* simp_inner = expr_simplify(inner);

            // Try to evaluate with existing integration engine
            Expr* attempted = expr_integrate(simp_inner, var);
            if (attempted) return expr_simplify(attempted);
            return expr_int(simp_inner, var);
            
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

Expr* expr_differentiate(const Expr* e, const char* var_name) {
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
            Expr* left = expr_differentiate(e->data.binop.left, var_name);
            Expr* right = expr_differentiate(e->data.binop.right, var_name);
            return expr_add(left, right);
        }

        case EXPR_MUL: {
            Expr* f = e->data.binop.left;
            Expr* g = e->data.binop.right;

            Expr* df = expr_differentiate(f, var_name);
            Expr* dg = expr_differentiate(g, var_name);

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
                return NULL;
            }
        }

        case EXPR_FUNC: {
            Expr* u = e->data.func.arg;
            Expr* du = expr_differentiate(u, var_name);

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
                    Expr* reiprocal = expr_pow(expr_copy(u), expr_number("-1"));
                    return expr_mul(reiprocal, du);
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

Expr* expr_number_mpq(mpq_t value) {
    Expr* num_expr;
    num_expr = (Expr*) malloc(sizeof(Expr));
    if (!num_expr) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    num_expr->type = EXPR_NUMBER;
    num_expr->refcount = 1;

    mpq_init(num_expr->data.value);

    mpq_canonicalize(value);
    mpq_set(num_expr->data.value, value);

    mpq_clear(value);

    return num_expr;
}

void mpq_add_ui(mpq_t rop, const mpq_t op1, unsigned long int op2) {
    mpq_t temp;
    mpq_init(temp);
    mpq_set_ui(temp, op2, 1);
    mpq_add(rop, op1, temp);
    mpq_clear(temp);
}

Expr* expr_integrate(const Expr* e, const char* var_name) {
    if (!e) return NULL;

    switch (e->type) {
        case EXPR_NUMBER:
            return expr_mul(expr_retain(e),expr_symbol(var_name));

        case EXPR_SYMBOL:
            if (strcmp(e->data.name, var_name) == 0) {
                return expr_mul(expr_number("1/2"),expr_pow(expr_symbol(var_name), expr_number("2")));
            } else {
                return expr_mul(expr_retain(e),expr_symbol(var_name));
            }

        case EXPR_ADD: {
            Expr* left = expr_integrate(e->data.binop.left, var_name);
            Expr* right = expr_integrate(e->data.binop.right, var_name);
            return expr_add(left, right);
        }

        case EXPR_MUL: {
            Expr* f = e->data.binop.left;
            Expr* g = e->data.binop.right;

            if (g->type == EXPR_NUMBER) {
                Expr* inner = expr_integrate(f, var_name);
                return expr_mul(expr_retain(g), inner);
            }

            if (f->type == EXPR_NUMBER) {
                Expr* inner = expr_integrate(g, var_name);
                return expr_mul(expr_retain(f), inner);
            }

            break; // product rule not implemented
            /*
            Expr* sf = expr_integrate(f, var_name);
            Expr* dg = expr_diff(g, var_name);

            Expr* left_term = expr_mul(expr_retain(sf), expr_retain(g));
            Expr* right_term = expr_mul(expr_retain(sf), expr_retain(dg));

            return expr_add(left_term, right_term);*/
        }

        case EXPR_POW: {
            Expr* base = e->data.binop.left;
            Expr* exponent = e->data.binop.right;

            if (exponent->type == EXPR_NUMBER &&
                base->type == EXPR_SYMBOL &&
                strcmp(base->data.name, var_name) == 0) {
                // d/dx x^n = n * x^(n-1)

                Expr* one = expr_number("1");
                Expr* new_exp_expr = expr_simplify(expr_add(expr_retain(exponent),one));
                //expr_release(one);
                Expr* pow_expr = expr_pow(expr_copy(base), new_exp_expr);

                Expr* coeff = expr_div(expr_copy(one),expr_retain(new_exp_expr));
                expr_release(one);

                return expr_mul(coeff, pow_expr);
            } else {
                fprintf(stderr, "Power rule for general expressions not implemented yet.\n");
                return NULL;
            }
        }

        case EXPR_FUNC: {
            Expr* u = e->data.func.arg;
            //Expr* du = expr_diff(u, var_name);
            if (u->type == EXPR_SYMBOL && strcmp(u->data.name, var_name) == 0) {
                switch (e->data.func.func) {
                    case FUNC_SIN: {
                        Expr* cos_u = expr_func(FUNC_COS, expr_copy(u));
                        Expr* neg_cos_u = expr_mul(expr_number("-1"), cos_u);
                        return neg_cos_u;
                    }
                    case FUNC_COS: {
                        Expr* sin_u = expr_func(FUNC_SIN, expr_copy(u));
                        return sin_u;
                    }
                    case FUNC_EXP: {
                        Expr* exp_u = expr_func(FUNC_EXP, expr_copy(u));
                        return exp_u;
                    }
                    case FUNC_LOG: {
                        Expr* reiprocal = expr_pow(expr_copy(u), expr_number("-1"));
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
            exit(1);
    }

    char* e_str = expr_to_string(e);
    
    fprintf(stderr, "Couldn't resolve integration for expression: %s.\n",e_str);
    free(e_str); 
    return NULL;
}

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

Expr* expr_div(const Expr* a, const Expr* b) {
    if (!a || !b) return NULL;

    if (b->type == EXPR_NUMBER && mpq_cmp_ui(b->data.value, 1, 1) == 0) {
        return expr_retain(a); // a / 1 = a
    }

    // 0 / b = 0, for any b ≠ 0
    if (a->type == EXPR_NUMBER && mpq_sgn(a->data.value) == 0) {
        return expr_number("0");
    }

    // a / a = 1, for non-zero a
    if (expr_equal(a, b)) {
        return expr_number("1");
    }

    if (a->type == EXPR_NUMBER && b->type == EXPR_NUMBER) {
        mpq_t q;
        mpq_init(q);
        mpq_div(q, a->data.value, b->data.value);
        Expr* result = expr_number_mpq(q); // takes ownership or copies
        //mpq_clear(q);
        return result;
    }

    // a / b = a * b^(-1)
    Expr* inverse = expr_pow(expr_retain(b), expr_number("-1"));
    Expr* result = expr_mul(expr_retain(a), inverse);
    expr_release(inverse);
    return expr_simplify(result);
}


#endif // CYMCALC_IMPLEMENTATION