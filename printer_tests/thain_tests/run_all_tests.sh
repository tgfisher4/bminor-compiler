#!/usr/bin/env bash

#workspace=$(mktemp -d)

for testfile in good*.bminor; do
    ./bminor -print ${testfile} > >(tee ${testfile}.out > /dev/null) 2> >(tee ${testfile}.out >&2)
    e_st=$?
    sleep 0.2
	if [ $e_st -eq 0 ]; then
		echo "$testfile success (as expected)"
        
        diff <(cat ${testfile}.out) <(./bminor -print ${testfile}.out)
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
