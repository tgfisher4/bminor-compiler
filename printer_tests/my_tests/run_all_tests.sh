#!/bin/bash


for testfile in good*.bminor; do
    # for some reason this has wack behavior if files already exist
    #./bminor -print ${testfile} > >(tee ${testfile}.out > /dev/null) 2> >(tee ${testfile}.out >&2)
    ./bminor -print ${testfile} > ${testfile}.out
    e_st=$?
    sleep 0.2
	if [ $e_st -eq 0 ]; then 
        diff <(cat ${testfile}.out) <(./bminor -print ${testfile}.out)
        e_st=$?
        sleep 0.2
        if [ $e_st -eq 0 ]; then
		    echo "$testfile success (as expected)"
        else
            echo "$testfile print not idempotent"
        fi
	else
		echo "$testfile parse failure (INCORRECT)"
	fi
done

for testfile in bad*.bminor; do
    ./bminor -print $testfile > >(tee ${testfile}.out > /dev/null) 2> >(tee ${testfile}.out >&2)
    e_st=$?
    sleep 0.2
	if [ $e_st -eq 0 ]; then
		echo "$testfile success (INCORRECT)"
	else
		echo "$testfile failure (as expected)"
	fi
done
