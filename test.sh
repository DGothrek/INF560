#!/bin/bash

make

INPUT_DIR=images/original
OUTPUT_DIR=images/processed
mkdir $OUTPUT_DIR 2>/dev/null

i=images/original/tumblr_myxlbtwVEb1qzt4vjo1_r14_500_large.gif
DEST=images/processed/tumblr_myxlbtwVEb1qzt4vjo1_r14_500_large.gif
echo "Running test on $i -> $DEST"

OMP_NUM_THREADS=1 salloc -n 1 -N 1  mpirun ./sobelf $i $DEST

