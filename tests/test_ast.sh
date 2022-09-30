# Only test49.tig have the syntax error
testcases=../testcases/
for entry in "$testcases"/*
do
    echo "./asttest $entry"
    ./asttest $entry
    echo ""
done
