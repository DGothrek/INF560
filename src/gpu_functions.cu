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
__global__ void gray(im, height, width) {
    int i, j, moy, pos;
    i = threadIdx.x + blockIdx.x*blockDim.x;
    j = threadIdx.y + blockIdx.y*blockDim.y;
    // int id = threadIdx.x
    //         + blockDim.x*threadIdx.y
    //         + blockIdx.x*blockDim.x*blockDim.y
    //         + blockIdx.y*blockDim.x*blockDim.y*gridDim.x;

    pos = i + j*width;
    if (i < height && j < width) {
        moy = (im[pos].r + im[pos].g + im[pos].b) / 3;
        if (moy < 0) moy = 0;
        if (moy > 255) moy = 255;

        im[pos].r = moy;
        im[pos].g = moy;
        im[pos].b = moy;

    }
}

__global__ void blur() {

}

__global__ void sobel(im, im_new, height, width) {
    // a écrire
    
}

/********************************************************
 * Host functions****************************************
 ********************************************************/

void apply_gray_filter_gpu(animated_gif *image) {
    /**
     * Assuming images of same size in a multiple image GIF
     **/ 
    int print_time = 1;
    struct timeval t1, t2;

    int im_num;
    int width = image->width[0];
    int height = image->height[0];
    int size = width*height;

    /**
     * Allocation on the device once for all the images (if multiple)
     * Memory allocation + dimension of grid
     **/
    if (print_time) gettimeofday(&t1, NULL);

    pixel *device_image;
    cudaMalloc(&device_image, size*sizeof(pixel));

    // Image cut into n*m rectangles
    int n = width / 1024 + 1;
    int m = height / 1024 + 1;
    dim3 dimGrid(n, m);
    dim3 dimBlock(width / n + 1, height / m + 1);

    if (print_time) {
        gettimeofday(&t2, NULL);
        printf("Alloc done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
        gettimeofday(&t1, NULL); 
    }

    /**
     * Memory transfert + computation
     **/
    for (im_num = 0; im_num < image->n_images; im_num++) {
        // Memory transfer
        cudaMemcpy(device_image, image->p[im_num], size * sizeof(pixel), cudaMemcpyHostToDevice);
        if (print_time) {
            gettimeofday(&t2, NULL);
            printf("Transfer done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
            gettimeofday(&t1, NULL); 
        }

        // Computation
        gray<<<dimGrid, dimBlock>>>(device_image, height, width);
        if (print_time) {
            gettimeofday(&t2, NULL);
            printf("Computation done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
            gettimeofday(&t1, NULL); 
        }

        // Transfer back
        cudaMemcpy(image->p[im_num], device_image, size * sizeof(pixel), cudaMemcpyHostToDevice);
        if (print_time) {
            gettimeofday(&t2, NULL);
            printf("Transfer back done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
            gettimeofday(&t1, NULL); 
        }

    }

    cudaFree(device_image);
}

void apply_blur_filter_gpu(animated_gif *image, int size, int threshold) {

}

void apply_sobel_filter_gpu(animated_gif *image) {
    /**
     * Almost same code than apply_gray_filter_gpu
     **/ 
     int print_time = 1;
     struct timeval t1, t2;
 
     int im_num;
     int width = image->width[0];
     int height = image->height[0];
     int size = width*height;
 
     /**
      * Allocation on the device once for all the images (if multiple)
      * Memory allocation + dimension of grid
      **/
     if (print_time) gettimeofday(&t1, NULL);
 
     pixel *device_image, *device_new;
     cudaMalloc(&device_image, size*sizeof(pixel));
     cudaMalloc(&device_new, size*sizeof(pixel));
 
     // Image cut into n*m rectangles
     int n = width / 1024 + 1;
     int m = height / 1024 + 1;
     dim3 dimGrid(n, m);
     dim3 dimBlock(width / n + 1, height / m + 1);
 
     if (print_time) {
         gettimeofday(&t2, NULL);
         printf("Alloc done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
         gettimeofday(&t1, NULL); 
     }
 
     /**
      * Memory transfert + computation
      **/
     for (im_num = 0; im_num < image->n_images; im_num++) {
         // Memory transfer
         cudaMemcpy(device_image, image->p[im_num], size * sizeof(pixel), cudaMemcpyHostToDevice);
         if (print_time) {
             gettimeofday(&t2, NULL);
             printf("Transfer done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
             gettimeofday(&t1, NULL); 
         }
 
         // Computation
         sobel<<<dimGrid, dimBlock>>>(device_image, device_new, height, width);
         if (print_time) {
             gettimeofday(&t2, NULL);
             printf("Computation done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
             gettimeofday(&t1, NULL); 
         }
 
         // Transfer back
         cudaMemcpy(image->p[im_num], device_new, size * sizeof(pixel), cudaMemcpyHostToDevice);
         if (print_time) {
             gettimeofday(&t2, NULL);
             printf("Transfer back done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
             gettimeofday(&t1, NULL); 
         }
 
     }
 
     cudaFree(device_image);
     cudaFree(device_new);
 }