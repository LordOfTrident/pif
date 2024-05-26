#include <cmath> /* std::sqrt, std::pow, std::round */

#define TITLE "Paint"
#define SCR_W 700
#define SCR_H 550
#define SCALE 2

#include "pif_sdl2.inc"

struct TransparentReplaceData {
	uint8_t color1, color2, size;
};

TransparentReplaceData transparentReplaceData = {16, 13, 3};

PIF_Image *colormap, *invertmap, *img;
PIF_Rect   imgDest;
PIF_Font  *font;

uint8_t borderColor, brushColor = BLACK;

int  brushSize = 5, brushX, brushY, brushPrevX, brushPrevY;
bool drawing, colorPicker, helpText = true;

void setup() {
	colormap    = PIF_paletteCreateColormap(pal, PIF_DEFAULT_SHADES, 0.5);
	font        = PIF_fontNewDefault();
	borderColor = PIF_paletteClosest(pal, PIF_Rgb{255, 255, 0});

	int size = PIF_min(canv->w, canv->h) - 2;
	img = PIF_imageNew(size, size);

	// Create a colormap to invert colors
	invertmap = PIF_imageNew(pal->size, 1);
	for (int i = 0; i < pal->size; ++ i) {
		// Invert transparency into white
		PIF_Rgb rgb = i == 0? PIF_Rgb{0, 0, 0} : pal->map[i];
		rgb.r = 255 - rgb.r;
		rgb.g = 255 - rgb.g;
		rgb.b = 255 - rgb.b;
		invertmap->buf[i] = PIF_paletteClosest(pal, rgb);
	}

	font = PIF_fontNewDefault();

	PIF_imageClear(img, WHITE);
	PIF_imageSkipTransparent(img,  false); // Overwrite pixels with transparent
	PIF_imageSkipTransparent(canv, false);

	SDL_ShowCursor(SDL_DISABLE);
}

void cleanup() {
	PIF_imagesFree(colormap, invertmap, img);
	PIF_fontFree(font);
}

void invertShader(int x, int y, uint8_t *pixel, uint8_t color, PIF_Image *img) {
	(void)x; (void)y; (void)color;
	PIF_Image *invertmap = (PIF_Image*)img->data;

	*pixel = invertmap->buf[*pixel];
}

void transparencyReplaceShader(int x, int y, uint8_t *pixel, uint8_t color, PIF_Image *img) {
	(void)img; (void)y;

	if (color != PIF_TRANSPARENT) {
		*pixel = color;
		return;
	}

	TransparentReplaceData *data = (TransparentReplaceData*)img->data;

	// Replace transparent areas with a checkerboard
	*pixel = x / data->size % 2 == (y / data->size % 2 == 0)? data->color1 : data->color2;
}

float distance(int x1, int y1, int x2, int y2) {
	return std::sqrt(std::pow(x1 - x2, 2) + std::pow(y1 - y2, 2));
}

float lerp(float a, float b, float f) {
    return a + f * (b - a); // Fast lerp
}

void draw(int xStart, int yStart, int xEnd, int yEnd) {
	// A crappy way to fill gaps, but it does not matter for the demo
	float step = 1.0 / distance(xStart, yStart, xEnd, yEnd);
	for (float t = 0; t <= 1; t += step) {
		int x = std::round(lerp(xStart, xEnd, t));
		int y = std::round(lerp(yStart, yEnd, t));
		PIF_imageFillCircle(img, x, y, brushSize, brushColor);
	}
}

void renderColorPicker() {
	const int colorSize = 15, showColors = 3, padding = 5, gap = 2;

	// Background
	PIF_Rect dest;
	dest.w = (colorSize + gap) * (showColors * 2 + 1) + padding * 2 - gap;
	dest.h = colorSize + padding * 2;
	dest.x = canv->w / 2 - dest.w / 2;
	dest.y = canv->h / 2 - dest.h / 2;
	PIF_imageSetShader(canv, PIF_blendShader, colormap);
	PIF_imageFillRect(canv, &dest, BLACK);
	PIF_imageSetShader(canv, NULL, NULL);
	PIF_imageDrawRect(canv, &dest, 0, WHITE);

	// Helper text
	PIF_imageSetShader(canv, invertShader, invertmap);
	PIF_fontRenderText(font, "Use scroll wheel to change color", canv,
	                   dest.x, dest.y + dest.h + padding, 1);
	PIF_imageSetShader(canv, NULL, NULL);

	// Selected color
	dest.w = colorSize;
	dest.h = colorSize;
	dest.x = canv->w / 2 - dest.w / 2;
	dest.y = canv->h / 2 - dest.h / 2;

	// Brush color could be transparent, so enable the transparency replace shader
	PIF_imageSetShader(canv, transparencyReplaceShader, &transparentReplaceData);
	PIF_imageFillRect(canv, &dest, brushColor);

	// Neighbor colors
	for (int i = 1; i <= showColors; ++ i) {
		PIF_Rect rect = dest;

		rect.x -= (colorSize + gap) * i;
		PIF_imageFillRect(canv, &rect, brushColor - i);

		rect.x = dest.x + (colorSize + gap) * i;
		PIF_imageFillRect(canv, &rect, brushColor + i);
	}
	PIF_imageSetShader(canv, NULL, NULL);

	// Selector icon
	int x1 = dest.x, y1 = dest.y - colorSize / 2 - 2;
	int x2 = dest.x + colorSize - 1, y2 = dest.y - colorSize / 2 - 2;
	int x3 = dest.x + colorSize / 2, y3 = dest.y - 2;
	PIF_imageFillTriangle(canv, x1, y1, x2, y2, x3, y3, BLACK);

	-- x1; -- y1;
	++ x2; -- y2;
	PIF_imageDrawTriangle(canv, x1, y1, x2, y2, x3, y3, 0, WHITE);

	// Selected color border
	-- dest.x;
	-- dest.y;
	dest.w += 2;
	dest.h += 2;
	PIF_imageDrawRect(canv, &dest, 1, borderColor);
}

