#!/bin/bash

make

INPUT_DIR=images/original
OUTPUT_DIR=images/processed
mkdir $OUTPUT_DIR 2>/dev/null

i=images/original/porsche.gif
DEST=images/processed/porsche.gif
echo "Running test on $i -> $DEST"

OMP_NUM_THREADS=1 salloc -n 1 -N 1  mpirun ./sobelf $i $DEST

