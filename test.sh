#!/bin/bash

make

INPUT_DIR=images/original
OUTPUT_DIR=images/processed
mkdir $OUTPUT_DIR 2>/dev/null

i=images/original/Campusplan-Hausnr.gif
DEST=images/processed/Campusplan-Hausnr.gif
echo "Running test on $i -> $DEST"

OMP_NUM_THREADS=2 salloc -n 4 -N 1  mpirun ./sobelf $i $DEST

