#include <time.h> /* time */
#include <math.h> /* sin */

#include "shared.inc"

#define TITLE "Blur | Escape to quit, space to pause"
#define SCR_W 704
#define SCR_H 400
#define SCALE 3

typedef struct {
	PIF_Palette *pal;
	int          size;
} Blur;

static struct {
	Demo d;

	PIF_Font *font;
	unsigned  seed;
	bool      paused, clicked;
	Blur      blur;
	float     animTime;
} demo;

static void setup(void) {
	demoSetup(&demo.d, TITLE, SCR_W, SCR_H, SCALE, 0.65, NULL);

	demo.font      = PIF_fontNewDefault();
	demo.seed      = (unsigned)time(NULL);
	demo.blur.pal  = demo.d.pal;
	demo.blur.size = 2;
}

static void cleanup(void) {
	demoCleanup(&demo.d);

	PIF_fontFree(demo.font);
}

void blurShader(int x, int y, uint8_t *pixel, uint8_t color, PIF_Image *img) {
	(void)color;
	Blur *blur = (Blur*)img->data;

	int r = 0, g = 0, b = 0;

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

	/* Finding the closest color for every pixel like this is very inefficient, but it serves only
	   as a demo for the PIF shaders */
	*pixel = PIF_paletteClosest(blur->pal, (PIF_Rgb){
		.r = r / pow(blur->size * 2 + 1, 2),
		.g = g / pow(blur->size * 2 + 1, 2),
		.b = b / pow(blur->size * 2 + 1, 2),
	});
}

static void renderBackground(void) {
	srand(demo.seed);
	float max = 0.8;
	float t   = (1.0 + cos(demo.animTime / 4000)) / 2 * max + 1.0 - max;

	int size = 9;
	for (int y = 0; y < demo.d.canv->h; ++ y) {
		for (int x = 0; x < demo.d.canv->w; ++ x)
			PIF_imageFillCircle(demo.d.canv, x * size, y * size, size / 2, (rand() % 255 * t) + 1);
	}

	if (!demo.paused)
		demo.animTime += demo.d.dt;
}

static void renderText(void) {
	const char *text = "Move your mouse around\nand scroll the mouse wheel";
	int w, h;
	PIF_fontSetScale(demo.font, 2);
	PIF_fontTextSize(demo.font, text, &w, &h);

	/* The last row of the font character pixels is only used by commas and similar symbols that
	   reach under the text "line", this means normal letters are basically offset upward by 1
	   character pixel. We fix this offset by subtracting that last row from the height. */
	h -= demo.font->scale;

	int x = demo.d.canv->w / 2 - w / 2, y = demo.d.canv->h / 2 - h / 2;

	int padding = 8;
	PIF_Rect rect = {
		.x = -1,
		.y = y - padding,
		.w = demo.d.canv->w + 2,
		.h = h + padding * 2,
	};
	/* Background darkening  */
	PIF_imageSetShader(demo.d.canv, PIF_blendShader, demo.d.colormap);
	PIF_imageFillRect(demo.d.canv, &rect, BLACK);

	/* Outline */
	PIF_imageSetShader(demo.d.canv, NULL, NULL);
	PIF_imageDrawRect(demo.d.canv, &rect, 0, WHITE);

	/* Text */
	PIF_fontRenderText(demo.font, text, demo.d.canv, x, y, WHITE);
}

static void renderBlurCircle(void) {
	/* Blur */
	PIF_imageSetShader(demo.d.canv, blurShader, &demo.blur);
	PIF_imageFillCircle(demo.d.canv, demo.d.mouseX, demo.d.mouseY, 30, 1 /* Color is unused */);

	/* Outline */
	PIF_imageSetShader(demo.d.canv, PIF_blendShader, demo.d.colormap);
	PIF_imageDrawCircle(demo.d.canv, demo.d.mouseX, demo.d.mouseY, 30, 0, WHITE);

	PIF_imageSetShader(demo.d.canv, NULL, NULL);
}

static void renderPauseIcon(void) {
	int offset = 5, size = 12;

	/* Background darkening */
	PIF_imageSetShader(demo.d.canv, PIF_blendShader, demo.d.colormap);
	PIF_imageFillCircle(demo.d.canv, 0, 0, offset + size * 2, BLACK);

	/* Outline */
	PIF_imageSetShader(demo.d.canv, NULL, NULL);
	PIF_imageDrawCircle(demo.d.canv, 0, 0, offset + size * 2, 0, WHITE);

	/* Icon */
	if (demo.paused) {
		PIF_Rect rect = {
			.x = offset,
			.y = offset,
			.w = size / 3,
			.h = size,
		};
		PIF_imageFillRect(demo.d.canv, &rect, WHITE);

		rect.x += size / 3 * 2;
		PIF_imageFillRect(demo.d.canv, &rect, WHITE);
	} else
		PIF_imageFillTriangle(demo.d.canv, offset, offset, offset, offset + size,
		                      offset + size, offset + size / 2, WHITE);

	/* Handle button click */
	if (demo.clicked && demo.d.mouseX <= offset + size && demo.d.mouseY <= offset + size)
		demo.paused = !demo.paused;
}

static void render(void) {
	PIF_imageClear(demo.d.canv, BLACK);

	renderBackground();
	renderText();
	renderBlurCircle();
	renderPauseIcon();

	demoDisplay(&demo.d);
}

static void input(void) {
	static bool mouseDown = false, mouseWasDown = false;

	SDL_Event evt;
	while (SDL_PollEvent(&evt)) {
		demoEvent(&demo.d, &evt);

		switch (evt.type) {
		case SDL_MOUSEWHEEL:
			demo.blur.size += evt.wheel.y;
			if (demo.blur.size < 0)  demo.blur.size = 0;
			if (demo.blur.size > 20) demo.blur.size = 20;
			break;

		case SDL_KEYDOWN:
			if (evt.key.keysym.sym == SDLK_SPACE)
				demo.paused = !demo.paused;
			break;

		case SDL_MOUSEBUTTONDOWN: mouseDown = true;  break;
		case SDL_MOUSEBUTTONUP:   mouseDown = false; break;
		}
	}

	demo.clicked = mouseWasDown && !mouseDown;
	mouseWasDown = mouseDown;
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
