#include "shared.inc"

#define TITLE "Palette | Escape to quit"
#define SCR_W 480
#define SCR_H 480
#define SCALE 30

static struct {
	Demo d;

	PIF_Font *font;
} demo;

static void setup(const char *palPath) {
	demoSetup(&demo.d, TITLE, SCR_W, SCR_H, SCALE, 0.5, palPath);

	demo.font = PIF_fontNewDefault();
}

static void cleanup(void) {
	demoCleanup(&demo.d);

	PIF_fontFree(demo.font);
}

static void render(void) {
	PIF_imageClear(demo.d.canv, PIF_TRANSPARENT);

	int x = 0, y = 0;
	for (int i = 0; i < demo.d.pal->size; ++ i) {
		PIF_imageDrawPoint(demo.d.canv, x, y, i);

		if (x > 0 && x % 15 == 0) {
			x = 0;
			++ y;
		} else
			++ x;
	}

	demoDisplay(&demo.d);
}

static void input(void) {
	SDL_Event evt;
	while (SDL_PollEvent(&evt))
		demoEvent(&demo.d, &evt);
}

int main(int argc, const char **argv) {
	if (argc < 2)
		die("Usage: %s <PALETTE_PATH>", argv[0]);

	setup(argv[1]);
	while (demo.d.running) {
		demoFrame(&demo.d);

		render();
		input();
	}
	cleanup();
	return 0;
}
