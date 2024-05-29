#include <stdlib.h>  /* srand */
#include <stdbool.h> /* bool, true, false */
#include <time.h>    /* time */
#include <locale.h>  /* locale, LC_CTYPE */
#include <assert.h>  /* assert */
#include <stdint.h>  /* uint64_t */

#include <sys/time.h> /* gettimeofday, struct timeval */

#include <ncurses.h>

#include "../shared.inc"

#define CHARMAP "  .,;|xX#"

#define BLACK 1
#define WHITE (sizeof(CHARMAP) - 2)

#define FPS 60

PIF_Palette *pal;
PIF_Image   *canv;

float  aspectRatio;
int    winW, winH;
double elapsedTime;
bool   running;

void setup(void);
void cleanup(void);

void render(double dt);
void handleKey(int key);

uint64_t getMs(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec / 1000 + tv.tv_sec * 1000;
}

int main(void) {
	srand(time(NULL));

	/* Generate monochrome palette */
	pal = PIF_paletteNew(sizeof(CHARMAP) - 1);
	for (int i = 0; i < pal->size; ++ i) {
		PIF_Rgb a   = {0,   0,   0};
		PIF_Rgb b   = {255, 255, 255};
		pal->map[i] = PIF_rgbLerp(a, b, (float)i / (pal->size - 1));
	}

	/* NCurses setup */
	setlocale(LC_CTYPE, "");
	initscr();
	raw();
	noecho();
	cbreak();
	timeout(-1);
	nodelay(stdscr, true);
	keypad(stdscr, true);
	curs_set(0);

	ESCDELAY = 0;

	/* PIF and generic data setup */
	getmaxyx(stdscr, winH, winW);
	canv        = PIF_imageNew(winW / 2, winH); /* The terminal font size ratio is usually 1:2 */
	aspectRatio = (float)winH / winW;
	running     = true;

	setup();
	uint64_t last, now = getMs();
	while (running) {
		last = now;
		now  = getMs();
		double dt = (double)(now - last);
		elapsedTime += dt;

		//printf("\033]0;%li\007\n", (long)now);

		/* Render */
		render(dt);

		/* Update screen texture */
		move(0, 0);
		for (int i = 0; i < canv->size; ++ i) {
			assert(canv->buf[i] < pal->size);
			addch(CHARMAP[canv->buf[i]]);
			addch(CHARMAP[canv->buf[i]]);

			/* Fill in space */
			if (winW % 2 != 0 && i % canv->w == canv->w - 1)
				addch(CHARMAP[PIF_TRANSPARENT]);
		}

		/* Input */
		int key = getch();
		switch (key) {
		case KEY_ENTER: case 27: case 10: running = false; break;
		case KEY_RESIZE:
			getmaxyx(stdscr, winH, winW);
			aspectRatio = (float)winH / winW;
			canv = PIF_imageResize(canv, winW / 2, winH, PIF_TRANSPARENT);
			break;
		}

		handleKey(key);

		napms(1000.0 / FPS);
	}
	cleanup();

	/* Cleanup PIF and generic data */
	PIF_imageFree(canv);
	PIF_paletteFree(pal);

	/* Cleanup NCurses */
	curs_set(1);
	endwin();
}
