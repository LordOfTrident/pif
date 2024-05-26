#include <stdio.h>  /* fprintf, stderr, fputc */
#include <stdlib.h> /* exit, EXIT_FAILURE */
#include <math.h>   /* M_PI, M_SQRT2 */

#include <pif.h>
#include <pif.c>

#ifndef M_PI
#	define M_PI 3.1415926535
#endif

#ifndef M_SQRT2
#	define M_SQRT2 1.41421356237
#endif

#define arraySize(ARR) (int)(sizeof(ARR) / sizeof(*(ARR)))
#define die(...)                   \
	(fprintf(stderr, __VA_ARGS__), \
	 fputc('\n', stderr),          \
	 exit(EXIT_FAILURE))

PIF_Image *generateRandomImage(int w, int h, uint8_t bg, PIF_Image *colormap) {
	PIF_Image *img = PIF_imageNew(w, h);
	PIF_imageClear(img, bg);
	for (int i = 0; i < 100; ++ i) {
		int x = rand() % img->w;
		int y = rand() % img->h;
		int r = rand() % 40 + 10;

		PIF_imageSetShader(img, PIF_blendShader, colormap);
		PIF_imageFillCircle(img, x, y, r, rand() % 255 + 1);
	}
	return img;
}
