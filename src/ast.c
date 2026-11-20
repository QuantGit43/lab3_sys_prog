#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "ast.h"

static const char *names[] = {
    "Program", "Function", "Params", "Param", "Decl", "Var", "Block", "If", "While", "For",
    "Return", "Break", "Continue", "ExprStmt", "Empty", "Num", "Str", "Id", "Index",
    "Call", "Binary", "Unary", "Assign", "IncDec"
};

const char *kindname(Kind k) { return names[k]; }

Node *mk(Kind k)
{
    Node *n = calloc(1, sizeof *n);
    n->kind = k;
    n->line = yylineno;
    return n;
}

Node *mkname(Kind k, char *name)
{
    Node *n = mk(k);
    n->name = name;
    return n;
}

Node *mkop(Kind k, const char *op, Node *a, Node *b)
{
    Node *n = mk(k);
    n->op = op;
    if (a) add(n, a);
    if (b) add(n, b);
    return n;
}

Node *add(Node *p, Node *c)
{
    if (p->n == p->cap) {
        p->cap = p->cap ? p->cap * 2 : 4;
        p->kid = realloc(p->kid, p->cap * sizeof *p->kid);
    }
    p->kid[p->n++] = c;
    return p;
}

void die(int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "line %d: error: ", line);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
    exit(1);
}

static void label(FILE *f, Node *n)
{
    fputs(kindname(n->kind), f);
    switch (n->kind) {
    case N_NUM: fprintf(f, " %ld", n->val); break;
    case N_FUNC: fprintf(f, " %s %s", n->val ? "int" : "void", n->name); break;
    case N_VAR:
        if (n->val) fprintf(f, " %s[%ld]", n->name, n->val);
        else fprintf(f, " %s", n->name);
        break;
    case N_INC: fprintf(f, " %s%s", n->val ? "pre" : "post", n->op); break;
    default:
        if (n->name) fprintf(f, " %s", n->name);
        if (n->op) fprintf(f, " %s", n->op);
    }
}

void print_tree(FILE *f, Node *n, int depth)
{
    fprintf(f, "%*s", depth * 2, "");
    if (!n) { fputs("-\n", f); return; }
    label(f, n);
    fputc('\n', f);
    for (int i = 0; i < n->n; i++) print_tree(f, n->kid[i], depth + 1);
}

void json_str(FILE *f, const char *s)
{
    fputc('"', f);
    for (; *s; s++) {
        if (*s == '"' || *s == '\\') fprintf(f, "\\%c", *s);
        else if (*s == '\n') fputs("\\n", f);
        else if (*s == '\t') fputs("\\t", f);
        else if (*s == '<' || (unsigned char)*s < 0x20) fprintf(f, "\\u%04x", *s);
        else fputc(*s, f);
    }
    fputc('"', f);
}

void print_json(FILE *f, Node *n)
{
    if (!n) { fputs("{\"name\":\"-\",\"type\":\"None\"}", f); return; }
    char *buf;
    size_t len;
    FILE *m = open_memstream(&buf, &len);
    label(m, n);
    fclose(m);
    fputs("{\"name\":", f);
    json_str(f, buf);
    free(buf);
    fprintf(f, ",\"type\":\"%s\",\"line\":%d", kindname(n->kind), n->line);
    if (n->n) {
        fputs(",\"children\":[", f);
        for (int i = 0; i < n->n; i++) {
            if (i) fputc(',', f);
            print_json(f, n->kid[i]);
        }
        fputc(']', f);
    }
    fputc('}', f);
}
