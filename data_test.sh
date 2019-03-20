#!/bin/bash

make

INPUT_DIR=images/original
OUTPUT_DIR=images/processed
mkdir $OUTPUT_DIR 2>/dev/null

for i in $INPUT_DIR/*gif ; do
    DEST=$OUTPUT_DIR/`basename $i .gif`.gif
    echo "Running test on $i -> $DEST\n"

    OMP_NUM_THREADS=4 salloc -n 1 -N 1  mpirun ./sobelf $i $DEST m
done




