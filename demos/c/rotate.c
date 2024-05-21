#include <time.h> /* time */
#include <math.h> /* sqrt */

#include "shared.inc"

#define TITLE "Rotate | Escape to quit"
#define SCR_W 600
#define SCR_H 600
#define SCALE 1

static struct {
	Demo d;

	PIF_Image *img;
} demo;

static void setup(void) {
	demoSetup(&demo.d, TITLE, SCR_W, SCR_H, SCALE, 0.7, NULL);

	srand(time(NULL));
	demo.img = generateRandomImage(300, 300, demo.d.colormap);
}

static void cleanup(void) {
	demoCleanup(&demo.d);

	PIF_imageFree(demo.img);
}

static void render(void) {
	PIF_imageClear(demo.d.canv, PIF_TRANSPARENT);

	float sqrt2 = sqrt(2);
	PIF_Rect dest;
	dest.w = demo.d.canv->w / (2 * sqrt2);
	dest.h = demo.d.canv->h / (2 * sqrt2);
	dest.x = demo.d.canv->w / 2 - dest.w / 2;
	dest.y = demo.d.canv->h / 2 - dest.h / 2;

	PIF_imageRotateBlit(demo.d.canv, &dest, demo.img, NULL, demo.d.elapsedTime / 300,
	                    dest.w / 2, dest.h / 2);

	demoDisplay(&demo.d);
}

static void input(void) {
	SDL_Event evt;
	while (SDL_PollEvent(&evt))
		demoEvent(&demo.d, &evt);
}

int main(void) {
	setup();
	while (demo.d.running) {
		demoFrame(&demo.d);

		render();
		input();
	}
	cleanup();
	return 0;
}
