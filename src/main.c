#include <stdlib.h>
#include <string.h>
#include "ast.h"

extern FILE *yyin;
int yyparse(void);

static char *slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return strdup("");
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);
    char *s = malloc(n + 1);
    s[fread(s, 1, n, f)] = 0;
    fclose(f);
    return s;
}

int main(int argc, char **argv)
{
    if (argc != 3 || (strcmp(argv[1], "-tree") && strcmp(argv[1], "-json") && strcmp(argv[1], "-run") && strcmp(argv[1], "-S"))) {
        fprintf(stderr, "usage: %s -tree|-json|-run|-S file.c\n", argv[0]);
        return 2;
    }
    if (!(yyin = fopen(argv[2], "r"))) { perror(argv[2]); return 1; }
    if (yyparse()) return 1;
    if (!strcmp(argv[1], "-tree")) print_tree(stdout, root, 0);
    else if (!strcmp(argv[1], "-run")) return interpret(root);
    else if (!strcmp(argv[1], "-S")) codegen(stdout, root);
    else {
        fputs("{\"file\":", stdout);
        json_str(stdout, argv[2]);
        fputs(",\"source\":", stdout);
        json_str(stdout, slurp(argv[2]));
        fputs(",\"ast\":", stdout);
        print_json(stdout, root);
        fputs("}\n", stdout);
    }
    return 0;
}
