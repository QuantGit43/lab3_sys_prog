#!/bin/sh
T=build/t
mkdir -p $T
fail=0
for f in examples/*.c; do
    n=$(basename "$f" .c)
    gcc -w -o $T/$n.ref "$f" && ./$T/$n.ref > $T/$n.ref.out
    ./build/mcc -run "$f" > $T/$n.run.out
    ./build/mcc -S "$f" > $T/$n.s && gcc -o $T/$n.asm $T/$n.s && ./$T/$n.asm > $T/$n.asm.out
    if cmp -s $T/$n.ref.out $T/$n.run.out && cmp -s $T/$n.ref.out $T/$n.asm.out; then
        echo "ok   $n"
    else
        echo "FAIL $n"
        diff $T/$n.ref.out $T/$n.run.out | head -5
        diff $T/$n.ref.out $T/$n.asm.out | head -5
        fail=1
    fi
done
for f in tests/err/*.c; do
    if ./build/mcc -run "$f" > /dev/null 2> $T/err.txt; then
        echo "FAIL $f accepted"
        fail=1
    else
        echo "ok   $(basename "$f"): $(head -1 $T/err.txt)"
    fi
done
exit $fail
