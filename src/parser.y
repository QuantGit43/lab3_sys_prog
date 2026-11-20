%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

int yylex(void);
void yyerror(const char *s);
Node *root;

static Node *assign(const char *op, Node *lv, Node *rhs)
{
    return mkop(N_ASSIGN, op, lv, rhs);
}

static Node *incdec(const char *op, int prefix, Node *lv)
{
    Node *n = mkop(N_INC, op, lv, NULL);
    n->val = prefix;
    return n;
}

static Node *opt(Node *n)
{
    return n ? n : mk(N_EMPTY);
}
%}

%define parse.error verbose
%locations

%union {
    long num;
    char *str;
    Node *node;
}

%token <num> NUM
%token <str> ID STR
%token INT VOID IF ELSE WHILE FOR RETURN BREAK CONTINUE
%token EQ NE LE GE AND OR INC DEC
%token ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN

%type <num> type
%type <node> program func params param_list param block items decl declarators declarator
%type <node> stmt expr expr_opt lvalue args arg_list

%precedence LOWER_THAN_ELSE
%precedence ELSE
%precedence '=' ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN
%left OR
%left AND
%left EQ NE
%left '<' '>' LE GE
%left '+' '-'
%left '*' '/' '%'
%precedence UNARY

%%

program
    : %empty                    { $$ = root = mk(N_PROG); }
    | program func              { $$ = add($1, $2); }
    | program decl              { $$ = add($1, $2); }
    ;

type
    : INT                       { $$ = 1; }
    | VOID                      { $$ = 0; }
    ;

func
    : type ID '(' params ')' block
                                { $$ = add(add(mkname(N_FUNC, $2), $4), $6); $$->val = $1; $$->line = @2.first_line; }
    ;

params
    : %empty                    { $$ = mk(N_PARAMS); }
    | VOID                      { $$ = mk(N_PARAMS); }
    | param_list
    ;

param_list
    : param                     { $$ = add(mk(N_PARAMS), $1); }
    | param_list ',' param      { $$ = add($1, $3); }
    ;

param
    : INT ID                    { $$ = mkname(N_PARAM, $2); }
    ;

decl
    : type declarators ';'      { if (!$1) die(yylineno, "variable declared void"); $$ = $2; }
    ;

declarators
    : declarator                { $$ = add(mk(N_DECL), $1); }
    | declarators ',' declarator
                                { $$ = add($1, $3); }
    ;

declarator
    : ID                        { $$ = mkname(N_VAR, $1); }
    | ID '=' expr               { $$ = add(mkname(N_VAR, $1), $3); }
    | ID '[' NUM ']'            { $$ = mkname(N_VAR, $1); if ($3 <= 0) die(yylineno, "bad array size"); $$->val = $3; }
    ;

block
    : '{' items '}'             { $$ = $2; }
    ;

items
    : %empty                    { $$ = mk(N_BLOCK); }
    | items decl                { $$ = add($1, $2); }
    | items stmt                { $$ = add($1, $2); }
    ;

stmt
    : block
    | ';'                       { $$ = mk(N_EMPTY); }
    | expr ';'                  { $$ = mkop(N_EXPR, NULL, $1, NULL); }
    | IF '(' expr ')' stmt %prec LOWER_THAN_ELSE
                                { $$ = mkop(N_IF, NULL, $3, $5); $$->line = @1.first_line; }
    | IF '(' expr ')' stmt ELSE stmt
                                { $$ = add(mkop(N_IF, NULL, $3, $5), $7); $$->line = @1.first_line; }
    | WHILE '(' expr ')' stmt   { $$ = mkop(N_WHILE, NULL, $3, $5); $$->line = @1.first_line; }
    | FOR '(' expr_opt ';' expr_opt ';' expr_opt ')' stmt
                                { $$ = add(add(mkop(N_FOR, NULL, opt($3), opt($5)), opt($7)), $9); $$->line = @1.first_line; }
    | FOR '(' decl expr_opt ';' expr_opt ')' stmt
                                { $$ = add(add(mkop(N_FOR, NULL, $3, opt($4)), opt($6)), $8); $$->line = @1.first_line; }
    | RETURN expr_opt ';'       { $$ = mkop(N_RETURN, NULL, $2, NULL); }
    | BREAK ';'                 { $$ = mk(N_BREAK); }
    | CONTINUE ';'              { $$ = mk(N_CONTINUE); }
    ;

expr_opt
    : %empty                    { $$ = NULL; }
    | expr
    ;

lvalue
    : ID                        { $$ = mkname(N_ID, $1); }
    | ID '[' expr ']'           { $$ = add(mkname(N_INDEX, $1), $3); }
    ;

expr
    : NUM                       { $$ = mk(N_NUM); $$->val = $1; }
    | STR                       { $$ = mkname(N_STR, $1); }
    | lvalue
    | ID '(' args ')'           { $$ = $3; $$->name = $1; }
    | '(' expr ')'              { $$ = $2; }
    | lvalue '=' expr           { $$ = assign("=", $1, $3); }
    | lvalue ADD_ASSIGN expr    { $$ = assign("+=", $1, $3); }
    | lvalue SUB_ASSIGN expr    { $$ = assign("-=", $1, $3); }
    | lvalue MUL_ASSIGN expr    { $$ = assign("*=", $1, $3); }
    | lvalue DIV_ASSIGN expr    { $$ = assign("/=", $1, $3); }
    | lvalue MOD_ASSIGN expr    { $$ = assign("%=", $1, $3); }
    | expr OR expr              { $$ = mkop(N_BIN, "||", $1, $3); }
    | expr AND expr             { $$ = mkop(N_BIN, "&&", $1, $3); }
    | expr EQ expr              { $$ = mkop(N_BIN, "==", $1, $3); }
    | expr NE expr              { $$ = mkop(N_BIN, "!=", $1, $3); }
    | expr '<' expr             { $$ = mkop(N_BIN, "<", $1, $3); }
    | expr '>' expr             { $$ = mkop(N_BIN, ">", $1, $3); }
    | expr LE expr              { $$ = mkop(N_BIN, "<=", $1, $3); }
    | expr GE expr              { $$ = mkop(N_BIN, ">=", $1, $3); }
    | expr '+' expr             { $$ = mkop(N_BIN, "+", $1, $3); }
    | expr '-' expr             { $$ = mkop(N_BIN, "-", $1, $3); }
    | expr '*' expr             { $$ = mkop(N_BIN, "*", $1, $3); }
    | expr '/' expr             { $$ = mkop(N_BIN, "/", $1, $3); }
    | expr '%' expr             { $$ = mkop(N_BIN, "%", $1, $3); }
    | '-' expr %prec UNARY      { $$ = mkop(N_UN, "-", $2, NULL); }
    | '!' expr %prec UNARY      { $$ = mkop(N_UN, "!", $2, NULL); }
    | INC lvalue                { $$ = incdec("++", 1, $2); }
    | DEC lvalue                { $$ = incdec("--", 1, $2); }
    | lvalue INC                { $$ = incdec("++", 0, $1); }
    | lvalue DEC                { $$ = incdec("--", 0, $1); }
    ;

args
    : %empty                    { $$ = mk(N_CALL); }
    | arg_list
    ;

arg_list
    : expr                      { $$ = add(mk(N_CALL), $1); }
    | arg_list ',' expr         { $$ = add($1, $3); }
    ;

%%

void yyerror(const char *s)
{
    fprintf(stderr, "line %d: %s\n", yylineno, s);
}
