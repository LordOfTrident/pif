#include <time.h> /* time */
#include <math.h> /* round, sin, cos, sqrt, pow, fabs */

#include "shared.inc"

#define TITLE "Triangles | Escape to quit"
#define SCR_W 600
#define SCR_H 440
#define SCALE 1
#define POINT_BUTTON_SIZE (10 / SCALE)

typedef struct {
	int x, y;
} Point;

typedef struct {
	int     p1, p2, p3;
	uint8_t color;
} Triangle;

static struct {
	Demo d;

	int dragging;

	Point    ps[64];
	Triangle tris[64];
	int psCount, trisCount;
} demo;

static void addPoint(int x, int y) {
	assert(demo.psCount < (int)arraySize(demo.ps));
	demo.ps[demo.psCount ++] = (Point){x, y};
}

static void addTriangle(int p1, int p2, int p3, uint8_t color) {
	assert(demo.trisCount < (int)arraySize(demo.tris));
	demo.tris[demo.trisCount ++] = (Triangle){p1, p2, p3, color};
}

static float lerp(float a, float b, float f) {
    return a + f * (b - a); /* Fast lerp */
}

static void splitTriangle(int p1, int p2, int p3, int depth) {
	if (depth <= 0) {
		addTriangle(p1, p2, p3, rand() % (demo.d.pal->size - 1) + 1);
		return;
	}

	/* Find the longest side middle split point */
	int x  = round(lerp(demo.ps[p2].x, demo.ps[p3].x, 0.5));
	int y  = round(lerp(demo.ps[p2].y, demo.ps[p3].y, 0.5));
	int p4 = demo.psCount;
	for (int i = demo.psCount; i --> 0;) {
		if (x == demo.ps[i].x && y == demo.ps[i].y) {
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
	addPoint(demo.d.canv->w - 1, 0);
	addPoint(0, demo.d.canv->h - 1);
	addPoint(demo.d.canv->w - 1, demo.d.canv->h - 1);

	-- depth;
	splitTriangle(0, 2, 1, depth);
	splitTriangle(3, 1, 2, depth);

	/* Offset the points for some variety */
	for (int i = 0; i < demo.psCount; ++ i) {
		Point *p = demo.ps + i;

		float angle = (float)(rand() % 628) / 100;
		int   x = cos(angle) * offset;
		int   y = sin(angle) * offset;

		if (p->x != 0 && p->x != demo.d.canv->w - 1) p->x += x;
		if (p->y != 0 && p->y != demo.d.canv->h - 1) p->y += y;
	}
}

static void setup(void) {
	demoSetup(&demo.d, TITLE, SCR_W, SCR_H, SCALE, 0.5, NULL);

	demo.dragging = -1;

	srand(time(NULL));
	generateTriangles(6, round(0.054 * demo.d.canv->h));
}

static void cleanup(void) {
	demoCleanup(&demo.d);
}

static void renderTriangles(void) {
	PIF_imageSetShader(demo.d.canv, PIF_blendShader, demo.d.colormap);

	for (int i = 0; i < demo.trisCount; ++ i) {
		Triangle *tri = demo.tris + i;
		Point p1 = demo.ps[tri->p1];
		Point p2 = demo.ps[tri->p2];
		Point p3 = demo.ps[tri->p3];

		PIF_imageFillTriangle(demo.d.canv, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, tri->color);
	}
}

static void renderPoints(void) {
	PIF_imageSetShader(demo.d.canv, PIF_blendShader, demo.d.colormap);

	for (int i = 0; i < demo.psCount; ++ i) {
		Point *p = demo.ps + i;

		if (i == demo.dragging) {
			PIF_imageSetShader(demo.d.canv, NULL, NULL);
			PIF_imageDrawCircle(demo.d.canv, p->x, p->y, POINT_BUTTON_SIZE, 1, WHITE);
		}

		int off = 0.5 * POINT_BUTTON_SIZE;
		PIF_imageDrawLine(demo.d.canv, p->x - off, p->y - off, p->x + off, p->y + off, 0, WHITE);
		PIF_imageDrawLine(demo.d.canv, p->x + off, p->y - off, p->x - off, p->y + off, 0, WHITE);

		if (i == demo.dragging)
			PIF_imageSetShader(demo.d.canv, PIF_blendShader, demo.d.colormap);
	}
}

static void render(void) {
	PIF_imageClear(demo.d.canv, BLACK);

	renderTriangles();
	renderPoints();

	demoDisplay(&demo.d);
}

static void input(void) {
	SDL_Event evt;
	while (SDL_PollEvent(&evt)) {
		demoEvent(&demo.d, &evt);

		switch (evt.type) {
		case SDL_MOUSEMOTION:
			if (demo.dragging != -1) {
				demo.ps[demo.dragging].x = demo.d.mouseX;
				demo.ps[demo.dragging].y = demo.d.mouseY;
			}
			break;

		case SDL_MOUSEBUTTONDOWN:
			/* Find a point touching the mouse */
			for (int i = 0; i < demo.psCount; ++ i) {
				float dist = fabs(sqrt(pow(demo.ps[i].x - demo.d.mouseX, 2) +
				                       pow(demo.ps[i].y - demo.d.mouseY, 2)));
				if (round(dist) <= POINT_BUTTON_SIZE) {
					demo.dragging = i;
					break;
				}
			}
			break;

		case SDL_MOUSEBUTTONUP: demo.dragging = -1; break;
		}
	}
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
