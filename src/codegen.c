#include <stdlib.h>
#include <string.h>
#include "ast.h"

#define E(...) fprintf(o, __VA_ARGS__)
#define MAXSYM 4096

typedef struct {
    char *name;
    int off;
    long size;
    int global;
} Sym;

static FILE *o;
static Node *prog;
static Sym syms[MAXSYM];
static int nsyms, nglob, cur, depth, lbl, retl;
static int brk[256], cont[256], nloop;
static Node **strs;
static int nstrs;
static const char *areg[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };

static void gen(Node *e);
static void stmt(Node *s);

static void push(void) { E("\tpushq %%rax\n"); depth++; }
static void pop(const char *r) { E("\tpopq %%%s\n", r); depth--; }

static Sym *lookup(Node *n)
{
    for (int i = nsyms - 1; i >= 0; i--)
        if (!strcmp(syms[i].name, n->name)) return &syms[i];
    die(n->line, "undeclared variable '%s'", n->name);
    return NULL;
}

static Sym *newsym(Node *v, int global)
{
    if (nsyms == MAXSYM) die(v->line, "too many variables");
    Sym *s = &syms[nsyms++];
    s->name = v->name;
    s->size = v->val;
    s->global = global;
    s->off = 0;
    return s;
}

static Node *find_func(const char *name)
{
    for (int i = 0; i < prog->n; i++)
        if (prog->kid[i]->kind == N_FUNC && !strcmp(prog->kid[i]->name, name)) return prog->kid[i];
    return NULL;
}

static void addr(Node *e)
{
    Sym *s = lookup(e);
    if (e->kind == N_ID && s->size) die(e->line, "array '%s' used as scalar", e->name);
    if (e->kind == N_INDEX && !s->size) die(e->line, "'%s' is not an array", e->name);
    if (e->kind == N_INDEX) { gen(e->kid[0]); push(); }
    if (s->global) E("\tleaq %s(%%rip), %%rax\n", s->name);
    else E("\tleaq %d(%%rbp), %%rax\n", s->off);
    if (e->kind == N_INDEX) { pop("rcx"); E("\tleaq (%%rax,%%rcx,8), %%rax\n"); }
}

static void binop(const char *op)
{
    static const char *cmp[][2] = { { "==", "e" }, { "!=", "ne" }, { "<", "l" }, { ">", "g" }, { "<=", "le" }, { ">=", "ge" } };
    if (!strcmp(op, "+")) E("\taddq %%rcx, %%rax\n");
    else if (!strcmp(op, "-")) E("\tsubq %%rcx, %%rax\n");
    else if (!strcmp(op, "*")) E("\timulq %%rcx, %%rax\n");
    else if (!strcmp(op, "/")) E("\tcqto\n\tidivq %%rcx\n");
    else if (!strcmp(op, "%")) E("\tcqto\n\tidivq %%rcx\n\tmovq %%rdx, %%rax\n");
    else
        for (int i = 0; i < 6; i++)
            if (!strcmp(op, cmp[i][0])) {
                E("\tcmpq %%rcx, %%rax\n\tset%s %%al\n\tmovzbq %%al, %%rax\n", cmp[i][1]);
                return;
            }
}

static void call(Node *e)
{
    Node *f = find_func(e->name);
    if (f && f->kid[0]->n != e->n) die(e->line, "'%s' expects %d arguments, got %d", e->name, f->kid[0]->n, e->n);
    int onstack = e->n > 6 ? e->n - 6 : 0, pad = (depth + onstack) % 2;
    if (pad) { E("\tsubq $8, %%rsp\n"); depth++; }
    for (int i = e->n - 1; i >= 0; i--) { gen(e->kid[i]); push(); }
    for (int i = 0; i < e->n && i < 6; i++) pop(areg[i]);
    E("\tmovl $0, %%eax\n\tcall %s\n", e->name);
    if (onstack + pad) { E("\taddq $%d, %%rsp\n", 8 * (onstack + pad)); depth -= onstack + pad; }
}

