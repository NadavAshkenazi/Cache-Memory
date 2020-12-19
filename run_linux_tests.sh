#!/bin/bash

echo Running tests...
echo The test succeed if there are no diffs printed.
echo

for filename in tests/test*.command; do
    test_num=`echo $filename | cut -d'.' -f1`
     bash ${filename} > ${test_num}.YoursOut
done

passedAll=true
ok=true
for filename in tests/test*.out; do
    test_num=`echo $filename | cut -d'.' -f1`
    diff_result=$(diff ${test_num}.OURS ${test_num}.YoursOut)
    if [ "$diff_result" != "" ]; then
        echo The test ${test_num} didnt pass
        passedAll=false
    else 
        echo The test ${test_num} PASSED!
    fi
done

echo
echo Ran all tests.

if [[ "$passedAll" == "$ok" ]]; then
    echo 'PASSED ALL ~~~ ya malik!'
else 
	echo 'Try again'
fi
