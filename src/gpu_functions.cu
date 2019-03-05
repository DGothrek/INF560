/**
 * File containing the 3 apply-filter functions implemented 
 * on the GPU using CUDA.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <datatype.h>

/********************************************************
 * Device functions *************************************
 ********************************************************/
__global__ void gray() {

}

__global__ void blur() {

}

__global__ void sobel() {
    
}

/********************************************************
 * Host functions****************************************
 ********************************************************/

void apply_gray_filter_gpu(animated_gif *image) {

}

void apply_blur_filter_gpu(animated_gif *image, int size, int threshold) {

}

void apply_sobel_filter_gpu(animated_gif *image) {

}