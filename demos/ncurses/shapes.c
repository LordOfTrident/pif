#include <math.h> /* sqrt */

#include "pif_ncurses.inc"

PIF_Image *colormap;

void setup(void) {
	colormap = PIF_paletteCreateColormap(pal, pal->size / 2, 0.5);
}

void cleanup(void) {
	PIF_imageFree(colormap);
}

void render(double dt) {
	(void)dt;
	PIF_imageClear(canv, BLACK);

	float angle = elapsedTime / 300;
	PIF_Rect rect;
	rect.w = PIF_min(canv->w, canv->h) / 2;
	rect.h = rect.w;
	rect.x = canv->w / 2 - rect.w / 2;
	rect.y = canv->h / 2 - rect.h / 2;
	PIF_imageFillRotateRect(canv, &rect, WHITE, angle, 0, 0);

	PIF_imageSetShader(canv, PIF_blendShader, colormap);
	PIF_imageFillTriangle(canv, canv->w / 2, 3, 3, canv->h - 4, canv->w - 4, canv->h - 4, WHITE);

	int radius = PIF_min(canv->w, canv->h) / 4;
	int x = (1.0 + cos(elapsedTime / 900)) / 2 * canv->w;
	int y = (1.0 + sin(elapsedTime / 200)) / 2 * canv->h;
	PIF_imageFillCircle(canv, x, y, radius, WHITE);
	PIF_imageSetShader(canv, NULL, NULL);
}

void handleKey(int key) {
	(void)key;
}
