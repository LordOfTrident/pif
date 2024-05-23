#include <math.h> /* tan, cos, sin, sqrt, pow, round */

#include <demo.c>

#define TITLE "Dots 3D | Escape to quit"
#define SCR_W 700
#define SCR_H 500
#define SCALE 1

#define FOV         70
#define DOT_SIZE    0.2
#define BOX_SIZE    8
#define BOX_SPACING 1.5
#define BOX_SPEED   0.0007

#ifndef M_PI
#	define M_PI 3.1415926535
#endif

#define FOV_RAD ((float)FOV * M_PI / 180)

typedef struct {
	float x, y, z;
} Point;

typedef struct {
	Point   p, rotated;
	uint8_t color;
} Dot;

static struct {
	DemoData d;

	PIF_Font *font;
	float     projMat[3][3];
	Point     cam;
	Dot       dots[BOX_SIZE * BOX_SIZE * BOX_SIZE];
} demo;

static void setup(void) {
	demoSetup(&demo.d, TITLE, SCR_W, SCR_H, SCALE, 0.5, NULL);

	demo.font  = PIF_fontNewDefault();
	demo.cam.z = (float)BOX_SIZE * BOX_SPACING * 1.1;

	/* Calculate projection matrix */
	float scale       = 1.0 / tan(FOV_RAD / 2);
	float aspectRatio = (float)demo.d.canv->h / demo.d.canv->w;

	demo.projMat[0][0] = scale * aspectRatio;
	demo.projMat[1][1] = scale;
	demo.projMat[2][2] = 1;

	/* Initialize dots */
	for (float z = 0; z < BOX_SIZE; ++ z) {
		for (float y = 0; y < BOX_SIZE; ++ y) {
			for (float x = 0; x < BOX_SIZE; ++ x) {
				static int i = 0;
				demo.dots[i] = (Dot){
					.p = (Point){
						.x = (x - (float)BOX_SIZE / 2 + 0.5) * BOX_SPACING,
						.y = (y - (float)BOX_SIZE / 2 + 0.5) * BOX_SPACING,
						.z = (z - (float)BOX_SIZE / 2 + 0.5) * BOX_SPACING,
					},
					.color = PIF_paletteClosest(demo.d.pal, (PIF_Rgb){
						.r = x / BOX_SIZE * 255,
						.g = y / BOX_SIZE * 255,
						.b = z / BOX_SIZE * 255,
					}),
				};
				++ i;
			}
		}
	}
}

static void cleanup(void) {
	demoCleanup(&demo.d);

	PIF_fontFree(demo.font);
}

static Point mulPointByMatrix3x3(Point p, float mat[3][3]) {
	return (Point){
		.x = p.x * mat[0][0] + p.y * mat[1][0] + p.z * mat[2][0],
		.y = p.x * mat[0][1] + p.y * mat[1][1] + p.z * mat[2][1],
		.z = p.x * mat[0][2] + p.y * mat[1][2] + p.z * mat[2][2],
	};
}

static void rotateDots(void) {
	for (int i = 0; i < (int)arraySize(demo.dots); ++ i) {
		Point rotated = demo.dots[i].p;
		float t = demo.d.elapsedTime * BOX_SPEED, tCos = cos(t), tSin = sin(t);

		float zRotMat[3][3] = {0};
		zRotMat[0][0] =  tCos;
		zRotMat[0][2] =  tSin;
		zRotMat[2][0] = -tSin;
		zRotMat[2][2] =  tCos;
		zRotMat[1][1] =  1;
		rotated = mulPointByMatrix3x3(rotated, zRotMat);

		float xRotMat[3][3] = {0};
		xRotMat[0][0] =  tCos;
		xRotMat[0][1] =  tSin;
		xRotMat[1][0] = -tSin;
		xRotMat[1][1] =  tCos;
		xRotMat[2][2] =  1;
		rotated = mulPointByMatrix3x3(rotated, xRotMat);

		demo.dots[i].rotated = rotated;
	}
}

static void sortDots(void) {
	for (int i = 0; i < (int)arraySize(demo.dots) - 1; ++ i) {
		bool swapped = false;
		for (int j = 0; j < (int)arraySize(demo.dots) - i - 1; ++ j) {
			if (demo.dots[j].rotated.z > demo.dots[j + 1].rotated.z)
				continue;

			Dot tmp = demo.dots[j];
			demo.dots[j]     = demo.dots[j + 1];
			demo.dots[j + 1] = tmp;
			swapped = true;
		}

		if (swapped == false)
			break;
	}
}

static void renderDots(void) {
	for (int i = 0; i < (int)arraySize(demo.dots); ++ i) {
		Dot  *dot = demo.dots + i;
		Point p   = {
			.x = dot->rotated.x + demo.cam.x,
			.y = dot->rotated.y + demo.cam.y,
			.z = dot->rotated.z + demo.cam.z,
		};

		float dist = p.z;
		if (dist < 0)
			break;

		p = mulPointByMatrix3x3(p, demo.projMat);
		int x = round((1.0 + p.x / p.z) / 2 * demo.d.canv->w);
		int y = round((1.0 + p.y / p.z) / 2 * demo.d.canv->h);

		float   fog   = (dist - 4) / 25;
		uint8_t color = PIF_shadeColor(dot->color, 0.5 + fog, demo.d.colormap);

		int radius = round(1.0 / dist / demo.projMat[1][1] * DOT_SIZE * demo.d.canv->h);
		if (radius < 1)
			radius = 1;

		PIF_imageFillCircle(demo.d.canv, x, y, radius, color);
	}
}

static void render(void) {
	PIF_imageClear(demo.d.canv, BLACK);

	rotateDots();
	sortDots();
	renderDots();

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
