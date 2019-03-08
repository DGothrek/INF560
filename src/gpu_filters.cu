/**
 * File containing the 3 apply-filter functions implemented 
 * on the GPU using CUDA.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <sys/time.h>

#include <filters.h>

/********************************************************
 * Device functions *************************************
 ********************************************************/
__global__ void gray(pixel *im, int height, int width)
{
  // int i, j;
  int moy, pos;
  // j = threadIdx.x + blockIdx.x * blockDim.x;
  // i = threadIdx.y + blockIdx.y * blockDim.y;
  pos = threadIdx.x + blockIdx.x * blockDim.x;
  // int id = threadIdx.x
  //         + blockDim.x*threadIdx.y
  //         + blockIdx.x*blockDim.x*blockDim.y
  //         + blockIdx.y*blockDim.x*blockDim.y*gridDim.x;

  // pos = i + j * width;
  // if (i < height && j < width)
  if (pos < width * height)
  {
    moy = (im[pos].r + im[pos].g + im[pos].b) / 3;
    if (moy < 0)
      moy = 0;
    if (moy > 255)
      moy = 255;

    im[pos].r = moy;
    im[pos].g = moy;
    im[pos].b = moy;
  }
}

__global__ void blur()
{
}

__global__ void sobel(pixel *im, pixel *im_new, int height, int width)
{
  int i, j, pos;

  pos = threadIdx.x + blockIdx.x * blockDim.x;
  i = pos / width;
  j = pos % width;

  int pixel_blue_no, pixel_blue_n, pixel_blue_ne;
  int pixel_blue_so, pixel_blue_s, pixel_blue_se;
  int pixel_blue_o, pixel_blue_e;

  float deltaX_blue;
  float deltaY_blue;
  float val_blue;

  if (i > 1 && i < height - 1 && j > 1 && j < width - 1)
  {
    pixel_blue_no = im[CONV(i - 1, j - 1, width)].b;
    pixel_blue_n = im[CONV(i - 1, j, width)].b;
    pixel_blue_ne = im[CONV(i - 1, j + 1, width)].b;
    pixel_blue_so = im[CONV(i + 1, j - 1, width)].b;
    pixel_blue_s = im[CONV(i + 1, j, width)].b;
    pixel_blue_se = im[CONV(i + 1, j + 1, width)].b;
    pixel_blue_o = im[CONV(i, j - 1, width)].b;
    pixel_blue_e = im[CONV(i, j + 1, width)].b;

    deltaX_blue = -pixel_blue_no + pixel_blue_ne - 2 * pixel_blue_o + 2 * pixel_blue_e - pixel_blue_so + pixel_blue_se;
    deltaY_blue = pixel_blue_se + 2 * pixel_blue_s + pixel_blue_so - pixel_blue_ne - 2 * pixel_blue_n - pixel_blue_no;
    val_blue = sqrt(deltaX_blue * deltaX_blue + deltaY_blue * deltaY_blue) / 4;

    if (val_blue > 50)
    {
        im_new[CONV(i, j, width)].r = 255;
        im_new[CONV(i, j, width)].g = 255;
        im_new[CONV(i, j, width)].b = 255;
    }
    else
    {
        im_new[CONV(i, j, width)].r = 0;
        im_new[CONV(i, j, width)].g = 0;
        im_new[CONV(i, j, width)].b = 0;
    }
  }

  else
  { 
    if (i < height && j < width) {
      im_new[CONV(i, j, width)] = im[CONV(i, j, width)];
    }
  }
}

/********************************************************
 * Host functions****************************************
 ********************************************************/
