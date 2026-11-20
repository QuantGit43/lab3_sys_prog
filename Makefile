CC = gcc
CFLAGS = -O2 -Wall -Wno-unused-function -Isrc -Ibuild
B = build
OUT = out
EXAMPLES = $(wildcard examples/*.c)

all: $(B)/mcc

$(B):
	mkdir -p $(B)

$(B)/parser.tab.c $(B)/parser.tab.h: src/parser.y | $(B)
	bison -d -v -Wall -Wcounterexamples -o $(B)/parser.tab.c src/parser.y

$(B)/lex.yy.c: src/lexer.l $(B)/parser.tab.h
	flex -o $@ src/lexer.l

$(B)/mcc: src/ast.c src/interp.c src/codegen.c src/main.c src/ast.h $(B)/parser.tab.c $(B)/lex.yy.c
	$(CC) $(CFLAGS) -o $@ src/ast.c src/interp.c src/codegen.c src/main.c $(B)/parser.tab.c $(B)/lex.yy.c

test: all
	sh test.sh

viz: all
	mkdir -p $(OUT)
	for f in $(EXAMPLES); do n=$$(basename $$f .c); { cat viz/head.html; $(B)/mcc -json $$f; cat viz/tail.html; } > $(OUT)/$$n.html; done

asm: all
	mkdir -p $(OUT)
	for f in $(EXAMPLES); do $(B)/mcc -S $$f > $(OUT)/$$(basename $$f .c).s; done

clean:
	rm -rf build out

.PHONY: all test viz asm clean
