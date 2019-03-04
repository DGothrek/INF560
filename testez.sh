#!/bin/bash

make

INPUT_DIR=images/original
OUTPUT_DIR=images/processed
mkdir $OUTPUT_DIR 2>/dev/null

i=../images/original/Campusplan-Mobilitaetsbeschraenkte.gif
DEST=../images/processed/Campusplan-Mobilitaetsbeschraenkte-sobel.gif
echo "Running test on $i -> $DEST"

cd traces
rm -f *

OMP_NUM_THREADS=1 salloc -N 1 -n 4 mpirun eztrace -t mpi ../sobelf $i $DEST

eztrace_convert ${USER}_eztrace_log_rank_*

cd ..

vite traces/eztrace_output.trace