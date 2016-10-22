./build/parse.out $1 | ../CodeGen/codeGen.o \
    && opt-3.6 -O3 -S -strip-debug -tailcallelim -constprop testRun.bc > testRun.ll \
    && rm testRun.bc \
    && lli-3.6 -enable-misched -O3 testRun.ll
