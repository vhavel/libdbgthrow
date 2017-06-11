#!/bin/sh

output=$(mktemp)

./test1
DBGTHROW_OUTPUT_FILENAME=$output LD_PRELOAD=./libdbgthrow.so ./test1
grep thrown $output || (echo exception probably not printed && exit 1)
