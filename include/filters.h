/**
 * Manipulating the animated gifs
 **/

#include <datatype.h>

#define SOBELF_DEBUG 0
#define DISPLAY_TIME 1
#define DISPLAY_TIME_DETAILED 0

#define CONV(l, c, nb_c) (l) * (nb_c) + (c)
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

/* Main */
int apply_filters_mpi(animated_gif *image, char *output_filename, int root);
void apply_filters_seq(animated_gif *image);
void apply_filters_gpu(animated_gif *image);
void apply_filters_omp(animated_gif *image);

/* Basic load and store */
animated_gif *load_pixels(char *filename);
int output_modified_read_gif(char *filename, GifFileType *g);
int store_pixels(char *filename, animated_gif *image);
animated_gif *load_image_seq(char *input_filename);

/* Sequential filters */
void apply_gray_filter_seq(animated_gif *image);
void apply_gray_line(animated_gif *image);
void apply_blur_filter_seq(animated_gif *image, int size, int threshold);
void apply_sobel_filter_seq(animated_gif *image);
int export_seq(char *output_filename, animated_gif *image);

/* Omp filters */
void apply_gray_filter_omp(animated_gif *image);
void apply_blur_filter_omp(animated_gif *image, int size, int threshold);
void apply_sobel_filter_omp(animated_gif *image);

/* MPI functions */
animated_gif *load_image_mpi(char *input_filename, int root);
void apply_gray_filter_mpi(animated_gif *image, int n_cut, int root);
void apply_sobel_filter_mpi(animated_gif *image, int n_cut, int root);
void apply_blur_filter_mpi(animated_gif *image, int n_cut, int root, int size, int threshold);

/* GPU functions */
#ifdef __cplusplus
extern "C"
{
#endif
    void apply_gray_filter_gpu(animated_gif *image);
    void apply_blur_filter_gpu(animated_gif *image, int size, int threshold);
    void apply_sobel_filter_gpu(animated_gif *image);
    void apply_all_filters_gpu(animated_gif *image, int size, int threshold);

#ifdef __cplusplus
}
#endif
