#!/bin/bash

for testfile in good*.bminor; do
    ./bminor -typecheck $testfile > >(tee ${testfile}.out > /dev/null) 2> >(tee ${testfile}.out >&2)
    e_st=$?
    sleep 0.2
	if [ $e_st -eq 0 ]; then
		echo "$testfile success (as expected)"
	else
		echo "$testfile failure (INCORRECT)"
	fi
done

for testfile in bad*.bminor; do
    ./bminor -typecheck $testfile > >(tee ${testfile}.out > /dev/null) 2> >(tee ${testfile}.out >&2)
    e_st=$?
    sleep 0.2
	if [ $e_st -eq 0 ]; then
		echo "$testfile success (INCORRECT)"
	else
		echo "$testfile failure (as expected)"
	fi
done
