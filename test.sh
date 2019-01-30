#!/bin/bash

make

INPUT_DIR=images/original
OUTPUT_DIR=images/processed
mkdir $OUTPUT_DIR 2>/dev/null

i=images/original/australian-flag-large.gif
DEST=images/processed/australian-flag-large.gif
echo "Running test on $i -> $DEST"

salloc -N 1 -n 2 mpirun ./sobelf $i $DEST

