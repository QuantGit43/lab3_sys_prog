#ifndef AST_H
#define AST_H

#include <stdio.h>

typedef enum {
    N_PROG, N_FUNC, N_PARAMS, N_PARAM, N_DECL, N_VAR, N_BLOCK, N_IF, N_WHILE, N_FOR,
    N_RETURN, N_BREAK, N_CONTINUE, N_EXPR, N_EMPTY, N_NUM, N_STR, N_ID, N_INDEX,
    N_CALL, N_BIN, N_UN, N_ASSIGN, N_INC
} Kind;

typedef struct Node {
    Kind kind;
    int line;
    char *name;
    const char *op;
    long val;
    int n, cap;
    struct Node **kid;
} Node;

extern Node *root;
extern int yylineno;

Node *mk(Kind k);
Node *mkname(Kind k, char *name);
Node *mkop(Kind k, const char *op, Node *a, Node *b);
Node *add(Node *p, Node *c);
const char *kindname(Kind k);
void print_tree(FILE *f, Node *n, int depth);
void print_json(FILE *f, Node *n);
void json_str(FILE *f, const char *s);
void die(int line, const char *fmt, ...);

int interpret(Node *prog);
void codegen(FILE *out, Node *prog);

#endif