void renderHelpText() {
	const char *text =
		"h - OPEN/CLOSE HELP MENU\n"
		"left mouse - DRAW\n"
		"right mouse - COLOR PICKER\n"
		"scroll wheel - CHANGE BRUSH SIZE\n"
		"right arrow - GROW CANVAS\n"
		"left arrow - SHRINK CANVAS\n"
		"quit - ESCAPE";

	int w, h;
	PIF_fontSetScale(font, 2);
	PIF_fontTextSize(font, text, &w, &h);
	int x = canv->w / 2 - w / 2, y = canv->h / 2 - h / 2;

	const int padding = 10;
	PIF_Rect dest;
	dest.x = x - padding;
	dest.y = y - padding;
	dest.w = w + padding * 2;
	dest.h = h + padding * 2;
	PIF_imageSetShader(canv, PIF_blendShader, colormap);
	PIF_imageFillRect(canv, &dest, BLACK);
	PIF_imageSetShader(canv, NULL, NULL);

	PIF_imageDrawRect(canv, &dest, 0, WHITE);

	PIF_fontRenderText(font, text, canv, x, y, WHITE);
	PIF_fontSetScale(font, 1);
}

void render(double dt) {
	(void)dt;
	PIF_imageClear(canv, BLACK);

	// Canvas
	imgDest.x = canv->w / 2 - img->w / 2;
	imgDest.y = canv->h / 2 - img->h / 2;
	imgDest.w = img->w;
	imgDest.h = img->h;

	PIF_imageSetShader(canv, transparencyReplaceShader, &transparentReplaceData);
	PIF_imageBlit(canv, &imgDest, img, NULL);
	PIF_imageSetShader(canv, NULL, NULL);

	// Canvas border
	PIF_Rect dest = imgDest;
	-- dest.x;
	-- dest.y;
	dest.w += 2;
	dest.h += 2;
	PIF_imageDrawRect(canv, &dest, 2, borderColor);

	// Brush
	PIF_imageSetShader(canv, invertShader, invertmap);
	PIF_imageDrawCircle(canv, mouseX, mouseY, brushSize + 1, 1, 1);
	PIF_imageSetShader(canv, NULL, NULL);

	if (colorPicker)
		renderColorPicker();
	if (helpText)
		renderHelpText();

	if (keyboard[SDL_SCANCODE_LEFT] && img->w > 10)
		img = PIF_imageResize(img, img->w - 1, img->h, PIF_TRANSPARENT);

	if (keyboard[SDL_SCANCODE_RIGHT] && img->w < canv->w - 2)
		img = PIF_imageResize(img, img->w + 1, img->h, PIF_TRANSPARENT);
}

void handleEvent(SDL_Event *evt) {
	switch (evt->type) {
	case SDL_MOUSEWHEEL:
		if (colorPicker)
			brushColor += evt->wheel.y;
		else {
			brushSize += evt->wheel.y;
			if (brushSize < 1)  brushSize = 1;
			if (brushSize > 40) brushSize = 40;
		}
		break;

	case SDL_MOUSEMOTION:
		brushPrevX = brushX;
		brushPrevY = brushY;

		brushX = mouseX - imgDest.x;
		brushY = mouseY - imgDest.y;

		if (drawing)
			draw(brushX, brushY, brushPrevX, brushPrevY);
		break;

	case SDL_KEYDOWN:
		if (!colorPicker && evt->key.keysym.sym == SDLK_h)
			helpText = !helpText;
		break;

	case SDL_MOUSEBUTTONDOWN:
		if (evt->button.button == SDL_BUTTON_LEFT) {
			drawing = true;
			draw(brushX, brushY, brushX, brushY);
		} else if (!helpText && evt->button.button == SDL_BUTTON_RIGHT)
			colorPicker = true;
		break;

	case SDL_MOUSEBUTTONUP:
		if (evt->button.button == SDL_BUTTON_LEFT)
			drawing = false;
		else if (evt->button.button == SDL_BUTTON_RIGHT)
			colorPicker = false;
		break;
	}
}
