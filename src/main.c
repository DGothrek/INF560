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

#define SOBELF_DEBUG 0
#define DISPLAY_TIME 1
#define DISPLAY_TIME_DETAILED 0

/* Represent one pixel from the image */
typedef struct pixel
{
    int r; /* Red */
    int g; /* Green */
    int b; /* Blue */
} pixel;

/* Represent one GIF image (animated or not */
typedef struct animated_gif
{
    int n_images;   /* Number of images */
    int *width;     /* Width of each image */
    int *height;    /* Height of each image */
    pixel **p;      /* Pixels of each image */
    GifFileType *g; /* Internal representation.
                         DO NOT MODIFY */
} animated_gif;

/*
 * Load a GIF image from a file and return a
 * structure of type animated_gif.
 */
animated_gif *
load_pixels(char *filename)
{
    GifFileType *g;
    ColorMapObject *colmap;
    int error;
    int n_images;
    int *width;
    int *height;
    pixel **p;
    int i;
    animated_gif *image;

    /* Open the GIF image (read mode) */
    g = DGifOpenFileName(filename, &error);
    if (g == NULL)
    {
        fprintf(stderr, "Error DGifOpenFileName %s\n", filename);
        return NULL;
    }

    /* Read the GIF image */
    error = DGifSlurp(g);
    if (error != GIF_OK)
    {
        fprintf(stderr,
                "Error DGifSlurp: %d <%s>\n", error, GifErrorString(g->Error));
        return NULL;
    }

    /* Grab the number of images and the size of each image */
    n_images = g->ImageCount;

    width = (int *)malloc(n_images * sizeof(int));
    if (width == NULL)
    {
        fprintf(stderr, "Unable to allocate width of size %d\n",
                n_images);
        return 0;
    }

    height = (int *)malloc(n_images * sizeof(int));
    if (height == NULL)
    {
        fprintf(stderr, "Unable to allocate height of size %d\n",
                n_images);
        return 0;
    }

    /* Fill the width and height */
    for (i = 0; i < n_images; i++)
    {
        width[i] = g->SavedImages[i].ImageDesc.Width;
        height[i] = g->SavedImages[i].ImageDesc.Height;

#if SOBELF_DEBUG
        printf("Image %d: l:%d t:%d w:%d h:%d interlace:%d localCM:%p\n",
               i,
               g->SavedImages[i].ImageDesc.Left,
               g->SavedImages[i].ImageDesc.Top,
               g->SavedImages[i].ImageDesc.Width,
               g->SavedImages[i].ImageDesc.Height,
               g->SavedImages[i].ImageDesc.Interlace,
               g->SavedImages[i].ImageDesc.ColorMap);
#endif
    }

    /* Get the global colormap */
    colmap = g->SColorMap;
    if (colmap == NULL)
    {
        fprintf(stderr, "Error global colormap is NULL\n");
        return NULL;
    }

#if SOBELF_DEBUG
    printf("Global CM: count:%d bpp:%d sort:%d\n",
           g->SColorMap->ColorCount,
           g->SColorMap->BitsPerPixel,
           g->SColorMap->SortFlag);
#endif

    /* Allocate the array of pixels to be returned */
    p = (pixel **)malloc(n_images * sizeof(pixel *));
    if (p == NULL)
    {
        fprintf(stderr, "Unable to allocate array of %d images\n",
                n_images);
        return NULL;
    }

    for (i = 0; i < n_images; i++)
    {
        p[i] = (pixel *)malloc(width[i] * height[i] * sizeof(pixel));
        if (p[i] == NULL)
        {
            fprintf(stderr, "Unable to allocate %d-th array of %d pixels\n",
                    i, width[i] * height[i]);
            return NULL;
        }
    }

    /* Fill pixels */

    /* For each image */
    for (i = 0; i < n_images; i++)
    {
        int j;

        /* Get the local colormap if needed */
        if (g->SavedImages[i].ImageDesc.ColorMap)
        {

            /* TODO No support for local color map */
            fprintf(stderr, "Error: application does not support local colormap\n");
            return NULL;

            colmap = g->SavedImages[i].ImageDesc.ColorMap;
        }

        /* Traverse the image and fill pixels */
        for (j = 0; j < width[i] * height[i]; j++)
        {
            int c;

            c = g->SavedImages[i].RasterBits[j];

            p[i][j].r = colmap->Colors[c].Red;
            p[i][j].g = colmap->Colors[c].Green;
            p[i][j].b = colmap->Colors[c].Blue;
        }
    }

    /* Allocate image info */
    image = (animated_gif *)malloc(sizeof(animated_gif));
    if (image == NULL)
    {
        fprintf(stderr, "Unable to allocate memory for animated_gif\n");
        return NULL;
    }

    /* Fill image fields */
    image->n_images = n_images;
    image->width = width;
    image->height = height;
    image->p = p;
    image->g = g;

#if SOBELF_DEBUG
    printf("-> GIF w/ %d image(s) with first image of size %d x %d\n",
           image->n_images, image->width[0], image->height[0]);
#endif

    return image;
}

