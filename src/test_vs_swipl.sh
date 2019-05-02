#/bin/bash
g++ ../tests/rand_prog.cpp
INPUT=`./a.out`
echo $INPUT
./tml --swipl < $INPUT > tml.output 2> tml.fname
OUTPUT=`head -n 1 tml.fname | awk 'NF>1{print $NF}'`
echo $OUTPUT
swipl $OUTPUT > swipl.output 2> /dev/null
sed -i 's/,/ /g' swipl.output
sed -i 's/$/./g' swipl.output
sort swipl.output > swipl.sorted
sort tml.output > tml.sorted
diff swipl.sorted tml.sorted
