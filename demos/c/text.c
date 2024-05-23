#include <demo.c>

#define TITLE "Text | Escape to quit"
#define SCR_W 704
#define SCR_H 400
#define SCALE 4

static struct {
	DemoData d;

	PIF_Font *font;
} demo;

static void setup(void) {
	demoSetup(&demo.d, TITLE, SCR_W, SCR_H, SCALE, 0.5, NULL);

	demo.font = PIF_fontNewDefault();
}

static void cleanup(void) {
	demoCleanup(&demo.d);

	PIF_fontFree(demo.font);
}

static void render(void) {
	PIF_imageClear(demo.d.canv, BLACK);

	PIF_imageSetShader(demo.d.canv, NULL, NULL);
	PIF_fontSetScale(demo.font, 3);

	const char *text = "Hello, world!";
	int w, h;
	PIF_fontTextSize(demo.font, text, &w, &h);
	int x = demo.d.canv->w / 2 - w / 2, y = demo.d.canv->h / 2 - h / 2;
	PIF_fontRenderText(demo.font, text, demo.d.canv, x, y, WHITE);

	PIF_imageSetShader(demo.d.canv, PIF_blendShader, demo.d.colormap);
	PIF_fontSetScale(demo.font, 1);

	PIF_fontRenderText(demo.font, "Very nice text rendering,\nnewlines supported too!",
	                   demo.d.canv, x, y + h + 4, WHITE);

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