static void logic(Node *e)
{
    int l = lbl, isand = e->op[0] == '&';
    lbl += 2;
    gen(e->kid[0]);
    E("\tcmpq $0, %%rax\n\t%s .L%d\n", isand ? "je" : "jne", l);
    gen(e->kid[1]);
    E("\tcmpq $0, %%rax\n\t%s .L%d\n", isand ? "je" : "jne", l);
    E("\tmovq $%d, %%rax\n\tjmp .L%d\n.L%d:\n\tmovq $%d, %%rax\n.L%d:\n", isand, l + 1, l, !isand, l + 1);
}

static void gen(Node *e)
{
    switch (e->kind) {
    case N_NUM:
        if (e->val == (int)e->val) E("\tmovq $%ld, %%rax\n", e->val);
        else E("\tmovabsq $%ld, %%rax\n", e->val);
        break;
    case N_STR:
        strs = realloc(strs, (nstrs + 1) * sizeof *strs);
        strs[nstrs] = e;
        E("\tleaq .LS%d(%%rip), %%rax\n", nstrs++);
        break;
    case N_ID:
    case N_INDEX:
        addr(e);
        E("\tmovq (%%rax), %%rax\n");
        break;
    case N_CALL: call(e); break;
    case N_ASSIGN:
        gen(e->kid[1]);
        push();
        addr(e->kid[0]);
        pop("rcx");
        if (e->op[0] == '=') { E("\tmovq %%rcx, (%%rax)\n\tmovq %%rcx, %%rax\n"); break; }
        E("\tmovq %%rax, %%rsi\n\tmovq (%%rsi), %%rax\n");
        binop((char[]){ e->op[0], 0 });
        E("\tmovq %%rax, (%%rsi)\n");
        break;
    case N_BIN:
        if (!strcmp(e->op, "&&") || !strcmp(e->op, "||")) { logic(e); break; }
        gen(e->kid[0]);
        push();
        gen(e->kid[1]);
        E("\tmovq %%rax, %%rcx\n");
        pop("rax");
        binop(e->op);
        break;
    case N_UN:
        gen(e->kid[0]);
        if (e->op[0] == '-') E("\tnegq %%rax\n");
        else E("\tcmpq $0, %%rax\n\tsete %%al\n\tmovzbq %%al, %%rax\n");
        break;
    case N_INC: {
        const char *ins = e->op[0] == '+' ? "addq" : "subq";
        addr(e->kid[0]);
        E("\tmovq %%rax, %%rsi\n");
        if (e->val) E("\t%s $1, (%%rsi)\n\tmovq (%%rsi), %%rax\n", ins);
        else E("\tmovq (%%rsi), %%rax\n\t%s $1, (%%rsi)\n", ins);
        break;
    }
    default:
        die(e->line, "not an expression: %s", kindname(e->kind));
    }
}

static void loop_body(Node *body, int b, int c)
{
    if (nloop == 256) die(body->line, "loops nested too deep");
    brk[nloop] = b;
    cont[nloop++] = c;
    stmt(body);
    nloop--;
}

