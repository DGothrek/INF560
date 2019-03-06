/**
 * Sequential version of the filters parallelized with OpenMP.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#include <filters.h>

void apply_gray_filter_omp(animated_gif *image)
{
  int i, j, pos;
  pixel **p;

  p = image->p;

  for (i = 0; i < image->n_images; i++)
  {
    #pragma omp parallel shared(p) private(j, pos)
    {
      #pragma omp for schedule(dynamic)
      for (j = 0; j < image->height[i]; j++)
      {
        for (pos = j * image->width[i]; pos < (j + 1) * image->width[i]; pos++)
        {
          int moy;

          // moy = p[i][pos].r/4 + ( p[i][pos].g * 3/4 ) ;
          moy = (p[i][pos].r + p[i][pos].g + p[i][pos].b) / 3;
          if (moy < 0)
            moy = 0;
          if (moy > 255)
            moy = 255;

          p[i][pos].r = moy;
          p[i][pos].g = moy;
          p[i][pos].b = moy;
        }
      }
    }
  }
}



void apply_blur_filter_omp(animated_gif *image, int size, int threshold)
{
  int i, j, k;
  int width, height;
  int end = 0;
  int n_iter = 0;

  pixel **p;
  pixel *new;

  /* Get the pixels of all images */
  p = image->p;

  /* Process all images */
  for (i = 0; i < image->n_images; i++)
  {
    n_iter = 0;
    width = image->width[i];
    height = image->height[i];

    /* Allocate array of new pixels */
    new = (pixel *)malloc(width * height * sizeof(pixel));

    /* Perform at least one blur iteration */
    do
    {
      end = 1;
      n_iter++;
#pragma omp parallel shared(p, new, size, height, i, width, threshold, end) private(k) default(none)
      {
/* Apply blur on top part of image (10%) */
#pragma omp for schedule(dynamic)
        for (j = size; j < height / 10 - size; j++)
        {
          for (k = size; k < width - size; k++)
          {
            int stencil_j, stencil_k;
            int t_r = 0;
            int t_g = 0;
            int t_b = 0;

            for (stencil_j = -size; stencil_j <= size; stencil_j++)
            {
              for (stencil_k = -size; stencil_k <= size; stencil_k++)
              {
                t_r += p[i][CONV(j + stencil_j, k + stencil_k, width)].r;
                t_g += p[i][CONV(j + stencil_j, k + stencil_k, width)].g;
                t_b += p[i][CONV(j + stencil_j, k + stencil_k, width)].b;
              }
            }

            new[CONV(j, k, width)].r = t_r / ((2 * size + 1) * (2 * size + 1));
            new[CONV(j, k, width)].g = t_g / ((2 * size + 1) * (2 * size + 1));
            new[CONV(j, k, width)].b = t_b / ((2 * size + 1) * (2 * size + 1));
          }
        }

        /* Copy the middle part of the image */
        int limit = height * 0.9 + size;
#pragma omp for schedule(dynamic)
        for (j = height / 10 - size; j < limit; j++)
        {
          for (k = size; k < width - size; k++)
          {
            new[CONV(j, k, width)].r = p[i][CONV(j, k, width)].r;
            new[CONV(j, k, width)].g = p[i][CONV(j, k, width)].g;
            new[CONV(j, k, width)].b = p[i][CONV(j, k, width)].b;
          }
        }

/* Apply blur on the bottom part of the image (10%) */
#pragma omp for schedule(dynamic)
        for (j = height * 0.9 + size; j < height - size; j++)
        {
          for (k = size; k < width - size; k++)
          {
            int stencil_j, stencil_k;
            int t_r = 0;
            int t_g = 0;
            int t_b = 0;

            for (stencil_j = -size; stencil_j <= size; stencil_j++)
            {
              for (stencil_k = -size; stencil_k <= size; stencil_k++)
              {
                t_r += p[i][CONV(j + stencil_j, k + stencil_k, width)].r;
                t_g += p[i][CONV(j + stencil_j, k + stencil_k, width)].g;
                t_b += p[i][CONV(j + stencil_j, k + stencil_k, width)].b;
              }
            }

            new[CONV(j, k, width)].r = t_r / ((2 * size + 1) * (2 * size + 1));
            new[CONV(j, k, width)].g = t_g / ((2 * size + 1) * (2 * size + 1));
            new[CONV(j, k, width)].b = t_b / ((2 * size + 1) * (2 * size + 1));
          }
        }

#pragma omp for schedule(dynamic)
        for (j = 1; j < height - 1; j++)
        {
          for (k = 1; k < width - 1; k++)
          {

            float diff_r;
            float diff_g;
            float diff_b;

            diff_r = (new[CONV(j, k, width)].r - p[i][CONV(j, k, width)].r);
            diff_g = (new[CONV(j, k, width)].g - p[i][CONV(j, k, width)].g);
            diff_b = (new[CONV(j, k, width)].b - p[i][CONV(j, k, width)].b);

            if (diff_r > threshold || -diff_r > threshold ||
                diff_g > threshold || -diff_g > threshold ||
                diff_b > threshold || -diff_b > threshold)
            {
#pragma omp atom
              end = 0;
            }

            p[i][CONV(j, k, width)].r = new[CONV(j, k, width)].r;
            p[i][CONV(j, k, width)].g = new[CONV(j, k, width)].g;
            p[i][CONV(j, k, width)].b = new[CONV(j, k, width)].b;
          }
        }
      }
    } while (threshold > 0 && !end);

    free(new);
  }
}





