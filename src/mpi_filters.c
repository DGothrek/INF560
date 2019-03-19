/**
 * MPI version of filters.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <mpi.h>

#include <filters.h>

/**
 * For the moment, only to ensure that only root diplay things
 */
animated_gif *load_image_mpi(char *input_filename, int root)
{
  animated_gif *image;
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

#if DISPLAY_TIME
  struct timeval t1, t2;
  double duration;
  gettimeofday(&t1, NULL);
#endif

  /* Load file and store the pixels in array */
  image = load_pixels(input_filename);

#if DISPLAY_TIME
  gettimeofday(&t2, NULL);
  duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);

  if (rank == root)
  {
    printf("GIF loaded from file %s with %d image(s)\n",
           input_filename, image->n_images);
    printf("EXECUTION TIME:\n");
    printf("  LOADING:      %lf s\n", duration);
  }
#endif

  return image;
}

/**
 * Applying gray filter like seq function, but only on some columns of the array.
 * For the moment it does not use OpenMP.
 */
void apply_gray_filter_mpi(animated_gif *image, int n_cut, int root)
{
  int n_process, rank;
  int i, j, pos;
  pixel **p;

  MPI_Comm_size(MPI_COMM_WORLD, &n_process);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  p = image->p;

  for (i = 0; i < image->n_images * n_cut; i++)
  {

#if SOBELF_DEBUG
    if (rank == root)
    {
      printf("Width %d, height %d\n", image->width[0], image->height[0]);
    }
#endif

    if (i % n_process == rank)
    {

      int portion = i % n_cut;
      int image_n = (i - portion) / n_cut;
      // printf("Processing portion %d of image %d with rank %d.\n", portion, image_n, rank);
      int work_width = image->width[image_n] / n_cut;
      int start_column = portion * work_width;
      int end_column = (portion + 1) * work_width;

      // Delegate the remaining columns to the same node
      if (portion + 1 == n_cut)
      {
        end_column = image->width[image_n];
      }

#pragma omp parallel shared(p, image, image_n, start_column, end_column) private(j, pos) default(none)
      {
#pragma omp for schedule(dynamic)
        for (j = 0; j < image->height[image_n]; j++)
        {
          for (pos = (j * image->width[image_n]) + start_column; pos < (j * image->width[image_n]) + end_column; pos++)
          {
            int moy;

            // moy = p[i][pos].r/4 + ( p[i][pos].g * 3/4 ) ;
            moy = (p[image_n][pos].r + p[image_n][pos].g + p[image_n][pos].b) / 3;
            if (moy < 0)
              moy = 0;
            if (moy > 255)
              moy = 255;

            p[image_n][pos].r = moy;
            p[image_n][pos].g = moy;
            p[image_n][pos].b = moy;
          }
        }
      }
    }
  }
  /* Here we are forced to wait for everyone to join,
     * as two nodes could be working on the same image,
     * and both need the information to continue working.
     */
  // MPI_Barrier(MPI_COMM_WORLD);
}

