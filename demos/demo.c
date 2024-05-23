#include <stdlib.h>  /* exit, EXIT_FAILURE, malloc, free, rand */
#include <stdio.h>   /* fprintf, stderr */
#include <assert.h>  /* assert */
#include <stdint.h>  /* uint8_t */
#include <stdbool.h> /* bool, true, false */
#include <string.h>  /* memset */
#include <stdint.h>  /* uint32_t, uint8_t */

#include <SDL2/SDL.h>

#include <pif.h>
#include <pif.c>

/* paletteData, BLACK, WHITE */
#include "palette.c"

#define arraySize(ARR) (sizeof(ARR) / sizeof(*(ARR)))
#define die(...)                   \
	(fprintf(stderr, __VA_ARGS__), \
	 fputc('\n', stderr),          \
	 exit(EXIT_FAILURE))

typedef struct {
	SDL_Window    *win;
	SDL_Renderer  *ren;
	SDL_Texture   *scr;
	uint32_t      *pixels;
	SDL_Rect       viewport;
	const uint8_t *keyboard;
	PIF_Palette   *pal;
	PIF_Image     *canv, *colormap;

	float aspectRatio;
	int   winW, winH, mouseX, mouseY;

	double dt, elapsedTime;

	bool running;
} DemoData;

void demoSetup(DemoData *self, const char *title, int scrW, int scrH,
               float scale, float shading, const char *palPath);
void demoCleanup(DemoData *self);
void demoFrame  (DemoData *self);
void demoDisplay(DemoData *self);
void demoEvent  (DemoData *self, SDL_Event *evt);

PIF_Image *generateRandomImage(int w, int h, PIF_Image *colormap);

void demoSetup(DemoData *self, const char *title, int scrW, int scrH,
               float scale, float shading, const char *palPath) {
	memset(self, 0, sizeof(*self));

	if (palPath == NULL) {
		self->pal = PIF_paletteNew(sizeof(paletteData) / 3);
		for (int i = 0; i < self->pal->size; ++ i) {
			self->pal->map[i].r = paletteData[i * 3];
			self->pal->map[i].g = paletteData[i * 3 + 1];
			self->pal->map[i].b = paletteData[i * 3 + 2];
		}
	} else {
		const char *err;
		self->pal = PIF_paletteLoad(palPath, &err);
		if (self->pal == NULL)
			die("Error: %s", err);
	}

	self->colormap    = PIF_paletteCreateColormap(self->pal, 64, shading);
	self->canv        = PIF_imageNew(scrW / scale, scrH / scale);
	self->pixels      = (uint32_t*)malloc(self->canv->size * sizeof(uint32_t));
	self->winW        = scrW;
	self->winH        = scrH;
	self->aspectRatio = ((float)scrH / scrW);
	self->running     = true;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		die("Failed to initialize SDL2: %s", SDL_GetError());

	self->win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                             scrW, scrH, SDL_WINDOW_SHOWN);
	if (self->win == NULL)
		die("Failed to create window: %s", SDL_GetError());

	self->ren = SDL_CreateRenderer(self->win, -1,
	                               SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (self->ren == NULL)
		die("Failed to create renderer: %s", SDL_GetError());

	self->scr = SDL_CreateTexture(self->ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
	                              scrW / scale, scrH / scale);
	if (self->scr == NULL)
		die("Failed to create screen texture: %s", SDL_GetError());

	self->keyboard = SDL_GetKeyboardState(NULL);
	SDL_SetTextureBlendMode(self->scr, SDL_BLENDMODE_BLEND);

	/* A hack to force i3wm to float the window on startup */
	SDL_SetWindowResizable(self->win, SDL_TRUE);
}

void demoCleanup(DemoData *self) {
	SDL_DestroyTexture(self->scr);
	SDL_DestroyRenderer(self->ren);
	SDL_DestroyWindow(self->win);

	SDL_Quit();

	free(self->pixels);
	PIF_imageFree(self->canv);
	PIF_imageFree(self->colormap);
	PIF_paletteFree(self->pal);
}

void demoFrame(DemoData *self) {
	static uint64_t last = 0, now = 0;
	last = last == 0 && now == 0? SDL_GetPerformanceCounter() : now;
	now  = SDL_GetPerformanceCounter();

	self->dt = (double)(now - last) * 1000 / (double)SDL_GetPerformanceFrequency();
	self->elapsedTime += self->dt;
}

void demoDisplay(DemoData *self) {
	SDL_SetRenderDrawColor(self->ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(self->ren);

	/* Checkerboard background */
	int size = 15;
	for (int y = 0; y < ceil((float)self->winH / size); ++ y) {
		for (int x = 0; x < ceil((float)self->winW / size); ++ x) {
			SDL_Rect rect;
			rect.x = x * size;
			rect.y = y * size;
			rect.w = size;
			rect.h = size;
			int shade = y % 2 == (x % 2 == 0)? 50 : 70;
			SDL_SetRenderDrawColor(self->ren, shade, shade, shade, SDL_ALPHA_OPAQUE);
			SDL_RenderFillRect(self->ren, &rect);
		}
	}

	/* Update screen texture */
	for (int i = 0; i < self->canv->size; ++ i) {
		uint8_t color = self->canv->buf[i];
		/* Make the texture pixel transparent if the canvas pixel is transparent */
		self->pixels[i] = color == PIF_TRANSPARENT? 0 : PIF_rgbToPixelRgba32(self->pal->map[color]);
	}

	SDL_UpdateTexture(self->scr, NULL, self->pixels, self->canv->w * sizeof(*self->pixels));

	/* Fit the viewport in the center of the window */
	int   w = self->winW;
	int   h = self->winH;
	float winAspectRatio = (float)self->winH / self->winW;

	if      (winAspectRatio > self->aspectRatio) h = w * self->aspectRatio;
	else if (winAspectRatio < self->aspectRatio) w = h / self->aspectRatio;

	self->viewport.x = self->winW / 2 - w / 2;
	self->viewport.y = self->winH / 2 - h / 2;
	self->viewport.w = w;
	self->viewport.h = h;

	SDL_RenderCopy(self->ren, self->scr, NULL, &self->viewport);
	SDL_RenderPresent(self->ren);
}

void demoEvent(DemoData *self, SDL_Event *evt) {
	switch (evt->type) {
	case SDL_QUIT: self->running = false; break;

	case SDL_MOUSEMOTION:
		/* Convert window position to canvas position */
		self->mouseX = (float)(evt->motion.x - self->viewport.x) / self->viewport.w * self->canv->w;
		self->mouseY = (float)(evt->motion.y - self->viewport.y) / self->viewport.h * self->canv->h;
		break;

	case SDL_WINDOWEVENT:
		if (evt->window.event == SDL_WINDOWEVENT_RESIZED) {
			self->winW = evt->window.data1;
			self->winH = evt->window.data2;
		}
		break;

	case SDL_KEYDOWN:
		if (evt->key.keysym.sym == SDLK_ESCAPE)
			self->running = false;
		break;
	}
}

PIF_Image *generateRandomImage(int w, int h, PIF_Image *colormap) {
	PIF_Image *img = PIF_imageNew(w, h);
	PIF_imageClear(img, BLACK);
	for (int i = 0; i < 100; ++ i) {
		int x = rand() % img->w;
		int y = rand() % img->h;
		int r = rand() % 40 + 10;

		PIF_imageSetShader(img, PIF_blendShader, colormap);
		PIF_imageFillCircle(img, x, y, r, rand() % 255 + 1);
	}
	return img;
}
