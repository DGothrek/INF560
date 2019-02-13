#!/bin/bash

make

INPUT_DIR=images/original
OUTPUT_DIR=images/processed
mkdir $OUTPUT_DIR 2>/dev/null

i=images/original/Campusplan-Mobilitaetsbeschraenkte.gif
DEST=images/processed/Campusplan-Mobilitaetsbeschraenkte.gif
echo "Running test on $i -> $DEST"

OMP_NUM_THREADS=8 salloc -N 1 -n 1 -c 8 mpirun ./sobelf $i $DEST