void apply_blur_filter_mpi(animated_gif *image, int n_cut, int root, int size, int threshold)
{
  int n_process, rank, n_request;

  int i, j, k, id;
  int width, height;
  int end = 0;
  int end_root;
  int n_iter = 0;

  pixel **p;
  pixel *new;

  MPI_Comm_size(MPI_COMM_WORLD, &n_process);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  /* Get the pixels of all images */
  p = image->p;

  /* Process all images */
  for (i = 0; i < image->n_images; i++)
  {

    width = image->width[i];
    height = image->height[i];
    n_iter = 0;

    /* Allocate array of new pixels */
    new = (pixel *)malloc(width * height * sizeof(pixel));

    /* Perform at least one blur iteration */
    do
    {
      end = 1;
      end_root = 1;
      n_iter++;

      n_request = 0;

      for (id = i * n_cut; id < (i + 1) * n_cut; id++)
      {
        int portion = id % n_cut;
        int image_n = i;
        if (id % n_process == rank)
        {
          if (portion != 0 && portion != n_cut - 1)
          {
            n_request += 2;
          }
          else
          {
            n_request++;
          }
        }
      }
      MPI_Request *send_request;
      MPI_Status *status;

      send_request = (MPI_Request *)malloc(n_request * sizeof(MPI_Request));
      int counter = 0;

      for (id = i * n_cut; id < (i + 1) * n_cut; id++)
      {
        int portion = id % n_cut;
        int image_n = i;

        // printf("Processing portion %d of image %d with rank %d.\n", portion, image_n, rank);

        int work_width = image->width[image_n] / n_cut;
        int start_column = portion * work_width;
        int end_column = (portion + 1) * work_width;

        // Send the data to the adjacent columns that need it to process.

        MPI_Datatype columnstype;
        MPI_Status recv_status;

        MPI_Type_vector(image->height[image_n], size * sizeof(pixel),
                        image->width[image_n] * sizeof(pixel), MPI_BYTE, &columnstype);
        MPI_Type_commit(&columnstype);

        /* Check if the node is processing the first part of the columns :
                 * No point in sending the data if so.
                 */
        if (id % n_process == rank)
        {
          if (portion != 0)
          {
            // Send to the node before so that it can use the correct information to update its blur.
            MPI_Isend(p[image_n] + start_column, 1, columnstype,
                      (rank - 1) % n_process, 2 * id, MPI_COMM_WORLD, send_request++);
            counter++;
          }
          if (portion != n_cut - 1)
          {
            MPI_Isend(p[image_n] + (end_column - size), 1, columnstype,
                      (rank + 1) % n_process, 2 * id + 1, MPI_COMM_WORLD, send_request++);
            counter++;
          }
        }

        if ((id - 1) % n_process == rank && portion != 0)
        {
          MPI_Recv(p[image_n] + start_column, 1, columnstype,
                   (rank + 1) % n_process, 2 * id, MPI_COMM_WORLD, &recv_status);
        }
        if ((id + 1) % n_process == rank && portion != n_cut - 1)
        {
          MPI_Recv(p[image_n] + (end_column - size), 1, columnstype,
                   (rank - 1) % n_process, 2 * id + 1, MPI_COMM_WORLD, &recv_status);
        }

        MPI_Type_free(&columnstype);
      }

      if (n_process > 1)
      {
        MPI_Waitall(n_request, send_request - n_request, MPI_STATUSES_IGNORE);
      }

      for (id = i * n_cut; id < (i + 1) * n_cut; id++)
      {
        int portion = id % n_cut;
        int image_n = i;

        // printf("Processing portion %d of image %d with rank %d.\n", portion, image_n, rank);

        int work_width = image->width[image_n] / n_cut;
        int start_column = portion * work_width;
        int end_column = (portion + 1) * work_width;

        if (id % n_process == rank)
        {
#pragma omp parallel shared(p, new, size, height, i, width, threshold, end, id, start_column, end_column) private(k) default(none)
          {
/* Apply blur on top part of image (10%) */
#pragma omp for schedule(dynamic)
            for (j = size; j < height / 10 - size; j++)
            {
              for (k = MAX(start_column, size); k < MIN(end_column, width - size); k++)
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
              for (k = MAX(start_column, size); k < MIN(end_column, width - size); k++)
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
              for (k = MAX(start_column, size); k < MIN(end_column, width - size); k++)
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

// Check if need to stop.
#pragma omp for schedule(dynamic)
            for (j = 1; j < height - 1; j++)
            {
              for (k = MAX(start_column, 1); k < MIN(end_column, width - 1); k++)
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

                // Update the values of p
                p[i][CONV(j, k, width)].r = new[CONV(j, k, width)].r;
                p[i][CONV(j, k, width)].g = new[CONV(j, k, width)].g;
                p[i][CONV(j, k, width)].b = new[CONV(j, k, width)].b;
              }
            }
          }
        }
      }

      // Everybody gets the information of the end variable and knows whether it should continue or not.
      MPI_Allreduce(&end, &end, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);

    } while (threshold > 0 && !end);

#if SOBELF_DEBUG
    if (rank == root)
    {
      printf("Iterations: %d\n", n_iter);
    }
#endif

    // MPI_Barrier(MPI_COMM_WORLD);
    // return;
    free(new);

    int portion, image_n, work_width, start_column, end_column;

    /* Send all to root for the moment*/
    for (id = i * n_cut; id < (i + 1) * n_cut; id++)
    {
      portion = id % n_cut;
      image_n = i;
      work_width = image->width[image_n] / n_cut;
      start_column = portion * work_width;
      end_column = (portion + 1) * work_width;
      if (portion + 1 == n_cut)
      {
        end_column = image->width[image_n];
      }

      MPI_Request send_request, recv_request;
      MPI_Datatype columnstype;

      MPI_Type_vector(image->height[image_n], (end_column - start_column) * sizeof(pixel),
                      image->width[image_n] * sizeof(pixel), MPI_BYTE, &columnstype);
      MPI_Type_commit(&columnstype);

      if (rank != root && id % n_process == rank)
      {
#if SOBELF_DEBUG
        printf("Sending portion %d from image %d with rank %d.\n", portion, image_n, rank);
#endif
        // Sending all the columns at once to root.
        MPI_Isend(p[image_n] + start_column, 1, columnstype,
                  root, id, MPI_COMM_WORLD, &send_request);
      }
      // Because root already has the information it processed.
      if (rank == root && id % n_process != root)
      {
#if SOBELF_DEBUG
        printf("Recieving portion %d from rank %d\n", portion, id % n_process);
#endif
        MPI_Irecv(p[image_n] + start_column, 1, columnstype,
                  id % n_process, id, MPI_COMM_WORLD, &recv_request);
      }
      MPI_Type_free(&columnstype);
    }
  }
}

