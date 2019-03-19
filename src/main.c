/*
 * INF560
 *
 * Image Filtering Project
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <mpi.h>

#include <gif_lib.h>
#include <filters.h>

int blur_size = 5;

/** 
 * Switch execution mode: 
 *  - s: seq
 *  - o: omp
 *  - m: mpi
 *  - g: gpu
 **/
char mode = 'g';

/** The main function reads the arguments and decide what to do depending
 *  on the type of image, the number of MPI rank and the number of omp threads */
int main(int argc, char **argv)
{
  /* MPI stuff */
  int rank, n_process;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &n_process);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  int root = n_process - 1;

#if SOBELF_DEBUG
  // #pragma omp parallel
  {
    char hostname[64];
    gethostname(hostname, 64);
    printf(hostname);
    printf("%d\n", omp_get_num_threads());
  }
#endif
  char *input_filename;
  char *output_filename;
  animated_gif *image;
  struct timeval t1, t2;
  double duration;

  /* Reading arguments */
  if (argc < 3)
  {
    fprintf(stderr, "Usage: %s input.gif output.gif \n", argv[0]);
    return 1;
  }
  input_filename = argv[1];
  output_filename = argv[2];

  switch (mode)
  {

  /* Code paralellized with mpi */
  case 'm':
    /* IMPORTING the original image */
    image = load_image_mpi(input_filename, root);
    if (image == NULL)
    {
      return 1;
    }

    /* FILTERING: apply filters */
    /* Decision to be taken depending on the size, nb of images... */
    apply_filters_mpi(image, output_filename, root);

    break;

  /* Code paralellized with OpenMP */
  case 'o':
    image = load_image_seq(input_filename);
    if (image == NULL)
    {
      return 1;
    }
    apply_filters_omp(image);
    export_seq(output_filename, image);

    break;

  /* Sequential */
  case 's':
    image = load_image_seq(input_filename);
    if (image == NULL)
    {
      return 1;
    }
    apply_filters_seq(image);
    export_seq(output_filename, image);

    break;

  /* GPU */
  case 'g':
    image = load_image_seq(input_filename);
    if (image == NULL)
    {
      return 1;
    }

    apply_filters_gpu(image);
    export_seq(output_filename, image);

    break;

  /* Only gray */
  case 'a':
    image = load_image_seq(input_filename);
    if (image == NULL)
    {
      return 1;
    }
    apply_gray_filter_gpu(image);
    export_seq(output_filename, image);
    break;
  }

  MPI_Finalize();
  return 0;
}

void apply_filters_seq(animated_gif *image)
{
#if DISPLAY_TIME
  struct timeval t1, t2;
  double duration;
  gettimeofday(&t1, NULL);
#endif

  /* Convert the pixels into grayscale */
  apply_gray_filter_seq(image);

#if DISPLAY_TIME
  gettimeofday(&t2, NULL);
  duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
  printf("  GRAY filter: %lf s\n", duration);
  gettimeofday(&t1, NULL);
#endif

  /* Apply blur filter with convergence value */
  apply_blur_filter_seq(image, blur_size, 20);

#if DISPLAY_TIME
  gettimeofday(&t2, NULL);
  duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
  printf("  BLUR filter: %lf s\n", duration);
  gettimeofday(&t1, NULL);
#endif

  /* Apply sobel filter on pixels */
  apply_sobel_filter_seq(image);

#if DISPLAY_TIME
  gettimeofday(&t2, NULL);
  duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
  printf("  SOBEL filter: %lf s\n", duration);
  gettimeofday(&t1, NULL);
#endif
}

void apply_filters_gpu(animated_gif *image)
{
#if DISPLAY_TIME
  struct timeval t1, t2;
  double duration;
  gettimeofday(&t1, NULL);
#endif

  /* Apply filters on the gpu */
  apply_all_filters_gpu(image, blur_size, 20);

#if DISPLAY_TIME
  gettimeofday(&t2, NULL);
  duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
  printf("  PARAL filters: %lf s\n", duration);
  gettimeofday(&t1, NULL);
#endif

//   /* Apply sobel filter on pixels */
//   apply_sobel_filter_gpu(image);

// #if DISPLAY_TIME
//   gettimeofday(&t2, NULL);
//   duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
//   printf("  SOBEL filter: %lf s\n", duration);
//   gettimeofday(&t1, NULL);
// #endif
}

void apply_filters_omp(animated_gif *image)
{
#if DISPLAY_TIME
  struct timeval t1, t2;
  double duration;
  gettimeofday(&t1, NULL);
#endif

  /* Convert the pixels into grayscale */
  apply_gray_filter_omp(image);

#if DISPLAY_TIME
  gettimeofday(&t2, NULL);
  duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
  printf("  GRAY filter: %lf s\n", duration);
  gettimeofday(&t1, NULL);
#endif

  /* Apply blur filter with convergence value */
  apply_blur_filter_omp(image, blur_size, 20);

#if DISPLAY_TIME
  gettimeofday(&t2, NULL);
  duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
  printf("  BLUR filter: %lf s\n", duration);
  gettimeofday(&t1, NULL);
#endif

  /* Apply sobel filter on pixels */
  apply_sobel_filter_omp(image);

#if DISPLAY_TIME
  gettimeofday(&t2, NULL);
  duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
  printf("  SOBEL filter: %lf s\n", duration);
  gettimeofday(&t1, NULL);
#endif
}

