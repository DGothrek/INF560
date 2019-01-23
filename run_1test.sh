#!/bin/bash

make

INPUT_DIR=images/original
OUTPUT_DIR=images/processed
mkdir $OUTPUT_DIR 2>/dev/null

i=images/original/Campusplan-Hausnr.gif
DEST=images/processed/Campusplan-Hausnr-sobel.gif
echo "Running test on $i -> $DEST"

./sobelf $i $DEST