void apply_sobel_filter_mpi(animated_gif *image, int n_cut, int root)
{
  int n_process, rank;
  int i, j, pos, width, height;
  pixel **p;

  MPI_Comm_size(MPI_COMM_WORLD, &n_process);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  p = image->p;

  for (i = 0; i < image->n_images * n_cut; i++)
  {
    pixel *sobel;

    if (i % n_process == rank)
    {
      int portion = i % n_cut;
      int image_n = (i - portion) / n_cut;
      // printf("Processing portion %d of image %d with rank %d.\n", portion, image_n, rank);
      int work_width = image->width[image_n] / n_cut;
      int start_column = portion * work_width;
      int end_column = (portion + 1) * work_width;

      // Delegate the remaining columns to the same node
      if (portion + 1 == n_cut)
      {
        end_column = image->width[image_n];
      }
      width = image->width[image_n];
      height = image->height[image_n];
      sobel = (pixel *)malloc(width * height * sizeof(pixel));

#pragma omp parallel shared(sobel, p, width, height, i, start_column, end_column, image_n) private(j) default(none)
      {
        int k;
#pragma omp for schedule(dynamic)
        for (j = 1; j < height - 1; j++)
        {
          for (k = MAX(start_column, 1); k < MIN(end_column, width - 1); k++)
          {
            int pixel_blue_no, pixel_blue_n, pixel_blue_ne;
            int pixel_blue_so, pixel_blue_s, pixel_blue_se;
            int pixel_blue_o, pixel_blue, pixel_blue_e;

            float deltaX_blue;
            float deltaY_blue;
            float val_blue;

            pixel_blue_no = p[image_n][CONV(j - 1, k - 1, width)].b;
            pixel_blue_n = p[image_n][CONV(j - 1, k, width)].b;
            pixel_blue_ne = p[image_n][CONV(j - 1, k + 1, width)].b;
            pixel_blue_so = p[image_n][CONV(j + 1, k - 1, width)].b;
            pixel_blue_s = p[image_n][CONV(j + 1, k, width)].b;
            pixel_blue_se = p[image_n][CONV(j + 1, k + 1, width)].b;
            pixel_blue_o = p[image_n][CONV(j, k - 1, width)].b;
            pixel_blue = p[image_n][CONV(j, k, width)].b;
            pixel_blue_e = p[image_n][CONV(j, k + 1, width)].b;

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
          for (k = MAX(start_column, 1); k < MIN(end_column, width - 1); k++)
          {
            p[image_n][CONV(j, k, width)].r = sobel[CONV(j, k, width)].r;
            p[image_n][CONV(j, k, width)].g = sobel[CONV(j, k, width)].g;
            p[image_n][CONV(j, k, width)].b = sobel[CONV(j, k, width)].b;
          }
        }
      }
      free(sobel);
    }
  }

  /*
     * Then send all the data back to root for exporting.
     */
  int id, portion, image_n, work_width, start_column, end_column;

  for (id = 0; id < image->n_images * n_cut; id++)
  {
    portion = id % n_cut;
    image_n = (id - portion) / n_cut;
    work_width = image->width[image_n] / n_cut;
    start_column = portion * work_width;
    end_column = (portion + 1) * work_width;
    if (portion + 1 == n_cut)
    {
      end_column = image->width[image_n];
    }

    MPI_Status status;
    MPI_Request send_request, recv_request;
    MPI_Datatype columnstype;

    MPI_Type_vector(image->height[image_n], (end_column - start_column) * sizeof(pixel),
                    image->width[image_n] * sizeof(pixel), MPI_BYTE, &columnstype);
    MPI_Type_commit(&columnstype);

    if (rank != root && id % n_process == rank)
    {
#if SOBELF_DEBUG
      printf("Sending portion %d from image %d with rank %d.\n", portion, image_n, rank);
#endif
      // Sending all the columns at once to root.
      MPI_Isend(p[image_n] + start_column, 1, columnstype,
                root, id, MPI_COMM_WORLD, &send_request);
    }
    // Because root already has the information it processed.
    if (rank == root && id % n_process != root)
    {
#if SOBELF_DEBUG
      printf("Recieving portion %d from rank %d", portion, id % n_process);
#endif
      MPI_Recv(p[image_n] + start_column, 1, columnstype,
               id % n_process, id, MPI_COMM_WORLD, &status);
    }
    MPI_Type_free(&columnstype);
  }
  return;
}
