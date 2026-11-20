%{
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.tab.h"

#define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno;

static long charval(const char *s)
{
    if (s[1] != '\\') return (unsigned char)s[1];
    switch (s[2]) {
    case 'n': return '\n';
    case 't': return '\t';
    case '0': return 0;
    default: return (unsigned char)s[2];
    }
}
%}

%option noyywrap nounput noinput yylineno
%x COMMENT

%%

^[ \t]*#.*                  ;
"//".*                      ;
"/*"                        BEGIN(COMMENT);
<COMMENT>"*/"               BEGIN(INITIAL);
<COMMENT>.|\n               ;
<COMMENT><<EOF>>            { fprintf(stderr, "line %d: error: unterminated comment\n", yylineno); exit(1); }

"int"                       return INT;
"void"                      return VOID;
"if"                        return IF;
"else"                      return ELSE;
"while"                     return WHILE;
"for"                       return FOR;
"return"                    return RETURN;
"break"                     return BREAK;
"continue"                  return CONTINUE;

[0-9]+                      { yylval.num = strtol(yytext, NULL, 10); return NUM; }
\'([^'\\\n]|\\.)\'          { yylval.num = charval(yytext); return NUM; }
[A-Za-z_][A-Za-z0-9_]*      { yylval.str = strdup(yytext); return ID; }
\"([^"\\\n]|\\.)*\"         { yylval.str = strdup(yytext); return STR; }

"=="                        return EQ;
"!="                        return NE;
"<="                        return LE;
">="                        return GE;
"&&"                        return AND;
"||"                        return OR;
"++"                        return INC;
"--"                        return DEC;
"+="                        return ADD_ASSIGN;
"-="                        return SUB_ASSIGN;
"*="                        return MUL_ASSIGN;
"/="                        return DIV_ASSIGN;
"%="                        return MOD_ASSIGN;

[ \t\r\n]+                  ;
[-+*/%<>=!(){}\[\];,]       return yytext[0];
.                           { fprintf(stderr, "line %d: error: unexpected character '%s'\n", yylineno, yytext); exit(1); }

%%
