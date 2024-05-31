#include <math.h> /* cos, sin */

#define TITLE "Blur"
#define SCR_W 704
#define SCR_H 400
#define SCALE 3

#include "pif_sdl2.inc"

typedef struct {
	PIF_Palette *pal;
	int          size;
} Blur;

PIF_Image *colormap, *rgbmap;
PIF_Font  *font;
unsigned   seed;
bool       paused;
Blur       blur;
double     animTime;

void setup(void) {
	colormap  = PIF_paletteCreateColormap(pal, PIF_DEFAULT_SHADES, 0.65);
	rgbmap    = PIF_paletteCreateRgbmap(pal, 32);
	font      = PIF_fontNewDefault();
	seed      = (unsigned)time(NULL);
	blur.pal  = pal;
	blur.size = 2;
}

void cleanup(void) {
	PIF_imagesFree(colormap, rgbmap);
	PIF_fontFree(font);
}

void blurShader(int x, int y, uint8_t *pixel, uint8_t color, PIF_Image *img) {
	(void)color;
	Blur *blur = (Blur*)img->data;
	int   r = 0, g = 0, b = 0;

	/* Box blur */
	for (int yo = -blur->size; yo <= blur->size; ++ yo) {
		for (int xo = -blur->size; xo <= blur->size; ++ xo) {
			uint8_t *src = PIF_imageAt(img, x + xo, y + yo);
			if (src == NULL)
				continue;

			PIF_Rgb rgb = blur->pal->map[*src];
			r += rgb.r;
			g += rgb.g;
			b += rgb.b;
		}
	}

	/* Finding the closest color for every pixel like this is very inefficient, because it has to
	   loop through the entire palette and find the closest color.

		*pixel = PIF_paletteClosest(blur->pal, (PIF_Rgb){
			.r = r / pow(blur->size * 2 + 1, 2),
			.g = g / pow(blur->size * 2 + 1, 2),
			.b = b / pow(blur->size * 2 + 1, 2),
		});
	*/

	/* A faster, but memory-heavy way to convert RGB to a color, using an rgbmap. */
	*pixel = PIF_rgbToColor((PIF_Rgb){
		.r = r / pow(blur->size * 2 + 1, 2),
		.g = g / pow(blur->size * 2 + 1, 2),
		.b = b / pow(blur->size * 2 + 1, 2),
	}, rgbmap);
}

void renderBackground(double dt) {
	srand(seed);
	float max = 0.8;
	float t   = (1.0 + cos(animTime / 4000)) / 2 * max + 1.0 - max;

	int size = 9;
	for (int y = 0; y < canv->h; ++ y) {
		for (int x = 0; x < canv->w; ++ x)
			PIF_imageFillCircle(canv, x * size, y * size, size / 2, (rand() % 255 * t) + 1);
	}

	if (!paused)
		animTime += dt;
}

void renderText(void) {
	const char *text = "Move your mouse around\nand scroll the mouse wheel";
	int w, h;
	PIF_fontSetScale(font, 2);
	PIF_fontTextSize(font, text, &w, &h);

	/* The last row of the font character pixels is only used by commas and similar symbols that
	   reach under the text "line", this means normal letters are basically offset upward by 1
	   character pixel. We fix this offset by subtracting that last row from the height. */
	h -= font->scale;

	int x = canv->w / 2 - w / 2, y = canv->h / 2 - h / 2;

	int padding = 8;
	PIF_Rect rect = {
		.x = -1,
		.y = y - padding,
		.w = canv->w + 2,
		.h = h + padding * 2,
	};
	/* Background darkening  */
	PIF_imageSetShader(canv, PIF_blendShader, colormap);
	PIF_imageFillRect(canv, &rect, BLACK);

	/* Outline */
	PIF_imageSetShader(canv, NULL, NULL);
	PIF_imageDrawRect(canv, &rect, 0, WHITE);

	/* Text */
	PIF_fontRenderText(font, text, canv, x, y, WHITE);
}

void renderBlurCircle(void) {
	/* Blur */
	PIF_imageSetShader(canv, blurShader, &blur);
	PIF_imageFillCircle(canv, mouseX, mouseY, 30, 1 /* Color is unused */);

	/* Outline */
	PIF_imageSetShader(canv, PIF_blendShader, colormap);
	PIF_imageDrawCircle(canv, mouseX, mouseY, 30, 0, WHITE);

	PIF_imageSetShader(canv, NULL, NULL);
}

void renderPauseIcon(void) {
	int offset = 5, size = 12;

	/* Background darkening */
	PIF_imageSetShader(canv, PIF_blendShader, colormap);
	PIF_imageFillCircle(canv, 0, 0, offset + size * 2, BLACK);

	/* Outline */
	PIF_imageSetShader(canv, NULL, NULL);
	PIF_imageDrawCircle(canv, 0, 0, offset + size * 2, 0, WHITE);

	/* Icon */
	if (paused) {
		PIF_Rect rect = {
			.x = offset,
			.y = offset,
			.w = size / 3,
			.h = size,
		};
		PIF_imageFillRect(canv, &rect, WHITE);

		rect.x += size / 3 * 2;
		PIF_imageFillRect(canv, &rect, WHITE);
	} else
		PIF_imageFillTriangle(canv, offset, offset, offset, offset + size,
		                      offset + size, offset + size / 2, WHITE);

	/* Handle button click */
	if (mouseClicked && mouseX <= offset + size && mouseY <= offset + size)
		paused = !paused;
}

void render(double dt) {
	PIF_imageClear(canv, BLACK);

	renderBackground(dt);
	renderText();
	renderBlurCircle();
	renderPauseIcon();
}

void handleEvent(SDL_Event *evt) {
	switch (evt->type) {
	case SDL_MOUSEWHEEL:
		blur.size += evt->wheel.y;
		if (blur.size < 0)  blur.size = 0;
		if (blur.size > 20) blur.size = 20;
		break;

	case SDL_KEYDOWN:
		if (evt->key.keysym.sym == SDLK_SPACE)
			paused = !paused;
		break;
	}
}
