testcases=../testcases/
for entry in "$testcases"/*
do
    echo "./type_check $entry"
    ./type_check $entry
    echo ""
done
