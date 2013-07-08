#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Missing argument: tests.def file";
	exit 1;
fi

VALGRIND="valgrind --leak-check=yes --suppressions=valgrind.supp"
WRAPPER="./wrapper"

TEST_DIR=`dirname $1`

for TEST_FILE in `cat $1`; do
	if [[ $TEST_FILE == \#* ]]; then
		continue;
	fi

	$VALGRIND $WRAPPER $TEST_DIR/$TEST_FILE

done
