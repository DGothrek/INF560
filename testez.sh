#!/bin/bash

make

INPUT_DIR=images/original
OUTPUT_DIR=images/processed
mkdir $OUTPUT_DIR 2>/dev/null

i=../images/original/1.gif
DEST=../images/processed/1.gif
echo "Running test on $i -> $DEST"

cd traces
rm -f *

OMP_NUM_THREADS=8 salloc -N 1 -n 1 mpirun eztrace -t mpi ../sobelf $i $DEST

eztrace_convert ${USER}_eztrace_log_rank_*

cd ..

vite traces/eztrace_output.trace