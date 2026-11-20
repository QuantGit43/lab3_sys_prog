# yacc-lab

Синтаксичний аналізатор підмножини мови C на flex + bison (LALR(1)).

Програма `mcc` будує AST і вміє:

- `-tree` — друк AST у вигляді дерева;
- `-json` — AST у JSON;
- `-run` — обчислення програми інтерпретатором AST;
- `-S` — трансляція в асемблер x86-64 (GNU, AT&T, System V ABI).

Підмножина: `int`/`void`, глобальні й локальні змінні та масиви `int`, функції з параметрами та рекурсією, `if/else`, `while`, `for`, `return`, `break`, `continue`, арифметичні, логічні, порівняльні оператори, `= += -= *= /= %=`, `++`/`--`, виклики `printf`/`putchar`.

Візуалізація AST — інтерактивне дерево на d3.js у браузері (`out/<приклад>.html`).

## Збірка і запуск (Docker)

```bash
docker build -t yacc-img .
docker run --rm -v "$PWD":/w yacc-img make test viz asm
docker run --rm -v "$PWD":/w yacc-img ./build/mcc -run examples/fact.c
```

`make test` порівнює вивід кожного прикладу з `examples/` у трьох варіантах: скомпільований gcc оригінал, інтерпретатор `mcc -run` і згенерований `mcc -S` асемблер, зібраний gcc.
