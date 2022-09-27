# Only test49.tig have the syntax error
testcases=../testcases/
for entry in "$testcases"/*
do
    echo "./prabsyn $entry"
    ./prabsyn $entry
    echo ""
done
