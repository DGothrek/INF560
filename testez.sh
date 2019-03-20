#!/bin/bash

make

INPUT_DIR=images/original
OUTPUT_DIR=images/processed
mkdir $OUTPUT_DIR 2>/dev/null

i=../images/original/Campusplan-Hausnr.gif
DEST=../images/processed/Campusplan-Hausnr.gif
echo "Running test on $i -> $DEST"

cd traces
rm -f *

OMP_NUM_THREADS=8 salloc -N 1 -n 1 mpirun eztrace -t mpi ../sobelf $i $DEST m

eztrace_convert ${USER}_eztrace_log_rank_*

cd ..

vite traces/eztrace_output.trace