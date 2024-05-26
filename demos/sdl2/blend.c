#define TITLE "Blend"
#define SCR_W 400
#define SCR_H 400
#define SCALE 1

#include "pif_sdl2.inc"

#define SPEED   3
#define HUE_MAX 32
#define RADIUS  60

typedef struct {
	int x, y, vx, vy;
} Object;

PIF_Image *img, *colormap, *huemap;
uint8_t    color;
Object     a, b;

float hueToRgb(float v1, float v2, float vH) {
	if (vH < 0) vH += 1;
	if (vH > 1) vH -= 1;

	if (vH * 6 < 1) return v1 + (v2 - v1) * 6 * vH;
	if (vH * 2 < 1) return v2;
	if (vH * 3 < 2) return v1 + (v2 - v1) * (2.0 / 3 - vH) * 6;
	return v1;
}

PIF_Rgb hslToRgb(float h, float s, float l) {
	PIF_Rgb rgb;
	if (s == 0)
		rgb.r = rgb.g = rgb.b = l * 255;
	else {
		float v2 = l < 0.5? l * (1 + s) : l + s - l * s;
		float v1 = 2 * l - v2;

		rgb.r = 255 * hueToRgb(v1, v2, h + 1.0 / 3);
		rgb.g = 255 * hueToRgb(v1, v2, h);
		rgb.b = 255 * hueToRgb(v1, v2, h - 1.0 / 3);
	}
	return rgb;
}

void setup(void) {
	/* Modify palette to add hue rainbow spectrum */
	for (int i = 0; i < HUE_MAX / 2; ++ i) {
		PIF_Rgb rgb = hslToRgb((float)i / (HUE_MAX / 2 - 1), 1, 0.5);

		paletteData[(i + 2) * 3 + 0] = rgb.r;
		paletteData[(i + 2) * 3 + 1] = rgb.g;
		paletteData[(i + 2) * 3 + 2] = rgb.b;
	}

	colormap = PIF_paletteCreateColormap(pal, PIF_DEFAULT_SHADES, 0.7);
	img      = generateRandomImage(300, 300, BLACK, colormap);

	a.x  = canv->w / 2;
	a.y  = canv->h / 2 + RADIUS;
	a.vx = -SPEED / 2;
	a.vy = -SPEED;

	b.x  = canv->w / 2;
	b.y  = canv->h / 2 - RADIUS;
	b.vx = SPEED / 2;
	b.vy = SPEED;

	/* Generate hue map */
	huemap = PIF_imageNew(HUE_MAX, 1);
	for (int i = 0; i < HUE_MAX; ++ i) {
		PIF_Rgb rgb = hslToRgb((float)i / (HUE_MAX - 1), 1, 0.5);
		huemap->buf[i] = PIF_paletteClosest(pal, rgb);
	}
}

void cleanup(void) {
	PIF_imagesFree(img, colormap, huemap);
}

void step(Object *obj) {
	int prevX = obj->x, prevY = obj->y;

	obj->x += obj->vx;
	if (obj->x + RADIUS > canv->w || obj->x - RADIUS < 0) {
		obj->x   = prevX;
		obj->vx *= -1;
	}

	obj->y += obj->vy;
	if (obj->y + RADIUS > canv->h || obj->y - RADIUS < 0) {
		obj->y   = prevY;
		obj->vy *= -1;
	}
}

void render(double dt) {
	(void)dt;
	PIF_imageClear(canv, BLACK);

	PIF_Rect dest;
	dest.x = canv->w / 2 - img->w / 2;
	dest.y = canv->h / 2 - img->h / 2;
	dest.w = img->w;
	dest.h = img->h;
	PIF_imageBlit(canv, &dest, img, NULL);

	uint8_t color = huemap->buf[(int)(elapsedTime / 200) % huemap->w];

	PIF_imageSetShader(canv, PIF_ditherShader, NULL);
	PIF_imageFillCircle(canv, a.x, a.y, RADIUS, color);

	PIF_imageSetShader(canv, PIF_blendShader, colormap);
	PIF_imageFillCircle(canv, b.x, b.y, RADIUS, color);

	PIF_imageSetShader(canv, NULL, NULL);

	step(&a);
	step(&b);
}

void handleEvent(SDL_Event *evt) {
	(void)evt;
}
