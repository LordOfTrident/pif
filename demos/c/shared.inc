#include <stdlib.h>  /* exit, EXIT_FAILURE, malloc, free, rand */
#include <stdio.h>   /* fprintf, stderr */
#include <assert.h>  /* assert */
#include <stdint.h>  /* uint8_t */
#include <stdbool.h> /* bool, true, false */
#include <stdint.h>  /* uint32_t, uint8_t */

#include <SDL2/SDL.h>

#include <pif.h>
#include <pif.c>

/* paletteData */
#include "../palette.inc"

#define BLACK 1
#define WHITE 255

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
} Demo;

void demoSetup(Demo *this, const char *title, int scrW, int scrH,
               float scale, float shading, const char *palPath);
void demoCleanup(Demo *this);
void demoFrame  (Demo *this);
void demoDisplay(Demo *this);
void demoEvent  (Demo *this, SDL_Event *evt);

PIF_Image *generateRandomImage(int w, int h, PIF_Image *colormap);

void demoSetup(Demo *this, const char *title, int scrW, int scrH,
               float scale, float shading, const char *palPath) {
	if (palPath == NULL) {
		this->pal = PIF_paletteNew(sizeof(paletteData) / 3);
		for (int i = 0; i < this->pal->size; ++ i) {
			this->pal->map[i].r = paletteData[i * 3];
			this->pal->map[i].g = paletteData[i * 3 + 1];
			this->pal->map[i].b = paletteData[i * 3 + 2];
		}
	} else {
		const char *err;
		this->pal = PIF_paletteLoad(palPath, &err);
		if (this->pal == NULL)
			die("Error: %s", err);
	}

	this->colormap    = PIF_paletteCreateColormap(this->pal, 64, shading);
	this->canv        = PIF_imageNew(scrW / scale, scrH / scale);
	this->pixels      = (uint32_t*)malloc(this->canv->size * sizeof(uint32_t));
	this->winW        = scrW;
	this->winH        = scrH;
	this->aspectRatio = ((float)scrH / scrW);
	this->running     = true;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		die("Failed to initialize SDL2: %s", SDL_GetError());

	this->win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                             scrW, scrH, SDL_WINDOW_SHOWN);
	if (this->win == NULL)
		die("Failed to create window: %s", SDL_GetError());

	this->ren = SDL_CreateRenderer(this->win, -1,
	                               SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (this->ren == NULL)
		die("Failed to create renderer: %s", SDL_GetError());

	this->scr = SDL_CreateTexture(this->ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
	                              scrW / scale, scrH / scale);
	if (this->scr == NULL)
		die("Failed to create screen texture: %s", SDL_GetError());

	this->keyboard = SDL_GetKeyboardState(NULL);
	SDL_SetTextureBlendMode(this->scr, SDL_BLENDMODE_BLEND);

	/* A hack to force i3wm to float the window on startup */
	SDL_SetWindowResizable(this->win, true);
}

void demoCleanup(Demo *this) {
	SDL_DestroyTexture(this->scr);
	SDL_DestroyRenderer(this->ren);
	SDL_DestroyWindow(this->win);

	SDL_Quit();

	free(this->pixels);
	PIF_imageFree(this->canv);
	PIF_imageFree(this->colormap);
	PIF_paletteFree(this->pal);
}

void demoFrame(Demo *this) {
	static uint64_t last = 0, now = 0;
	bool firstFrame = last == 0 && now == 0;
	last = now;
	now  = SDL_GetPerformanceCounter();

	if (firstFrame)
		return;

	this->dt = (double)(now - last) * 1000 / (double)SDL_GetPerformanceFrequency();
	this->elapsedTime += this->dt;
}

void demoDisplay(Demo *this) {
	SDL_SetRenderDrawColor(this->ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(this->ren);

	/* Checkerboard background */
	int size = 15;
	for (int y = 0; y < ceil((float)this->winH / size); ++ y) {
		for (int x = 0; x < ceil((float)this->winW / size); ++ x) {
			SDL_Rect rect = {
				.x = x * size,
				.y = y * size,
				.w = size,
				.h = size,
			};
			int shade = y % 2 == (x % 2 == 0)? 50 : 70;
			SDL_SetRenderDrawColor(this->ren, shade, shade, shade, SDL_ALPHA_OPAQUE);
			SDL_RenderFillRect(this->ren, &rect);
		}
	}

	/* Update screen texture */
	for (int i = 0; i < this->canv->size; ++ i) {
		uint8_t color = this->canv->buf[i];
		/* Make the texture pixel transparent if the canvas pixel is transparent */
		this->pixels[i] = color == PIF_TRANSPARENT? 0 : PIF_rgbToPixelRgba32(this->pal->map[color]);
	}

	SDL_UpdateTexture(this->scr, NULL, this->pixels, this->canv->w * sizeof(*this->pixels));

	/* Fit the viewport in the center of the window */
	int   w = this->winW;
	int   h = this->winH;
	float winAspectRatio = (float)this->winH / this->winW;

	if      (winAspectRatio > this->aspectRatio) h = w * this->aspectRatio;
	else if (winAspectRatio < this->aspectRatio) w = h / this->aspectRatio;

	this->viewport.x = this->winW / 2 - w / 2;
	this->viewport.y = this->winH / 2 - h / 2;
	this->viewport.w = w;
	this->viewport.h = h;

	SDL_RenderCopy(this->ren, this->scr, NULL, &this->viewport);
	SDL_RenderPresent(this->ren);
}

void demoEvent(Demo *this, SDL_Event *evt) {
	switch (evt->type) {
	case SDL_QUIT: this->running = false; break;

	case SDL_MOUSEMOTION:
		/* Convert window position to canvas position */
		this->mouseX = (float)(evt->motion.x - this->viewport.x) / this->viewport.w * this->canv->w;
		this->mouseY = (float)(evt->motion.y - this->viewport.y) / this->viewport.h * this->canv->h;
		break;

	case SDL_WINDOWEVENT:
		if (evt->window.event == SDL_WINDOWEVENT_RESIZED) {
			this->winW = evt->window.data1;
			this->winH = evt->window.data2;
		}
		break;

	case SDL_KEYDOWN:
		if (evt->key.keysym.sym == SDLK_ESCAPE)
			this->running = false;
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
