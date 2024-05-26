#include <math.h>   /* sqrt */
#include <assert.h> /* assert */

#define TITLE "Triangles"
#define SCR_W 600
#define SCR_H 440
#define SCALE 1

#include "pif_sdl2.inc"

#define POINT_BUTTON_SIZE (10 / SCALE)

typedef struct {
	int x, y;
} Point;

typedef struct {
	int     p1, p2, p3;
	uint8_t color;
} Triangle;

PIF_Image *colormap;

int dragging = -1;

Point    ps[64];
Triangle tris[64];
int psCount, trisCount;

static void addPoint(int x, int y) {
	assert(psCount < arraySize(ps));
	ps[psCount ++] = (Point){x, y};
}

static void addTriangle(int p1, int p2, int p3, uint8_t color) {
	assert(trisCount < arraySize(tris));
	tris[trisCount ++] = (Triangle){p1, p2, p3, color};
}

static float lerp(float a, float b, float f) {
    return a + f * (b - a); /* Fast lerp */
}

static void splitTriangle(int p1, int p2, int p3, int depth) {
	if (depth <= 0) {
		addTriangle(p1, p2, p3, rand() % (pal->size - 1) + 1);
		return;
	}

	/* Find the longest side middle split point */
	int x  = round(lerp(ps[p2].x, ps[p3].x, 0.5));
	int y  = round(lerp(ps[p2].y, ps[p3].y, 0.5));
	int p4 = psCount;
	for (int i = psCount; i --> 0;) {
		if (x == ps[i].x && y == ps[i].y) {
			p4 = i;
			goto found;
		}
	}

	/* Point not found, add it */
	addPoint(x, y);

found:
	-- depth;
	splitTriangle(p4, p3, p1, depth);
	splitTriangle(p4, p1, p2, depth);
}

static void generateTriangles(int depth, int offset) {
	addPoint(0, 0);
	addPoint(canv->w - 1, 0);
	addPoint(0, canv->h - 1);
	addPoint(canv->w - 1, canv->h - 1);

	-- depth;
	splitTriangle(0, 2, 1, depth);
	splitTriangle(3, 1, 2, depth);

	/* Offset the points for some variety */
	for (int i = 0; i < psCount; ++ i) {
		Point *p = ps + i;

		float angle = (float)(rand() % 628) / 100;
		int   x = cos(angle) * offset;
		int   y = sin(angle) * offset;

		if (p->x != 0 && p->x != canv->w - 1) p->x += x;
		if (p->y != 0 && p->y != canv->h - 1) p->y += y;
	}
}

void setup(void) {
	colormap = PIF_paletteCreateColormap(pal, PIF_DEFAULT_SHADES, 0.5);

	generateTriangles(6, round(0.054 * canv->h));
}

void cleanup(void) {
	PIF_imagesFree(colormap);
}

void renderTriangles(void) {
	PIF_imageSetShader(canv, PIF_blendShader, colormap);

	for (int i = 0; i < trisCount; ++ i) {
		Triangle *tri = tris + i;
		Point p1 = ps[tri->p1];
		Point p2 = ps[tri->p2];
		Point p3 = ps[tri->p3];

		PIF_imageFillTriangle(canv, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, tri->color);
	}
}

void renderPoints(void) {
	PIF_imageSetShader(canv, PIF_blendShader, colormap);

	for (int i = 0; i < psCount; ++ i) {
		Point *p = ps + i;

		if (i == dragging) {
			PIF_imageSetShader(canv, NULL, NULL);
			PIF_imageDrawCircle(canv, p->x, p->y, POINT_BUTTON_SIZE, 1, WHITE);
		}

		int off = 0.5 * POINT_BUTTON_SIZE;
		PIF_imageDrawLine(canv, p->x - off, p->y - off, p->x + off, p->y + off, 0, WHITE);
		PIF_imageDrawLine(canv, p->x + off, p->y - off, p->x - off, p->y + off, 0, WHITE);

		if (i == dragging)
			PIF_imageSetShader(canv, PIF_blendShader, colormap);
	}
}

void render(double dt) {
	(void)dt;
	PIF_imageClear(canv, BLACK);

	renderTriangles();
	renderPoints();
}

void handleEvent(SDL_Event *evt) {
	switch (evt->type) {
	case SDL_MOUSEBUTTONUP: dragging = -1; break;
	case SDL_MOUSEBUTTONDOWN:
		/* Find a point touching the mouse */
		for (int i = 0; i < psCount; ++ i) {
			float dist = sqrt(pow(ps[i].x - mouseX, 2) +
			                  pow(ps[i].y - mouseY, 2));
			if (round(dist) <= POINT_BUTTON_SIZE) {
				dragging = i;
				break;
			}
		}
		break;

	case SDL_MOUSEMOTION:
		if (dragging != -1) {
			ps[dragging].x = mouseX;
			ps[dragging].y = mouseY;
		}
		break;
	}
}
