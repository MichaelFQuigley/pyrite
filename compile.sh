filename=$1

echo 'Parsing'
python model.py

cat ${filename}.ll stdlib/stdlib.ll > ${filename}2.ll
echo 'Optimization pass'
opt-3.6 -O1 ${filename}2.ll > ${filename}.bc

echo 'Generating executable'
llc ${filename}.bc -filetype=asm -o ${filename}.s
gcc ${filename}.s -o ${filename}.o
