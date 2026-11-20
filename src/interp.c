#include <stdlib.h>
#include <string.h>
#include "ast.h"

typedef struct {
    char *name;
    long *mem;
    long size;
} Var;

enum { ST_NORMAL, ST_BREAK, ST_CONTINUE, ST_RETURN };

static Var *vars;
static int nvars, capvars, frame, nglobals;
static Node *prog;
static long retval;

static long eval(Node *e);
static int exec(Node *s);

static Var *lookup(Node *n)
{
    for (int i = nvars - 1; i >= frame; i--)
        if (!strcmp(vars[i].name, n->name)) return &vars[i];
    for (int i = nglobals - 1; i >= 0; i--)
        if (!strcmp(vars[i].name, n->name)) return &vars[i];
    die(n->line, "undeclared variable '%s'", n->name);
    return NULL;
}

static long *declare(char *name, long size)
{
    if (nvars == capvars) {
        capvars = capvars ? capvars * 2 : 64;
        vars = realloc(vars, capvars * sizeof *vars);
    }
    vars[nvars].name = name;
    vars[nvars].size = size;
    vars[nvars].mem = calloc(size ? size : 1, sizeof(long));
    return vars[nvars++].mem;
}

static void pop_to(int k)
{
    while (nvars > k) free(vars[--nvars].mem);
}

static long *addr(Node *e)
{
    long i = e->kind == N_INDEX ? eval(e->kid[0]) : 0;
    Var *v = lookup(e);
    if (e->kind == N_ID) {
        if (v->size) die(e->line, "array '%s' used as scalar", e->name);
        return v->mem;
    }
    if (!v->size) die(e->line, "'%s' is not an array", e->name);
    if (i < 0 || i >= v->size) die(e->line, "index %ld out of bounds of '%s[%ld]'", i, e->name, v->size);
    return v->mem + i;
}

static char *unescape(const char *s)
{
    char *r = malloc(strlen(s) + 1), *p = r;
    for (s++; s[1]; s++) {
        if (*s != '\\') { *p++ = *s; continue; }
        switch (*++s) {
        case 'n': *p++ = '\n'; break;
        case 't': *p++ = '\t'; break;
        case '0': *p++ = 0; break;
        default: *p++ = *s;
        }
    }
    *p = 0;
    return r;
}

static long arith(int line, const char *op, long l, long r)
{
    if ((op[0] == '/' || op[0] == '%') && r == 0) die(line, "division by zero");
    if (!strcmp(op, "+")) return l + r;
    if (!strcmp(op, "-")) return l - r;
    if (!strcmp(op, "*")) return l * r;
    if (!strcmp(op, "/")) return l / r;
    if (!strcmp(op, "%")) return l % r;
    if (!strcmp(op, "==")) return l == r;
    if (!strcmp(op, "!=")) return l != r;
    if (!strcmp(op, "<")) return l < r;
    if (!strcmp(op, ">")) return l > r;
    if (!strcmp(op, "<=")) return l <= r;
    if (!strcmp(op, ">=")) return l >= r;
    die(line, "unknown operator '%s'", op);
    return 0;
}

static long do_printf(Node *e)
{
    if (!e->n || e->kid[0]->kind != N_STR) die(e->line, "printf needs a string literal format");
    const char *f = (const char *)eval(e->kid[0]);
    int ai = 1;
    while (*f) {
        if (*f != '%') { putchar(*f++); continue; }
        if (f[1] == '%') { putchar('%'); f += 2; continue; }
        char spec[40];
        int k = 0;
        spec[k++] = *f++;
        while (*f && strchr("-+ #0123456789.", *f) && k < 30) spec[k++] = *f++;
        while (*f == 'l') f++;
        char c = *f++;
        if (ai >= e->n) die(e->line, "too few arguments for printf");
        long v = eval(e->kid[ai++]);
        if (strchr("dixu", c)) { spec[k++] = 'l'; spec[k++] = c; spec[k] = 0; printf(spec, v); }
        else if (c == 'c') { spec[k++] = c; spec[k] = 0; printf(spec, (int)v); }
        else if (c == 's') { spec[k++] = c; spec[k] = 0; printf(spec, (char *)v); }
        else die(e->line, "unsupported printf conversion '%%%c'", c);
    }
    return 0;
}

static Node *find_func(const char *name)
{
    for (int i = 0; i < prog->n; i++)
        if (prog->kid[i]->kind == N_FUNC && !strcmp(prog->kid[i]->name, name)) return prog->kid[i];
    return NULL;
}

