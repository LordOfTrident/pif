#define TITLE "Text"
#define SCR_W 704
#define SCR_H 400
#define SCALE 4

#include "pif_sdl2.inc"

PIF_Image *colormap;
PIF_Font  *font;

void setup(void) {
	colormap = PIF_paletteCreateColormap(pal, PIF_DEFAULT_SHADES, 0.5);
	font     = PIF_fontNewDefault();
}

void cleanup(void) {
	PIF_imageFree(colormap);
	PIF_fontFree(font);
}

void render(double dt) {
	(void)dt;
	PIF_imageClear(canv, BLACK);

	const char *text = "Hello, world!";
	int w, h;
	PIF_fontSetScale(font, 3);
	PIF_fontTextSize(font, text, &w, &h);

	int x = canv->w / 2 - w / 2, y = canv->h / 2 - h / 2;
	PIF_fontRenderText(font, text, canv, x, y, WHITE);
	PIF_fontSetScale(font, 1);

	PIF_imageSetShader(canv, PIF_blendShader, colormap);
	PIF_fontRenderText(font, "Very nice text rendering,\nnewlines supported too!",
	                   canv, x, y + h + 4, WHITE);
	PIF_imageSetShader(canv, NULL, NULL);
}

void handleEvent(SDL_Event *evt) {
	(void)evt;
}
