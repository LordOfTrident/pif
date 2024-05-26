#include <math.h>   /* round, cos, sin, tan */
#include <stdint.h> /* uint8_t */

#define TITLE "Dots 3D"
#define SCR_W 700
#define SCR_H 500
#define SCALE 1

#include "pif_sdl2.inc"

#define FOV         70
#define DOT_SIZE    0.2
#define BOX_SIZE    8
#define BOX_SPACING 1.5
#define BOX_SPEED   0.0007

#define FOV_RAD    ((float)FOV * M_PI / 180)
#define DOTS_COUNT BOX_SIZE * BOX_SIZE * BOX_SIZE

typedef struct {
	float x, y, z;
} Point;

typedef struct {
	Point   p, rotated;
	uint8_t color;
} Dot;

PIF_Image *img, *colormap;
PIF_Font  *font;
float      projMat[3][3];
Point      cam;
Dot        dots[DOTS_COUNT];

void setup(void) {
	colormap = PIF_paletteCreateColormap(pal, PIF_DEFAULT_SHADES, 0.7);
	img      = generateRandomImage(300, 300, BLACK, colormap);

	font  = PIF_fontNewDefault();
	cam.z = (float)BOX_SIZE * BOX_SPACING * 1.1;

	/* Calculate projection matrix */
	float scale       = 1.0 / tan(FOV_RAD / 2);
	float aspectRatio = (float)canv->h / canv->w;

	projMat[0][0] = scale * aspectRatio;
	projMat[1][1] = scale;
	projMat[2][2] = 1;

	/* Initialize dots */
	for (float z = 0; z < BOX_SIZE; ++ z) {
		for (float y = 0; y < BOX_SIZE; ++ y) {
			for (float x = 0; x < BOX_SIZE; ++ x) {
				static int i = 0;
				dots[i] = (Dot){
					.p = (Point){
						.x = (x - (float)BOX_SIZE / 2 + 0.5) * BOX_SPACING,
						.y = (y - (float)BOX_SIZE / 2 + 0.5) * BOX_SPACING,
						.z = (z - (float)BOX_SIZE / 2 + 0.5) * BOX_SPACING,
					},
					.color = PIF_paletteClosest(pal, (PIF_Rgb){
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

void cleanup(void) {
	PIF_imagesFree(img, colormap);
	PIF_fontFree(font);
}

Point mulPointByMatrix3x3(Point p, float mat[3][3]) {
	return (Point){
		.x = p.x * mat[0][0] + p.y * mat[1][0] + p.z * mat[2][0],
		.y = p.x * mat[0][1] + p.y * mat[1][1] + p.z * mat[2][1],
		.z = p.x * mat[0][2] + p.y * mat[1][2] + p.z * mat[2][2],
	};
}

void rotateDots(void) {
	for (int i = 0; i < DOTS_COUNT; ++ i) {
		Point rotated  = dots[i].p;
		float angle    = elapsedTime * BOX_SPEED;
		float angleCos = cos(angle), angleSin = sin(angle);

		float zRotMat[3][3] = {0};
		zRotMat[0][0] =  angleCos;
		zRotMat[0][2] =  angleSin;
		zRotMat[2][0] = -angleSin;
		zRotMat[2][2] =  angleCos;
		zRotMat[1][1] =  1;
		rotated = mulPointByMatrix3x3(rotated, zRotMat);

		float xRotMat[3][3] = {0};
		xRotMat[0][0] =  angleCos;
		xRotMat[0][1] =  angleSin;
		xRotMat[1][0] = -angleSin;
		xRotMat[1][1] =  angleCos;
		xRotMat[2][2] =  1;
		rotated = mulPointByMatrix3x3(rotated, xRotMat);

		dots[i].rotated = rotated;
	}
}

void sortDots(void) {
	for (int i = 0; i < DOTS_COUNT - 1; ++ i) {
		bool swd = false;
		for (int j = 0; j < DOTS_COUNT - i - 1; ++ j) {
			if (dots[j].rotated.z > dots[j + 1].rotated.z)
				continue;

			Dot tmp = dots[j];
			dots[j]     = dots[j + 1];
			dots[j + 1] = tmp;
			swd = true;
		}

		if (swd == false)
			break;
	}
}

void renderDots(void) {
	for (int i = 0; i < DOTS_COUNT; ++ i) {
		Dot  *dot = dots + i;
		Point p   = {
			.x = dot->rotated.x + cam.x,
			.y = dot->rotated.y + cam.y,
			.z = dot->rotated.z + cam.z,
		};

		float dist = p.z;
		if (dist < 0)
			break;

		p = mulPointByMatrix3x3(p, projMat);
		int x = round((1.0 + p.x / p.z) / 2 * canv->w);
		int y = round((1.0 + p.y / p.z) / 2 * canv->h);

		float   fog   = (dist - 4) / 25;
		uint8_t color = PIF_shadeColor(dot->color, 0.5 + fog, colormap);

		int radius = round(1.0 / dist / projMat[1][1] * DOT_SIZE * canv->h);
		if (radius < 1)
			radius = 1;

		PIF_imageFillCircle(canv, x, y, radius, color);
	}
}

void render(double dt) {
	(void)dt;
	PIF_imageClear(canv, BLACK);

	rotateDots();
	sortDots();
	renderDots();
}

void handleEvent(SDL_Event *evt) {
	(void)evt;
}