extern "C"
{
  void apply_gray_filter_gpu(animated_gif *image)
  {
    /**
     * Assuming images of same size in a multiple image GIF
     **/
    int print_time = 1;
    struct timeval t1, t2;

    int im_num;
    int width = image->width[0];
    int height = image->height[0];
    int size = width * height;

    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp, 0);

    /**
     * Allocation on the device once for all the images (if multiple)
     * Memory allocation + dimension of grid
     **/
    if (print_time)
      gettimeofday(&t1, NULL);

    pixel *device_image;
    cudaMalloc(&device_image, size * sizeof(pixel));

    // Image cut into n*m rectangles
    // int n = width / deviceProp.maxThreadsPerBlock + 1;
    // int m = height / deviceProp.maxThreadsPerBlock + 1;
    // int n = width / 32 + 1;
    // int m = height / 32 + 1;
    // dim3 dimGrid(n, m);
    // dim3 dimBlock(width / n + 1, height / m + 1);
    dim3 dimGrid(size / deviceProp.maxThreadsPerBlock + 1);
    dim3 dimBlock(deviceProp.maxThreadsPerBlock);

    if (print_time)
    {
      gettimeofday(&t2, NULL);
      printf("Alloc done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
      gettimeofday(&t1, NULL);
    }

    /**
     * Memory transfert + computation
     **/
    for (im_num = 0; im_num < image->n_images; im_num++)
    {
      // Memory transfer
      cudaMemcpy(device_image, image->p[im_num], size * sizeof(pixel), cudaMemcpyHostToDevice);
      if (print_time)
      {
        gettimeofday(&t2, NULL);
        printf("Transfer done in %ld us with ", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
        printf(cudaGetErrorString(cudaGetLastError()));
        printf("\n");
        gettimeofday(&t1, NULL);
      }



      // Computation
      gray<<<dimGrid, dimBlock>>>(device_image, height, width);

      if (print_time)
      {
        gettimeofday(&t2, NULL);
        printf("Computation done in %ld us with ", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
        printf(cudaGetErrorString(cudaGetLastError()));
        printf("\n");
        gettimeofday(&t1, NULL);
      }

      // Transfer back
      cudaMemcpy(image->p[im_num], device_image, size * sizeof(pixel), cudaMemcpyDeviceToHost);
      if (print_time)
      {
        gettimeofday(&t2, NULL);
        printf("Transfer back done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
        gettimeofday(&t1, NULL);
      }
    }

    cudaFree(device_image);
  }

  void apply_blur_filter_gpu(animated_gif *image, int size, int threshold)
  {
  }

  void apply_sobel_filter_gpu(animated_gif *image)
  {
    /**
     * Almost same code than apply_gray_filter_gpu
     **/
    int print_time = 1;
    struct timeval t1, t2;

    int im_num;
    int width = image->width[0];
    int height = image->height[0];
    int size = width * height;
    // printf("Size of the image = %d\n", size);

    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp, 0);

    /**
      * Allocation on the device once for all the images (if multiple)
      * Memory allocation + dimension of grid
      **/
    if (print_time)
      gettimeofday(&t1, NULL);

    pixel *device_image, *device_new;
    cudaMalloc(&device_image, size * sizeof(pixel));
    cudaMalloc(&device_new, size * sizeof(pixel));

    dim3 dimGrid(size / deviceProp.maxThreadsPerBlock + 1);
    dim3 dimBlock(deviceProp.maxThreadsPerBlock);


    if (print_time)
    {
      gettimeofday(&t2, NULL);
      printf("Alloc done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
      gettimeofday(&t1, NULL);
    }

    /**
      * Memory transfert + computation
      **/
    for (im_num = 0; im_num < image->n_images; im_num++)
    {
      // Memory transfer
      cudaMemcpy(device_image, image->p[im_num], size * sizeof(pixel), cudaMemcpyHostToDevice);
      if (print_time)
      {
        gettimeofday(&t2, NULL);
        printf("Transfer done in %ld us with ", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
        printf(cudaGetErrorString(cudaGetLastError()));
        printf("\n");
        gettimeofday(&t1, NULL);
      }

      // Computation
      sobel<<<dimGrid, dimBlock>>>(device_image, device_new, height, width);

      if (print_time)
      {
        gettimeofday(&t2, NULL);
        printf("Computation done in %ld us with ", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
        printf(cudaGetErrorString(cudaGetLastError()));
        printf("\n");
        gettimeofday(&t1, NULL);
      }

      // Transfer back
      cudaMemcpy(image->p[im_num], device_new, size * sizeof(pixel), cudaMemcpyDeviceToHost);
      if (print_time)
      {
        gettimeofday(&t2, NULL);
        printf("Transfer back done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
        gettimeofday(&t1, NULL);
      }
    }

    cudaFree(device_image);
    cudaFree(device_new);
  }
}