int apply_filters_mpi(animated_gif *image, char *output_filename, int root)
{
  /* MPI stuff */
  int rank, n_process;
  MPI_Comm_size(MPI_COMM_WORLD, &n_process);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int n_images = image->n_images;
  //Find cut
  int n_cut = 1;
  // Ensures that we cannot cut more than the number of lines
  while ((n_images * n_cut < n_process) && (n_cut < image->width[0]))
  {
    n_cut++;
  }
  //TODO : Change line
  n_cut = n_process;

#if DISPLAY_TIME
  struct timeval t1, t2;
  double duration;
  gettimeofday(&t1, NULL);
#endif

  // The second test makes sure that the blur radius is not too big for the columns after the cut.
  if (image->n_images > n_process || image->width[0] / n_cut < blur_size)
  {
    // an animated_gif is created, with 1 image, to apply filters on it
    animated_gif *work_gif = (animated_gif *)malloc(sizeof(animated_gif));
    work_gif->n_images = 1;
    work_gif->g = image->g;
    int *width = (int *)malloc(sizeof(int));
    int *height = (int *)malloc(sizeof(int));
    pixel **new_p = malloc(1 * sizeof(pixel *));

    int im, im_size;

    if (rank == root)
    {
      int node;

      for (im = 0; im < image->n_images; im++)
      {
        im_size = image->width[im] * image->height[im] * sizeof(pixel);

        node = im % n_process;

        /** If node is root, work on the image. 
          Else, receive the image from the node */

        if (node == root)
        {
          printf("  Root %d working on image n°%d\n", rank, im);
          *width = image->width[im];
          *height = image->height[im];
          work_gif->width = width;
          work_gif->height = height;

          im_size = (*width) * (*height) * sizeof(pixel);
          new_p[0] = image->p[im];
          work_gif->p = new_p;

          /* Convert the pixels into grayscale */
          apply_gray_filter_omp(work_gif);
          apply_blur_filter_omp(work_gif, blur_size, 20);
          apply_sobel_filter_omp(work_gif);

          /* Copy work_gif into the image */
          memcpy(image->p[im], work_gif->p[0], im_size);
        }

        else
        {
          MPI_Recv(image->p[im], im_size, MPI_BYTE, node, im, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
      }

#if DISPLAY_TIME
      gettimeofday(&t2, NULL);
      duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
      printf("  Paral filters: %lf s\n", duration);
      gettimeofday(&t1, NULL);
#endif

      /* Store file from array of pixels to GIF file */
      if (!store_pixels(output_filename, image))
      {
        return 1;
      }

#if DISPLAY_TIME
      gettimeofday(&t2, NULL);
      duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
      printf("  Export:       %lf s\n", duration);
#endif
    }

    else
    /* Case node != root */
    {
      int num_im_to_treat = (image->n_images - rank - 1) / n_process + 1;
      MPI_Request *send_req = malloc(num_im_to_treat * sizeof(MPI_Request));
      //MPI_Request *req;

      for (im = rank; im < image->n_images; im += n_process)
      {
        /*  */
        printf("  Node %d working on image n°%d\n", rank, im);
        *width = image->width[im];
        *height = image->height[im];
        work_gif->width = width;
        work_gif->height = height;

        im_size = (*width) * (*height) * sizeof(pixel);
        new_p[0] = image->p[im];
        work_gif->p = new_p;

        /* Convert the pixels into grayscale */
        apply_gray_filter_omp(work_gif);
        apply_blur_filter_omp(work_gif, blur_size, 20);
        apply_sobel_filter_omp(work_gif);

        // ---- Sending back to root ----
        MPI_Isend(work_gif->p[0], im_size, MPI_BYTE, root, im, MPI_COMM_WORLD, send_req++);
      }

      // TODO
      //MPI_WaitAll
    }
  }
  /* There are no good repartitions just considering the images inside a GIF,
     * we should cut each image to have a better repartition.
    */
  else
  {

    /* We thus need to cut each image n_cut times,
         * and to apply each filter to part of the image.
         * We must make sure, for sobel filter and blur filter,
         * that the information in correctly shared between the nodes 
        */
    apply_gray_filter_mpi(image, n_cut, root);
    apply_blur_filter_mpi(image, n_cut, root, blur_size, 20);
    apply_sobel_filter_mpi(image, n_cut, root);

    if (rank == root)
    {
#if DISPLAY_TIME
      gettimeofday(&t2, NULL);
      duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
      printf("  Paral filters: %lf s\n", duration);
      gettimeofday(&t1, NULL);
#endif

      /* Store file from array of pixels to GIF file */
      if (!store_pixels(output_filename, image))
      {
        return 1;
      }

#if DISPLAY_TIME
      gettimeofday(&t2, NULL);
      duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
      printf("  Export:       %lf s\n", duration);
#endif
    }
  }
}