static long call(Node *e)
{
    if (!strcmp(e->name, "printf")) return do_printf(e);
    if (!strcmp(e->name, "putchar") && e->n == 1) return putchar((int)eval(e->kid[0]));
    Node *f = find_func(e->name);
    if (!f) die(e->line, "undefined function '%s'", e->name);
    Node *ps = f->kid[0];
    if (ps->n != e->n) die(e->line, "'%s' expects %d arguments, got %d", e->name, ps->n, e->n);
    long *args = malloc((e->n + 1) * sizeof *args);
    for (int i = 0; i < e->n; i++) args[i] = eval(e->kid[i]);
    int saved = frame, base = nvars;
    frame = nvars;
    for (int i = 0; i < ps->n; i++) *declare(ps->kid[i]->name, 0) = args[i];
    free(args);
    int st = exec(f->kid[1]);
    if (st == ST_BREAK || st == ST_CONTINUE) die(f->line, "break/continue outside loop in '%s'", f->name);
    long r = st == ST_RETURN ? retval : 0;
    pop_to(base);
    frame = saved;
    return r;
}

static long eval(Node *e)
{
    long *p, r;
    switch (e->kind) {
    case N_NUM: return e->val;
    case N_STR:
        if (!e->op) e->op = unescape(e->name);
        return (long)e->op;
    case N_ID:
    case N_INDEX: return *addr(e);
    case N_CALL: return call(e);
    case N_ASSIGN:
        r = eval(e->kid[1]);
        p = addr(e->kid[0]);
        if (e->op[0] == '=') *p = r;
        else *p = arith(e->line, (char[]){ e->op[0], 0 }, *p, r);
        return *p;
    case N_BIN:
        if (!strcmp(e->op, "&&")) return eval(e->kid[0]) && eval(e->kid[1]);
        if (!strcmp(e->op, "||")) return eval(e->kid[0]) || eval(e->kid[1]);
        r = eval(e->kid[0]);
        return arith(e->line, e->op, r, eval(e->kid[1]));
    case N_UN:
        r = eval(e->kid[0]);
        return e->op[0] == '-' ? -r : !r;
    case N_INC:
        p = addr(e->kid[0]);
        r = *p;
        *p += e->op[0] == '+' ? 1 : -1;
        return e->val ? *p : r;
    default:
        die(e->line, "not an expression: %s", kindname(e->kind));
    }
    return 0;
}

static void run_init(Node *s)
{
    if (s->kind == N_DECL) exec(s);
    else if (s->kind != N_EMPTY) eval(s);
}

static int exec(Node *s)
{
    int st, base;
    switch (s->kind) {
    case N_BLOCK:
        base = nvars;
        for (int i = 0; i < s->n; i++)
            if ((st = exec(s->kid[i])) != ST_NORMAL) { pop_to(base); return st; }
        pop_to(base);
        return ST_NORMAL;
    case N_DECL:
        for (int i = 0; i < s->n; i++) {
            Node *v = s->kid[i];
            long init = v->n ? eval(v->kid[0]) : 0;
            *declare(v->name, v->val) = init;
        }
        return ST_NORMAL;
    case N_EXPR: eval(s->kid[0]); return ST_NORMAL;
    case N_EMPTY: return ST_NORMAL;
    case N_IF:
        if (eval(s->kid[0])) return exec(s->kid[1]);
        return s->n > 2 ? exec(s->kid[2]) : ST_NORMAL;
    case N_WHILE:
        while (eval(s->kid[0])) {
            st = exec(s->kid[1]);
            if (st == ST_BREAK) break;
            if (st == ST_RETURN) return st;
        }
        return ST_NORMAL;
    case N_FOR:
        base = nvars;
        run_init(s->kid[0]);
        while (s->kid[1]->kind == N_EMPTY || eval(s->kid[1])) {
            st = exec(s->kid[3]);
            if (st == ST_BREAK) break;
            if (st == ST_RETURN) { pop_to(base); return st; }
            if (s->kid[2]->kind != N_EMPTY) eval(s->kid[2]);
        }
        pop_to(base);
        return ST_NORMAL;
    case N_RETURN:
        retval = s->n ? eval(s->kid[0]) : 0;
        return ST_RETURN;
    case N_BREAK: return ST_BREAK;
    case N_CONTINUE: return ST_CONTINUE;
    default:
        eval(s);
        return ST_NORMAL;
    }
}

int interpret(Node *p)
{
    prog = p;
    for (int i = 0; i < p->n; i++)
        if (p->kid[i]->kind == N_DECL) exec(p->kid[i]);
    nglobals = frame = nvars;
    Node *m = mkname(N_CALL, "main");
    long r = call(m);
    fflush(stdout);
    return (int)r;
}
