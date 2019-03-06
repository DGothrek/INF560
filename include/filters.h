/**
 * Declare the functions programmed by us
 **/

#include <datatype.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * GPU programming
 **/
void apply_gray_filter_gpu(animated_gif *image);
void apply_blur_filter_gpu(animated_gif *image, int size, int threshold);
void apply_sobel_filter_gpu(animated_gif *image);

#ifdef __cplusplus
}
#endif
