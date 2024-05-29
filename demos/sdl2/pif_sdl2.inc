#include <stdlib.h>  /* malloc, free, srand */
#include <stdbool.h> /* bool, true, false */
#include <stdint.h>  /* uint64_t, uint32_t, uint8_t */
#include <time.h>    /* time */

#include <SDL2/SDL.h>

#include "../shared.inc"

/* paletteData, BLACK, WHITE */
#include "palette.inc"

SDL_Window    *win;
SDL_Renderer  *ren;
SDL_Texture   *scr;    /* Screen texture */
uint32_t      *pixels; /* Pixel array for updating SDL2 screen texture */
SDL_Rect       viewport;
const uint8_t *keyboard;
PIF_Palette   *pal;
PIF_Image     *canv;

float  aspectRatio;
int    winW, winH, mouseX, mouseY;
double elapsedTime;
bool   running, mouseDown, mouseClicked;

void setup(void);
void cleanup(void);

void render(double dt);
void handleEvent(SDL_Event *evt);

int main(void) {
	srand(time(NULL));

	/* Load built-in palette from memory */
	pal = PIF_paletteNew(sizeof(paletteData) / 3);
	for (int i = 0; i < pal->size; ++ i) {
		pal->map[i].r = paletteData[i * 3];
		pal->map[i].g = paletteData[i * 3 + 1];
		pal->map[i].b = paletteData[i * 3 + 2];
	}

	/* PIF and generic data setup */
	canv        = PIF_imageNew(SCR_W / SCALE, SCR_H / SCALE);
	pixels      = (uint32_t*)malloc(canv->size * sizeof(uint32_t));
	winW        = SCR_W;
	winH        = SCR_H;
	aspectRatio = (float)SCR_H / SCR_W;
	running     = true;

	/* SDL2 setup */
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		die("Failed to initialize SDL2: %s", SDL_GetError());

	win = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                       SCR_W, SCR_H, SDL_WINDOW_SHOWN);
	if (win == NULL)
		die("Failed to create window: %s", SDL_GetError());

	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (ren == NULL)
		die("Failed to create renderer: %s", SDL_GetError());

	scr = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
	                        SCR_W / SCALE, SCR_H / SCALE);
	if (scr == NULL)
		die("Failed to create screen texture: %s", SDL_GetError());

	keyboard = SDL_GetKeyboardState(NULL);
	SDL_SetTextureBlendMode(scr, SDL_BLENDMODE_BLEND);

	/* A hack to force i3wm to float the window on startup */
	SDL_SetWindowResizable(win, SDL_TRUE);

	setup();
	uint64_t last, now = SDL_GetPerformanceCounter();
	while (running) {
		last = now;
		now  = SDL_GetPerformanceCounter();
		double dt = (double)(now - last) * 1000 / (double)SDL_GetPerformanceFrequency();
		elapsedTime += dt;

		/* Render */
		render(dt);

		SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(ren);

		/* Update screen texture */
		for (int i = 0; i < canv->size; ++ i) {
			uint8_t color = canv->buf[i];
			/* Make the texture pixel transparent if the canvas pixel is transparent */
			pixels[i] = color == PIF_TRANSPARENT? 0 : PIF_rgbToPixelRgba32(pal->map[color]);
		}

		SDL_UpdateTexture(scr, NULL, pixels, canv->w * sizeof(*pixels));

		/* Fit the viewport in the center of the window */
		float winAspectRatio = (float)winH / winW;
		viewport.w = winAspectRatio < aspectRatio? winH / aspectRatio : winW;
		viewport.h = winAspectRatio > aspectRatio? winW * aspectRatio : winH;
		viewport.x = winW / 2 - viewport.w / 2;
		viewport.y = winH / 2 - viewport.h / 2;
		SDL_RenderCopy(ren, scr, NULL, &viewport);

		/* Border around canvas (for when the window is resized) */
		SDL_Rect rect;
		rect.x = viewport.x - 1;
		rect.y = viewport.y - 1;
		rect.w = viewport.w + 2;
		rect.h = viewport.h + 2;
		SDL_SetRenderDrawColor(ren, 255, 255, 255, SDL_ALPHA_OPAQUE);
		SDL_RenderDrawRect(ren, &rect);

		SDL_RenderPresent(ren);

		static bool mouseWasDown = false;

		SDL_Event evt;
		while (SDL_PollEvent(&evt)) {
			switch (evt.type) {
			case SDL_QUIT: running = false; break;

			case SDL_MOUSEMOTION:
				/* Convert window position to canvas position */
				mouseX = (float)(evt.motion.x - viewport.x) / viewport.w * canv->w;
				mouseY = (float)(evt.motion.y - viewport.y) / viewport.h * canv->h;
				break;

			case SDL_WINDOWEVENT:
				if (evt.window.event == SDL_WINDOWEVENT_RESIZED) {
					winW = evt.window.data1;
					winH = evt.window.data2;
				}
				break;

			case SDL_KEYDOWN:
				if (evt.key.keysym.sym == SDLK_ESCAPE)
					running = false;
				break;

			case SDL_MOUSEBUTTONDOWN: mouseDown = true;  break;
			case SDL_MOUSEBUTTONUP:   mouseDown = false; break;
			}

			handleEvent(&evt);
		}

		mouseClicked = mouseWasDown && !mouseDown;
		mouseWasDown = mouseDown;
	}
	cleanup();

	/* Cleanup PIF and generic data */
	free(pixels);
	PIF_imageFree(canv);
	PIF_paletteFree(pal);

	/* Cleanup SDL2 */
	SDL_DestroyTexture(scr);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);

	SDL_Quit();
	return 0;
}