void apply_sobel_filter_omp(animated_gif *image)
{
  int i;
  int width, height;

  pixel **p;

  p = image->p;

  for (i = 0; i < image->n_images; i++)
  {
    width = image->width[i];
    height = image->height[i];

    pixel *sobel;

    sobel = (pixel *)malloc(width * height * sizeof(pixel));
    #pragma omp parallel shared(sobel, p, width, height, i) default(none)
    {
      int j, k;
      #pragma omp for schedule(dynamic)
      for (j = 1; j < height - 1; j++)
      {
        for (k = 1; k < width - 1; k++)
        {
          int pixel_blue_no, pixel_blue_n, pixel_blue_ne;
          int pixel_blue_so, pixel_blue_s, pixel_blue_se;
          int pixel_blue_o, pixel_blue, pixel_blue_e;

          float deltaX_blue;
          float deltaY_blue;
          float val_blue;

          pixel_blue_no = p[i][CONV(j - 1, k - 1, width)].b;
          pixel_blue_n = p[i][CONV(j - 1, k, width)].b;
          pixel_blue_ne = p[i][CONV(j - 1, k + 1, width)].b;
          pixel_blue_so = p[i][CONV(j + 1, k - 1, width)].b;
          pixel_blue_s = p[i][CONV(j + 1, k, width)].b;
          pixel_blue_se = p[i][CONV(j + 1, k + 1, width)].b;
          pixel_blue_o = p[i][CONV(j, k - 1, width)].b;
          pixel_blue = p[i][CONV(j, k, width)].b;
          pixel_blue_e = p[i][CONV(j, k + 1, width)].b;

          deltaX_blue = -pixel_blue_no + pixel_blue_ne - 2 * pixel_blue_o + 2 * pixel_blue_e - pixel_blue_so + pixel_blue_se;

          deltaY_blue = pixel_blue_se + 2 * pixel_blue_s + pixel_blue_so - pixel_blue_ne - 2 * pixel_blue_n - pixel_blue_no;

          val_blue = sqrt(deltaX_blue * deltaX_blue + deltaY_blue * deltaY_blue) / 4;

          if (val_blue > 50)
          {
            sobel[CONV(j, k, width)].r = 255;
            sobel[CONV(j, k, width)].g = 255;
            sobel[CONV(j, k, width)].b = 255;
          }
          else
          {
            sobel[CONV(j, k, width)].r = 0;
            sobel[CONV(j, k, width)].g = 0;
            sobel[CONV(j, k, width)].b = 0;
          }
        }
      }

      #pragma omp for schedule(dynamic)
      for (j = 1; j < height - 1; j++)
      {
        for (k = 1; k < width - 1; k++)
        {
          p[i][CONV(j, k, width)].r = sobel[CONV(j, k, width)].r;
          p[i][CONV(j, k, width)].g = sobel[CONV(j, k, width)].g;
          p[i][CONV(j, k, width)].b = sobel[CONV(j, k, width)].b;
        }
      }
    }
    free(sobel);
  }
}