static void stmt(Node *s)
{
    int save = nsyms, l;
    switch (s->kind) {
    case N_BLOCK:
        for (int i = 0; i < s->n; i++) stmt(s->kid[i]);
        nsyms = save;
        break;
    case N_DECL:
        for (int i = 0; i < s->n; i++) {
            Node *v = s->kid[i];
            if (v->n) gen(v->kid[0]);
            Sym *sym = newsym(v, 0);
            cur -= 8 * (v->val ? v->val : 1);
            sym->off = cur;
            if (v->n) E("\tmovq %%rax, %d(%%rbp)\n", cur);
        }
        break;
    case N_EXPR: gen(s->kid[0]); break;
    case N_EMPTY: break;
    case N_IF:
        l = lbl;
        lbl += 2;
        gen(s->kid[0]);
        E("\tcmpq $0, %%rax\n\tje .L%d\n", l);
        stmt(s->kid[1]);
        E("\tjmp .L%d\n.L%d:\n", l + 1, l);
        if (s->n > 2) stmt(s->kid[2]);
        E(".L%d:\n", l + 1);
        break;
    case N_WHILE:
        l = lbl;
        lbl += 2;
        E(".L%d:\n", l);
        gen(s->kid[0]);
        E("\tcmpq $0, %%rax\n\tje .L%d\n", l + 1);
        loop_body(s->kid[1], l + 1, l);
        E("\tjmp .L%d\n.L%d:\n", l, l + 1);
        break;
    case N_FOR:
        l = lbl;
        lbl += 3;
        if (s->kid[0]->kind == N_DECL) stmt(s->kid[0]);
        else if (s->kid[0]->kind != N_EMPTY) gen(s->kid[0]);
        E(".L%d:\n", l);
        if (s->kid[1]->kind != N_EMPTY) {
            gen(s->kid[1]);
            E("\tcmpq $0, %%rax\n\tje .L%d\n", l + 2);
        }
        loop_body(s->kid[3], l + 2, l + 1);
        E(".L%d:\n", l + 1);
        if (s->kid[2]->kind != N_EMPTY) gen(s->kid[2]);
        E("\tjmp .L%d\n.L%d:\n", l, l + 2);
        nsyms = save;
        break;
    case N_RETURN:
        if (s->n) gen(s->kid[0]);
        else E("\tmovq $0, %%rax\n");
        E("\tjmp .L%d\n", retl);
        break;
    case N_BREAK:
    case N_CONTINUE:
        if (!nloop) die(s->line, "%s outside loop", s->kind == N_BREAK ? "break" : "continue");
        E("\tjmp .L%d\n", s->kind == N_BREAK ? brk[nloop - 1] : cont[nloop - 1]);
        break;
    default:
        gen(s);
    }
}

static long slots(Node *n)
{
    if (!n) return 0;
    long s = n->kind == N_VAR ? (n->val ? n->val : 1) : 0;
    for (int i = 0; i < n->n; i++) s += slots(n->kid[i]);
    return s;
}

static void func(Node *f)
{
    Node *ps = f->kid[0], *body = f->kid[1];
    long frame = ((slots(body) + ps->n) * 8 + 15) & ~15L;
    E("\n\t.text\n\t.globl %s\n\t.type %s, @function\n%s:\n\tpushq %%rbp\n\tmovq %%rsp, %%rbp\n", f->name, f->name, f->name);
    if (frame) E("\tsubq $%ld, %%rsp\n", frame);
    nsyms = nglob;
    cur = depth = nloop = 0;
    retl = lbl++;
    for (int i = 0; i < ps->n; i++) {
        Sym *s = newsym(ps->kid[i], 0);
        if (i >= 6) { s->off = 16 + 8 * (i - 6); continue; }
        cur -= 8;
        s->off = cur;
        E("\tmovq %%%s, %d(%%rbp)\n", areg[i], cur);
    }
    stmt(body);
    E("\tmovq $0, %%rax\n.L%d:\n\tleave\n\tret\n", retl);
}

static long constval(Node *e)
{
    if (e->kind == N_NUM) return e->val;
    if (e->kind == N_UN && e->op[0] == '-') return -constval(e->kid[0]);
    die(e->line, "global initializer must be a constant");
    return 0;
}

void codegen(FILE *out, Node *p)
{
    o = out;
    prog = p;
    for (int i = 0; i < p->n; i++) {
        Node *d = p->kid[i];
        if (d->kind != N_DECL) continue;
        for (int j = 0; j < d->n; j++) {
            Node *v = d->kid[j];
            newsym(v, 1);
            E("\t.data\n\t.align 8\n%s:\n", v->name);
            if (v->n) E("\t.quad %ld\n", constval(v->kid[0]));
            else E("\t.zero %ld\n", 8 * (v->val ? v->val : 1));
        }
    }
    nglob = nsyms;
    for (int i = 0; i < p->n; i++)
        if (p->kid[i]->kind == N_FUNC) func(p->kid[i]);
    if (nstrs) E("\n\t.section .rodata\n");
    for (int i = 0; i < nstrs; i++) E(".LS%d:\n\t.string %s\n", i, strs[i]->name);
    E("\t.section .note.GNU-stack,\"\",@progbits\n");
}
