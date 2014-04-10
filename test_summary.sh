#!/bin/bash

files=(/home/sam/devel/aurora/test/testcases/misc/tests.def.summary
	   /home/sam/devel/aurora/test/testcases/aegis/tests.def.summary
	   /home/sam/devel/aurora/test/testcases/necla/tests.def.summary
	   /home/sam/devel/aurora/test/testcases/contracts/tests.def.summary
)
pattern=""

for file in "${files[@]}"; do
	while read line; do
		if [ ${line:0:1} != "#" ]; then
		    if [ -z "$pattern" ]; then
		    	pattern="*$line*"
		    else
		    	pattern="$pattern:*$line*"
		    fi
		fi
	done < $file
done

eval "/home/sam/devel/aurora/run-tests --gtest_filter=$pattern"
