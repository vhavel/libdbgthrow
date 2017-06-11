#!/bin/sh

output=$(mktemp)

ulimit -c unlimited

./test1
DBGTHROW_OUTPUT_FILENAME=$output LD_PRELOAD=./libdbgthrow.so ./test1
cat $output
grep thrown $output || (echo exception probably not printed && gdb test1 core -ex "thread apply all bt" -ex "set pagination 0" -batch && exit 1)
