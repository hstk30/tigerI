# compare this output with `grep error -r testcases`
testcases=testcases
for entry in "$testcases"/*
do
    ./semanttest $entry
done

