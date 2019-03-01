#!/bin/bash

gcc -o obj/compare.o src/compare.c

TEST_DIR=images/processed
REF_DIR=images/ref

for i in $TEST_DIR/*gif ; do
    DEST=$REF_DIR/`basename $i`
    echo "Comparing $i with $DEST"

    obj/compare.o $i $DEST
    echo
done
