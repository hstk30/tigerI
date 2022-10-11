# compare this output with `grep error -r testcases`
testcases=testcases
for entry in "$testcases"/*
do
    ./semanttest $entry

    if [ $? != 0 ]; then
        echo "error: "$entry 1>&2
    fi
done

