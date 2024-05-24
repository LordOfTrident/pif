#include <time.h> /* time */
#include <math.h> /* sin, cos, round */

#include <demo.c>

#define TITLE "Blend | Escape to quit"
#define SCR_W 400
#define SCR_H 400
#define SCALE 1
#define SPEED 2

static struct {
	DemoData d;

	PIF_Image *img;

	uint8_t color;
	int     radius;
	int     x1, y1, vx1, vy1;
	int     x2, y2, vx2, vy2;

	PIF_Image *hueMap;
} demo;

static float hueToRgb(float v1, float v2, float vH) {
	if (vH < 0) vH += 1;
	if (vH > 1) vH -= 1;

	if (vH * 6 < 1) return v1 + (v2 - v1) * 6 * vH;
	if (vH * 2 < 1) return v2;
	if (vH * 3 < 2) return v1 + (v2 - v1) * (2.0 / 3 - vH) * 6;

	return v1;
}

static PIF_Rgb hslToRgb(float h, float s, float l) {
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

static void setup(void) {
	int hueMax = 32;

	/* Modify palette to add HSL rainbow spectrum */
	for (int i = 0; i < hueMax / 2; ++ i) {
		PIF_Rgb rgb = hslToRgb((float)i / (hueMax / 2 - 1), 1, 0.5);

		paletteData[(i + 2) * 3 + 0] = rgb.r;
		paletteData[(i + 2) * 3 + 1] = rgb.g;
		paletteData[(i + 2) * 3 + 2] = rgb.b;
	}

	demoSetup(&demo.d, TITLE, SCR_W, SCR_H, SCALE, 0.5, NULL);

	srand(time(NULL));
	demo.img = generateRandomImage(300, 300, demo.d.colormap);

	demo.color  = PIF_paletteClosest(demo.d.pal, (PIF_Rgb){255, 255, 255});
	demo.radius = 60;

	demo.x1  = demo.d.canv->w / 2;
	demo.y1  = demo.d.canv->h / 2 + demo.radius;
	demo.vx1 = -SPEED;
	demo.vy1 = -SPEED;

	demo.x2  = demo.d.canv->w / 2;
	demo.y2  = demo.d.canv->h / 2 - demo.radius;
	demo.vx2 = SPEED;
	demo.vy2 = SPEED;

	/* Generate rainbow spectrum hue map */
	demo.hueMap = PIF_imageNew(hueMax, 1);
	for (int i = 0; i < hueMax; ++ i) {
		PIF_Rgb rgb = hslToRgb((float)i / (hueMax - 1), 1, 0.5);
		demo.hueMap->buf[i] = PIF_paletteClosest(demo.d.pal, rgb);
	}
}

static void cleanup(void) {
	demoCleanup(&demo.d);

	PIF_imageFree(demo.img);
	PIF_imageFree(demo.hueMap);
}

static void render(void) {
	PIF_imageClear(demo.d.canv, BLACK);

	PIF_Rect dest = {
		.x = demo.d.canv->w / 2 - demo.img->w / 2,
		.y = demo.d.canv->h / 2 - demo.img->h / 2,
		.w = demo.img->w,
		.h = demo.img->h,
	};
	PIF_imageBlit(demo.d.canv, &dest, demo.img, NULL);

	uint8_t color = demo.hueMap->buf[(int)(demo.d.elapsedTime / 200) % demo.hueMap->w];

	PIF_imageSetShader(demo.d.canv, PIF_ditherShader, NULL);
	PIF_imageFillCircle(demo.d.canv, demo.x1, demo.y1, demo.radius, color);

	PIF_imageSetShader(demo.d.canv, PIF_blendShader, demo.d.colormap);
	PIF_imageFillCircle(demo.d.canv, demo.x2, demo.y2, demo.radius, color);

	PIF_imageSetShader(demo.d.canv, NULL, NULL);

	demoDisplay(&demo.d);
}

static void input(void) {
	SDL_Event evt;
	while (SDL_PollEvent(&evt))
		demoEvent(&demo.d, &evt);
}

static void step(int *x, int *y, int *vx, int *vy) {
	int prevX = *x, prevY = *y;

	*x += *vx;
	*y += *vy;

	if (*x + demo.radius > demo.d.canv->w || *x - demo.radius < 0) {
		*x   = prevX;
		*vx *= -1;
	}

	if (*y + demo.radius > demo.d.canv->h || *y - demo.radius < 0) {
		*y   = prevY;
		*vy *= -1;
	}
}

static void update(void) {
	step(&demo.x1, &demo.y1, &demo.vx1, &demo.vy1);
	step(&demo.x2, &demo.y2, &demo.vx2, &demo.vy2);
}

int main(void) {
	setup();
	while (demo.d.running) {
		demoFrame(&demo.d);

		render();
		input();
		update();
	}
	cleanup();
	return 0;
}