int output_modified_read_gif(char *filename, GifFileType *g)
{
    GifFileType *g2;
    int error2;

#if SOBELF_DEBUG
    printf("Starting output to file %s\n", filename);
#endif

    g2 = EGifOpenFileName(filename, false, &error2);
    if (g2 == NULL)
    {
        fprintf(stderr, "Error EGifOpenFileName %s\n",
                filename);
        return 0;
    }

    g2->SWidth = g->SWidth;
    g2->SHeight = g->SHeight;
    g2->SColorResolution = g->SColorResolution;
    g2->SBackGroundColor = g->SBackGroundColor;
    g2->AspectByte = g->AspectByte;
    g2->SColorMap = g->SColorMap;
    g2->ImageCount = g->ImageCount;
    g2->SavedImages = g->SavedImages;
    g2->ExtensionBlockCount = g->ExtensionBlockCount;
    g2->ExtensionBlocks = g->ExtensionBlocks;

    error2 = EGifSpew(g2);
    if (error2 != GIF_OK)
    {
        fprintf(stderr, "Error after writing g2: %d <%s>\n",
                error2, GifErrorString(g2->Error));
        return 0;
    }

    return 1;
}

/* Stores
 */
int store_pixels(char *filename, animated_gif *image)
{
    int n_colors = 0;
    pixel **p;
    int i, j, k, l;
    GifColorType *colormap;

    /* Initialize the new set of colors */
    colormap = (GifColorType *)malloc(256 * sizeof(GifColorType));
    if (colormap == NULL)
    {
        fprintf(stderr,
                "Unable to allocate 256 colors\n");
        return 0;
    }

    /* Everything is white by default */
    for (i = 0; i < 256; i++)
    {
        colormap[i].Red = 255;
        colormap[i].Green = 255;
        colormap[i].Blue = 255;
    }

    /* Change the background color and store it */
    int moy;
    moy = (image->g->SColorMap->Colors[image->g->SBackGroundColor].Red +
           image->g->SColorMap->Colors[image->g->SBackGroundColor].Green +
           image->g->SColorMap->Colors[image->g->SBackGroundColor].Blue) /
          3;
    if (moy < 0)
        moy = 0;
    if (moy > 255)
        moy = 255;

#if SOBELF_DEBUG
    printf("[DEBUG] Background color (%d,%d,%d) -> (%d,%d,%d)\n",
           image->g->SColorMap->Colors[image->g->SBackGroundColor].Red,
           image->g->SColorMap->Colors[image->g->SBackGroundColor].Green,
           image->g->SColorMap->Colors[image->g->SBackGroundColor].Blue,
           moy, moy, moy);
#endif

    colormap[0].Red = moy;
    colormap[0].Green = moy;
    colormap[0].Blue = moy;

    image->g->SBackGroundColor = 0;

    n_colors++;

#if DISPLAY_TIME_DETAILED
    struct timeval t1, t2;
    double duration;
    gettimeofday(&t1, NULL);
#endif

    /* Process extension blocks in main structure. First step, quite fast. */
    for (j = 0; j < image->g->ExtensionBlockCount; j++)
    {
        int f;

        f = image->g->ExtensionBlocks[j].Function;
        if (f == GRAPHICS_EXT_FUNC_CODE)
        {
            int tr_color = image->g->ExtensionBlocks[j].Bytes[3];

            if (tr_color >= 0 &&
                tr_color < 255)
            {

                int found = -1;

                moy =
                    (image->g->SColorMap->Colors[tr_color].Red +
                     image->g->SColorMap->Colors[tr_color].Green +
                     image->g->SColorMap->Colors[tr_color].Blue) /
                    3;
                if (moy < 0)
                    moy = 0;
                if (moy > 255)
                    moy = 255;

#if SOBELF_DEBUG
                printf("[DEBUG] Transparency color image %d (%d,%d,%d) -> (%d,%d,%d)\n",
                       i,
                       image->g->SColorMap->Colors[tr_color].Red,
                       image->g->SColorMap->Colors[tr_color].Green,
                       image->g->SColorMap->Colors[tr_color].Blue,
                       moy, moy, moy);
#endif

                for (k = 0; k < n_colors; k++)
                {
                    if (
                        moy == colormap[k].Red &&
                        moy == colormap[k].Green &&
                        moy == colormap[k].Blue)
                    {
                        found = k;
                    }
                }
                if (found == -1)
                {
                    if (n_colors >= 256)
                    {
                        fprintf(stderr,
                                "Error: Found too many colors inside the image\n");
                        return 0;
                    }

#if SOBELF_DEBUG
                    printf("[DEBUG]\tNew color %d\n",
                           n_colors);
#endif

                    colormap[n_colors].Red = moy;
                    colormap[n_colors].Green = moy;
                    colormap[n_colors].Blue = moy;

                    image->g->ExtensionBlocks[j].Bytes[3] = n_colors;

                    n_colors++;
                }
                else
                {
#if SOBELF_DEBUG
                    printf("[DEBUG]\tFound existing color %d\n",
                           found);
#endif
                    image->g->ExtensionBlocks[j].Bytes[3] = found;
                }
            }
        }
    }

#if DISPLAY_TIME_DETAILED
    gettimeofday(&t2, NULL);
    duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
    printf("  LOAD1: %lf s\n", duration);
    gettimeofday(&t1, NULL);
#endif

    // Second step, very fast aswell.
    for (i = 0; i < image->n_images; i++)
    {
        for (j = 0; j < image->g->SavedImages[i].ExtensionBlockCount; j++)
        {
            int f;

            f = image->g->SavedImages[i].ExtensionBlocks[j].Function;
            if (f == GRAPHICS_EXT_FUNC_CODE)
            {
                int tr_color = image->g->SavedImages[i].ExtensionBlocks[j].Bytes[3];

                if (tr_color >= 0 &&
                    tr_color < 255)
                {

                    int found = -1;

                    moy =
                        (image->g->SColorMap->Colors[tr_color].Red +
                         image->g->SColorMap->Colors[tr_color].Green +
                         image->g->SColorMap->Colors[tr_color].Blue) /
                        3;
                    if (moy < 0)
                        moy = 0;
                    if (moy > 255)
                        moy = 255;

#if SOBELF_DEBUG
                    printf("[DEBUG] Transparency color image %d (%d,%d,%d) -> (%d,%d,%d)\n",
                           i,
                           image->g->SColorMap->Colors[tr_color].Red,
                           image->g->SColorMap->Colors[tr_color].Green,
                           image->g->SColorMap->Colors[tr_color].Blue,
                           moy, moy, moy);
#endif

                    for (k = 0; k < n_colors; k++)
                    {
                        if (
                            moy == colormap[k].Red &&
                            moy == colormap[k].Green &&
                            moy == colormap[k].Blue)
                        {
                            found = k;
                        }
                    }
                    if (found == -1)
                    {
                        if (n_colors >= 256)
                        {
                            fprintf(stderr,
                                    "Error: Found too many colors inside the image\n");
                            return 0;
                        }

#if SOBELF_DEBUG
                        printf("[DEBUG]\tNew color %d\n",
                               n_colors);
#endif

                        colormap[n_colors].Red = moy;
                        colormap[n_colors].Green = moy;
                        colormap[n_colors].Blue = moy;

                        image->g->SavedImages[i].ExtensionBlocks[j].Bytes[3] = n_colors;

                        n_colors++;
                    }
                    else
                    {
#if SOBELF_DEBUG
                        printf("[DEBUG]\tFound existing color %d\n",
                               found);
#endif
                        image->g->SavedImages[i].ExtensionBlocks[j].Bytes[3] = found;
                    }
                }
            }
        }
    }

#if DISPLAY_TIME_DETAILED
    gettimeofday(&t2, NULL);
    duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
    printf("  LOAD2: %lf s\n", duration);
    gettimeofday(&t1, NULL);
#endif

#if SOBELF_DEBUG
    printf("[DEBUG] Number of colors after background and transparency: %d\n",
           n_colors);
#endif

    p = image->p;

    /* Find the number of colors inside the image */
    // Third step, slower -> to optimize!
    for (i = 0; i < image->n_images; i++)
    {

#if SOBELF_DEBUG
        printf("OUTPUT: Processing image %d (total of %d images) -> %d x %d\n",
               i, image->n_images, image->width[i], image->height[i]);
#endif
        int error_n_colors = 0;

#pragma omp parallel shared(n_colors, image, p, i, colormap, error_n_colors) private(j, k) default(none)
        {
#pragma omp for schedule(dynamic)
            for (j = 0; j < image->height[i]; j++)
            {
                int pos;
                for (pos = j * image->width[i]; pos < (j + 1) * image->width[i]; pos++)
                {
                    int found = 0;
                    for (k = 0; k < n_colors; k++)
                    {
                        if (p[i][pos].r == colormap[k].Red &&
                            p[i][pos].g == colormap[k].Green &&
                            p[i][pos].b == colormap[k].Blue)
                        {
                            found = 1;
                        }
                    }

                    if (found == 0)
                    {
#pragma omp critical
                        {
                            if (n_colors >= 256)
                            {
                                n_colors--;
                                error_n_colors = 1;
                            }

#if SOBELF_DEBUG
                            printf("[DEBUG] Found new %d color (%d,%d,%d)\n",
                                   n_colors, p[i][pos].r, p[i][pos].g, p[i][pos].b);
#endif

                            colormap[n_colors].Red = p[i][pos].r;
                            colormap[n_colors].Green = p[i][pos].g;
                            colormap[n_colors].Blue = p[i][pos].b;
                            n_colors++;
                        }
                    }
                }
            }
        }
        if (error_n_colors)
        {
            fprintf(stderr,
                    "Error: Found too many colors inside the image\n");
            return 0;
        }
    }

#if DISPLAY_TIME_DETAILED
    gettimeofday(&t2, NULL);
    duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
    printf("  LOAD3: %lf s\n", duration);
    gettimeofday(&t1, NULL);
#endif

#if SOBELF_DEBUG
    printf("OUTPUT: found %d color(s)\n", n_colors);
#endif

    /* Round up to a power of 2 */
    if (n_colors != (1 << GifBitSize(n_colors)))
    {
        n_colors = (1 << GifBitSize(n_colors));
    }

#if SOBELF_DEBUG
    printf("OUTPUT: Rounding up to %d color(s)\n", n_colors);
#endif

    /* Change the color map inside the animated gif */
    ColorMapObject *cmo;

    cmo = GifMakeMapObject(n_colors, colormap);
    if (cmo == NULL)
    {
        fprintf(stderr, "Error while creating a ColorMapObject w/ %d color(s)\n",
                n_colors);
        return 0;
    }

    image->g->SColorMap = cmo;

    /* Update the raster bits according to color map */
    // Fourth step, also long.
    for (i = 0; i < image->n_images; i++)
    {
        int error_find_color = 0;
#pragma omp parallel shared(image, p, i, n_colors, error_find_color) private(j, k)
        {
#pragma omp for schedule(dynamic)
            for (j = 0; j < image->height[i]; j++)
            {
                int pos;
                for (pos = j * image->width[i]; pos < (j + 1) * image->width[i]; pos++)
                {
                    int found_index = -1;
                    for (k = 0; k < n_colors; k++)
                    {
                        if (p[i][pos].r == image->g->SColorMap->Colors[k].Red &&
                            p[i][pos].g == image->g->SColorMap->Colors[k].Green &&
                            p[i][pos].b == image->g->SColorMap->Colors[k].Blue)
                        {
                            found_index = k;
                        }
                    }

                    if (found_index == -1)
                    {
#pragma omp atom
                        error_find_color = 1;
                    }

                    image->g->SavedImages[i].RasterBits[pos] = found_index;
                }
            }
        }
        if (error_find_color)
        {
            fprintf(stderr,
                    "Error: Unable to find a pixel in the color map\n");
            return 0;
        }
    }

#if DISPLAY_TIME_DETAILED
    gettimeofday(&t2, NULL);
    duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
    printf("  LOAD4: %lf s\n", duration);
    gettimeofday(&t1, NULL);
#endif

    /* Write the final image */
    if (!output_modified_read_gif(filename, image->g))
    {
        return 0;
    }

    return 1;
}

