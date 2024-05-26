#include <math.h> /* sqrt */

#define TITLE "Rotate"
#define SCR_W 600
#define SCR_H 600
#define SCALE 1

#include "pif_sdl2.inc"

PIF_Image *img, *colormap;

void setup(void) {
	colormap = PIF_paletteCreateColormap(pal, PIF_DEFAULT_SHADES, 0.7);
	img      = generateRandomImage(300, 300, BLACK, colormap);
}

void cleanup(void) {
	PIF_imagesFree(img, colormap);
}

void render(double dt) {
	(void)dt;
	PIF_imageClear(canv, PIF_TRANSPARENT);

	PIF_Rect dest;
	dest.w = canv->w / (2 * M_SQRT2);
	dest.h = canv->h / (2 * M_SQRT2);
	dest.x = canv->w / 2 - dest.w / 2;
	dest.y = canv->h / 2 - dest.h / 2;

	float angle = elapsedTime / 300;
	PIF_imageRotateBlit(canv, &dest, img, NULL, angle, dest.w / 2, dest.h / 2);
}

void handleEvent(SDL_Event *evt) {
	(void)evt;
}
