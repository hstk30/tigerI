# compare this output with `grep error -r testcases`
testcases=../testcases/
for entry in "$testcases"/*
do
    echo "./type_check $entry"
    ./type_check $entry > /dev/null
    echo ""
done