void apply_gray_filter(animated_gif *image)
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

#define CONV(l, c, nb_c) \
    (l) * (nb_c) + (c)

void apply_gray_line(animated_gif *image)
{
    int i, j, k;
    pixel **p;

    p = image->p;

    for (i = 0; i < image->n_images; i++)
    {
        for (j = 0; j < 10; j++)
        {
            for (k = image->width[i] / 2; k < image->width[i]; k++)
            {
                p[i][CONV(j, k, image->width[i])].r = 0;
                p[i][CONV(j, k, image->width[i])].g = 0;
                p[i][CONV(j, k, image->width[i])].b = 0;
            }
        }
    }
}

void apply_blur_filter(animated_gif *image, int size, int threshold)
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

        // printf( "Nb iter for image %d\n", n_iter ) ;

        free(new);
    }
}

void apply_sobel_filter(animated_gif *image)
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

animated_gif *load_image_seq(char *input_filename)
{
    animated_gif *image;

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

    printf("GIF loaded from file %s with %d image(s)\n",
           input_filename, image->n_images);
    printf("EXECUTION TIME:\n");
    printf("  LOADING:      %lf s\n", duration);
#endif

    return image;
}

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

void apply_filters_seq(animated_gif *image)
{
#if DISPLAY_TIME
    struct timeval t1, t2;
    double duration;
    gettimeofday(&t1, NULL);
#endif

    /* Convert the pixels into grayscale */
    apply_gray_filter(image);

#if DISPLAY_TIME
    gettimeofday(&t2, NULL);
    duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
    printf("  GRAY filter: %lf s\n", duration);
    gettimeofday(&t1, NULL);
#endif

    /* Apply blur filter with convergence value */
    apply_blur_filter(image, 5, 20);

#if DISPLAY_TIME
    gettimeofday(&t2, NULL);
    duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
    printf("  BLUR filter: %lf s\n", duration);
    gettimeofday(&t1, NULL);
#endif

    /* Apply sobel filter on pixels */
    apply_sobel_filter(image);

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

#if DISPLAY_TIME
    struct timeval t1, t2;
    double duration;
    gettimeofday(&t1, NULL);
#endif

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
                apply_gray_filter(work_gif);
                apply_blur_filter(work_gif, 5, 20);
                apply_sobel_filter(work_gif);

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
            apply_gray_filter(work_gif);
            apply_blur_filter(work_gif, 5, 20);
            apply_sobel_filter(work_gif);

            // ---- Sending back to root ----
            MPI_Isend(work_gif->p[0], im_size, MPI_BYTE, root, im, MPI_COMM_WORLD, send_req++);
        }

        // TODO
        //MPI_WaitAll
    }
}

int export_seq(char *output_filename, animated_gif *image)
{
#if DISPLAY_TIME
    struct timeval t1, t2;
    double duration;
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
    printf("  EXPORT: %lf s\n", duration);
#endif

    return 0;
}

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
#pragma omp parallel
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

    int mpi = 1;

    /* Code paralellized with mpi */
    if (mpi)
    {
        /* IMPORTING the original image */
        image = load_image_mpi(input_filename, root);
        if (image == NULL)
        {
            return 1;
        }

        /* FILTERING: apply filters */
        /* Decision to be taken depending on the size, nb of images... */
        apply_filters_mpi(image, output_filename, root);
    }

    /* Sequential */
    else
    {
        image = load_image_seq(input_filename);
        if (image == NULL)
        {
            return 1;
        }
        apply_filters_seq(image);
        export_seq(output_filename, image);
    }

    MPI_Finalize();
    return 0;
}
