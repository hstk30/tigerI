# compare this output with `grep error -r testcases`
testcases=../testcases/
for entry in "$testcases"/*
do
    echo "./typetest $entry"
    ./typetest $entry > /dev/null
    echo ""
done
