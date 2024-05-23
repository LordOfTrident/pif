#include <time.h> /* time */
#include <math.h> /* sin, cos */

#include <demo.c>

#define TITLE "Blit | Escape to quit"
#define SCR_W 400
#define SCR_H 400
#define SCALE 1

static struct {
	DemoData d;

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

	int w = demo.img->w + (cos(demo.d.elapsedTime / 300) * 1.5 - 0.5) * demo.img->w / 4;
	int h = demo.img->h + (sin(demo.d.elapsedTime / 300) * 1.5 - 0.5) * demo.img->h / 4;

	PIF_Rect dest = {
		.x = demo.d.canv->w / 2 - w / 2,
		.y = demo.d.canv->h / 2 - h / 2,
		.w = w,
		.h = h,
	};
	PIF_imageBlit(demo.d.canv, &dest, demo.img, NULL);

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
