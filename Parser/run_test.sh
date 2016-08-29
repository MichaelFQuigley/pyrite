./build/parse.out $1 | ../CodeGen/codeGen.o && llvm-link-3.6 ../CodeGen/stdlib/stdlib.ll test.bc -o testRun.bc \
    && opt-3.6 -O3 -S -strip-debug -inline -tailcallelim -constprop testRun.bc > testRun.ll \
    && lli-3.6 -enable-misched -O3 testRun.ll
