#include <math.h> /* cos, sin */

#define TITLE "Blit"
#define SCR_W 400
#define SCR_H 400
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

	float angle = elapsedTime / 300;

	PIF_Rect dest;
	dest.w = img->w + (cos(angle) * 1.5 - 0.5) * img->w / 4;
	dest.h = img->h + (sin(angle) * 1.5 - 0.5) * img->h / 4;
	dest.x = canv->w / 2 - dest.w / 2;
	dest.y = canv->h / 2 - dest.h / 2;
	PIF_imageBlit(canv, &dest, img, NULL);
}

void handleEvent(SDL_Event *evt) {
	(void)evt;
}
