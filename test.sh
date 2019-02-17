#!/bin/bash

make

INPUT_DIR=images/original
OUTPUT_DIR=images/processed
mkdir $OUTPUT_DIR 2>/dev/null

i=images/original/giphy-3.gif
DEST=images/processed/giphy-3.gif
echo "Running test on $i -> $DEST"

OMP_NUM_THREADS=4 salloc -n 2 -N 1  mpirun ./sobelf $i $DEST

