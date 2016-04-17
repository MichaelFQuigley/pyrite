filename=foo

echo 'Parsing'
python model.py

#linking stdlib
#llvm-link-3.6 stdlib/stdlib.bc ${filename}.ll > ${filename}.bc

#llvm-dis ${filename}.bc -o ${filename}.ll

cat ${filename}.ll stdlib/stdlib.ll > ${filename}2.ll
echo 'Optimization pass'
opt -O1 ${filename}2.ll > ${filename}.bc
#cp ${filename}.ll ${filename}.temp.ll
#opt -O1 ${filename}.ll -o ${filename}.bc
#llvm-dis ${filename}.bc -o ${filename}.optimized.ll
#mv ${filename}.temp.ll ${filename}.ll

echo 'Generating executable'
llvm-link-3.6 -x86-asm-syntax=intel ${filename}.bc > ${filename}.o
chmod 775 ${filename}.o
