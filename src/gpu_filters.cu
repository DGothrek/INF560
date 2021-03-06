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

int print_time = 0;

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

__global__ void blur(pixel *im, pixel *im_new, int *end,
                     int height, int width, int size, int threshold)
{
  /** 
   * Perform one blur iteration and store in end if we need more
   **/

  int j, k, pos;

  pos = threadIdx.x + blockIdx.x * blockDim.x;
  j = pos / width;
  k = pos % width;

  if (k == 0 && j == 0)
  {
    /* One process only in charge of updating end */
    *end = 1;
  }

  if (k >= size && k < width - size)
  {

    if (j >= size && j < height / 10 - size || j >= height * 0.9 + size && j < height - size)
    {
      /* If in the top or bottom 10% :
         Apply blur on top or bottom part of image (10%) */
      int stencil_j, stencil_k;
      int t_r = 0;
      int t_g = 0;
      int t_b = 0;

      for (stencil_j = -size; stencil_j <= size; stencil_j++)
      {
        for (stencil_k = -size; stencil_k <= size; stencil_k++)
        {
          t_r += im[CONV(j + stencil_j, k + stencil_k, width)].r;
          t_g += im[CONV(j + stencil_j, k + stencil_k, width)].g;
          t_b += im[CONV(j + stencil_j, k + stencil_k, width)].b;
        }
      }

      im_new[CONV(j, k, width)].r = t_r / ((2 * size + 1) * (2 * size + 1));
      im_new[CONV(j, k, width)].g = t_g / ((2 * size + 1) * (2 * size + 1));
      im_new[CONV(j, k, width)].b = t_b / ((2 * size + 1) * (2 * size + 1));
    }

    if (j >= height / 10 - size && j < height * 0.9 + size)
    {
      /* Just copy the middle part of the image */
      im_new[CONV(j, k, width)].r = im[CONV(j, k, width)].r;
      im_new[CONV(j, k, width)].g = im[CONV(j, k, width)].g;
      im_new[CONV(j, k, width)].b = im[CONV(j, k, width)].b;
    }
  }

  // Wait until all threads have written in the memory
  __threadfence();

  // Test the end condition
  if (j >= 1 && j < height - 1 && k >= 1 && k < width - 1)
  {
    float diff_r;
    float diff_g;
    float diff_b;

    diff_r = (im_new[CONV(j, k, width)].r - im[CONV(j, k, width)].r);
    diff_g = (im_new[CONV(j, k, width)].g - im[CONV(j, k, width)].g);
    diff_b = (im_new[CONV(j, k, width)].b - im[CONV(j, k, width)].b);

    if (diff_r > threshold || -diff_r > threshold ||
        diff_g > threshold || -diff_g > threshold ||
        diff_b > threshold || -diff_b > threshold)
    {
      *end = 0;
    }
  }

  // Wait for all the threads to have tested the end condition
  __threadfence();

  if (j >= 1 && j < height - 1 && k >= 1 && k < width - 1)
  {
    // Erase and copy for new iteration
    im[CONV(j, k, width)].r = im_new[CONV(j, k, width)].r;
    im[CONV(j, k, width)].g = im_new[CONV(j, k, width)].g;
    im[CONV(j, k, width)].b = im_new[CONV(j, k, width)].b;
  }
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

  if (i >= 1 && i < height - 1 && j >= 1 && j < width - 1)
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
    if (i < height && j < width)
    {
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
      printf("    Alloc done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
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
        printf("    Transfer done in %ld us with ", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
        printf(cudaGetErrorString(cudaGetLastError()));
        printf("\n");
        gettimeofday(&t1, NULL);
      }

      // Computation
      gray<<<dimGrid, dimBlock>>>(device_image, height, width);

      if (print_time)
      {
        gettimeofday(&t2, NULL);
        printf("    Computation done in %ld us with ", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
        printf(cudaGetErrorString(cudaGetLastError()));
        printf("\n");
        gettimeofday(&t1, NULL);
      }

      // Transfer back
      cudaMemcpy(image->p[im_num], device_image, size * sizeof(pixel), cudaMemcpyDeviceToHost);
      if (print_time)
      {
        gettimeofday(&t2, NULL);
        printf("    Transfer back done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
        gettimeofday(&t1, NULL);
      }
    }

    cudaFree(device_image);
  }

  void apply_blur_filter_gpu(animated_gif *image, int blur_size, int threshold)
  {
    // struct timeval t1, t2;

    int im_num;
    int width = image->width[0];
    int height = image->height[0];
    int size = width * height;

    int *end_dev, end_host;

    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp, 0);

    /**
      * Allocation on the device once for all the images (if multiple)
      * Memory allocation + dimension of grid
      **/
    pixel *device_image, *device_new;
    cudaMalloc(&device_image, size * sizeof(pixel));
    cudaMalloc(&device_new, size * sizeof(pixel));
    cudaMalloc(&end_dev, sizeof(int));

    dim3 dimGrid(size / deviceProp.maxThreadsPerBlock + 1);
    dim3 dimBlock(deviceProp.maxThreadsPerBlock);

    /* Process all images */
    for (im_num = 0; im_num < image->n_images; im_num++)
    {
      // Memory transfer
      cudaMemcpy(device_image, image->p[im_num], size * sizeof(pixel), cudaMemcpyHostToDevice);

      // Computation
      int num_iter = 0;
      end_host = 1;
      do
      {
        num_iter++;
        blur<<<dimGrid, dimBlock>>>(device_image, device_new, end_dev, height, width, blur_size, threshold);
        cudaMemcpy(&end_host, end_dev, sizeof(int), cudaMemcpyDeviceToHost);
      } while (threshold > 0 && !end_host);

      // Transfer back: could be optimized because only the first and last 10% will change
      cudaMemcpy(image->p[im_num], device_new, size * sizeof(pixel), cudaMemcpyDeviceToHost);
      if (print_time)
      {
        printf("    Image %d processed in %d iteration(s) and ", im_num, ++num_iter);
        printf(cudaGetErrorString(cudaGetLastError()));
        printf("\n");
      }
    }

    cudaFree(device_image);
    cudaFree(device_new);
    cudaFree(end_dev);
  }

  void apply_sobel_filter_gpu(animated_gif *image)
  {
    /**
     * Almost same code than apply_gray_filter_gpu
     **/
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
      printf("    Alloc done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
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
        printf("    Transfer done in %ld us with ", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
        printf(cudaGetErrorString(cudaGetLastError()));
        printf("\n");
        gettimeofday(&t1, NULL);
      }

      // Computation
      sobel<<<dimGrid, dimBlock>>>(device_image, device_new, height, width);

      if (print_time)
      {
        gettimeofday(&t2, NULL);
        printf("    Computation done in %ld us with ", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
        printf(cudaGetErrorString(cudaGetLastError()));
        printf("\n");
        gettimeofday(&t1, NULL);
      }

      // Transfer back
      cudaMemcpy(image->p[im_num], device_new, size * sizeof(pixel), cudaMemcpyDeviceToHost);
      if (print_time)
      {
        gettimeofday(&t2, NULL);
        printf("    Transfer back done in %ld us\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
        gettimeofday(&t1, NULL);
      }
    }

    cudaFree(device_image);
    cudaFree(device_new);
  }
}

void apply_all_filters_gpu(animated_gif *image, int blur_size, int threshold)
{
  /** 
    * Apply the three last filters with the help of the GPU.
    * To avoid memcopying 3 times, we merged apply_gray, apply_blur and apply_sobel
    **/

  int im_num;
  int width = image->width[0];
  int height = image->height[0];
  int size = width * height;

  int *end_dev, end_host; // to know when blur has finished

  cudaDeviceProp deviceProp;
  cudaGetDeviceProperties(&deviceProp, 0);

  /**
     * Allocation on the device once for all the images (if multiple)
     * Memory allocation + dimension of grid
     **/
  dim3 dimGrid(10*size / deviceProp.maxThreadsPerBlock + 1);
  dim3 dimBlock(deviceProp.maxThreadsPerBlock/10);

  pixel *device_image, *device_new;
  cudaMalloc(&device_image, size * sizeof(pixel));
  cudaMalloc(&device_new, size * sizeof(pixel));
  cudaMalloc(&end_dev, sizeof(int));

  /* For all images, blur than sobel */
  for (im_num = 0; im_num < image->n_images; im_num++)
  {
    cudaMemcpy(device_image, image->p[im_num], size * sizeof(pixel), cudaMemcpyHostToDevice);

    gray<<<dimGrid, dimBlock>>>(device_image, height, width);
    /* Bluring while it isn't finished */
    int num_iter = 0;
    end_host = 1;
    do
    {
      num_iter++;
      blur<<<dimGrid, dimBlock>>>(device_image, device_new, end_dev, height, width, blur_size, threshold);
      cudaMemcpy(&end_host, end_dev, sizeof(int), cudaMemcpyDeviceToHost);
    } while (threshold > 0 && !end_host);

    /* Applying sobel */
    sobel<<<dimGrid, dimBlock>>>(device_image, device_new, height, width);

    cudaMemcpy(image->p[im_num], device_new, size * sizeof(pixel), cudaMemcpyDeviceToHost);
  }

  cudaFree(device_image);
  cudaFree(device_new);
  cudaFree(end_dev);
}