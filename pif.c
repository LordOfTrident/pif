#include "pif.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PIF_error(ERR, MSG) ((ERR) != NULL? (*(ERR) = MSG, NULL) : NULL)

#define PIF_checkAlloc(PTR)                                                         \
	do {                                                                            \
		if (PTR == NULL) {                                                          \
			fprintf(stderr, "%s:%i: PIF allocation failure\n", __FILE__, __LINE__); \
			abort();                                                                \
		}                                                                           \
	} while (0)

static void PIF_invertMatrix2x2(float input[2][2], float output[2][2]) {
	float n, mat[2][2] = {
		{input[0][0], input[0][1]},
		{input[1][0], input[1][1]},
	};

	output[0][0] = 1;
	output[1][0] = 0;
	output[0][1] = 0;
	output[1][1] = 1;

	n = mat[0][0];
	for (int x = 0; x < 2; ++ x) {
		output[0][x] /= n;
		mat[0][x]    /= n;
	}

	n = mat[1][0];
	for (int x = 0; x < 2; ++ x) {
		output[1][x] -= output[0][x] * n;
		mat[1][x]    -= mat[0][x]    * n;
	}

	n = mat[1][1];
	for (int x = 0; x < 2; ++ x) {
		output[1][x] /= n;
		/*mat[1][x]    /= n;*/
	}

	n = mat[0][1];
	for (int x = 0; x < 2; ++ x) {
		output[0][x] -= output[1][x] * n;
		/*mat[0][x]    -= mat[1][x]    * n;*/
	}
}

static void PIF_applyTransformMatrix2x2(int *x, int *y, float mat[2][2]) {
	int resultX = *x * mat[0][0] + *y * mat[1][0];
	int resultY = *x * mat[0][1] + *y * mat[1][1];

	*x = resultX;
	*y = resultY;
}

static void PIF_transformRect(PIF_Rect *rect, float mat[2][2], int cx, int cy, int ps[4][2]) {
	PIF_assert(rect != NULL);

	int ox = rect->w / 2 + rect->x;
	int oy = rect->h / 2 + rect->y;

	ps[0][0] = -rect->w / 2 + cx, ps[0][1] = -rect->h / 2 + cy;
	ps[1][0] = -rect->w / 2 + cx, ps[1][1] =  rect->h / 2 + cy;
	ps[2][0] =  rect->w / 2 + cx, ps[2][1] =  rect->h / 2 + cy;
	ps[3][0] =  rect->w / 2 + cx, ps[3][1] = -rect->h / 2 + cy;
	PIF_applyTransformMatrix2x2(&ps[0][0], &ps[0][1], mat);
	PIF_applyTransformMatrix2x2(&ps[1][0], &ps[1][1], mat);
	PIF_applyTransformMatrix2x2(&ps[2][0], &ps[2][1], mat);
	PIF_applyTransformMatrix2x2(&ps[3][0], &ps[3][1], mat);
	ps[0][0] += ox; ps[0][1] += oy;
	ps[1][0] += ox; ps[1][1] += oy;
	ps[2][0] += ox; ps[2][1] += oy;
	ps[3][0] += ox; ps[3][1] += oy;
}

static void PIF_makeRotationMatrix2x2(float angle, float mat[2][2]) {
	float angleCos = cos(angle), angleSin = sin(angle);
	mat[0][0] =  angleCos;
	mat[1][0] =  angleSin;
	mat[0][1] = -angleSin;
	mat[1][1] =  angleCos;
}

static uint16_t PIF_bytesToU16(uint8_t *input) {
	return ((uint16_t)input[0]) |
	       ((uint16_t)input[1] << 8);
}

static void PIF_u16ToBytes(uint16_t input, uint8_t *output) {
	output[0] =  input & 0x00FF;
	output[1] = (input & 0xFF00) >> 8;
}

static uint32_t PIF_bytesToU32(uint8_t *input) {
	return ((uint32_t)input[0])       |
	       ((uint32_t)input[1] << 8)  |
	       ((uint32_t)input[2] << 16) |
	       ((uint32_t)input[3] << 24);
}

static void PIF_u32ToBytes(uint32_t input, uint8_t *output) {
	output[0] =  input & 0x000000FF;
	output[1] = (input & 0x0000FF00) >> 8;
	output[2] = (input & 0x00FF0000) >> 16;
	output[3] = (input & 0xFF000000) >> 24;
}

static int PIF_read16(FILE *file, uint16_t *output) {
	uint8_t bytes[2];
	if (fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes))
		return -1;

	*output = PIF_bytesToU16(bytes);
	return 0;
}

static void PIF_write16(FILE *file, uint16_t input) {
	uint8_t bytes[4];
	PIF_u16ToBytes(input, bytes);
	fwrite(bytes, 1, sizeof(bytes), file);
}

static float PIF_lerp(float a, float b, float f) {
    return a + f * (b - a); /* Fast lerp */
}

PIF_DEF uint8_t PIF_shadeColor(uint8_t color, float shade, PIF_Image *colormap) {
	PIF_assert(colormap->h > colormap->w);
	PIF_assert(color       < colormap->w);

	if (shade > 1) shade = 1;
	if (shade < 0) shade = 0;

	int shades = colormap->h - colormap->w;
	return *PIF_imageAt(colormap, color, shade * shades);
}

PIF_DEF uint8_t PIF_blendColor(uint8_t from, uint8_t to, PIF_Image *colormap) {
	PIF_assert(colormap->h > colormap->w);
	PIF_assert(from        < colormap->w);
	PIF_assert(to          < colormap->w);

	if      (from == PIF_TRANSPARENT) return to;
	else if (to   == PIF_TRANSPARENT) return from;

	int shades = colormap->h - colormap->w;
	return *PIF_imageAt(colormap, from, to + shades);
}

PIF_DEF int PIF_rgbDiff(PIF_Rgb a, PIF_Rgb b) {
	int dr = (int)a.r - b.r;
	int dg = (int)a.g - b.g;
	int db = (int)a.b - b.b;
	return dr * dr + dg * dg + db * db;
}

PIF_DEF PIF_Rgb PIF_rgbLerp(PIF_Rgb a, PIF_Rgb b, float t) {
	PIF_Rgb rgb;
	rgb.r = PIF_lerp(a.r, b.r, t);
	rgb.g = PIF_lerp(a.g, b.g, t);
	rgb.b = PIF_lerp(a.b, b.b, t);
	return rgb;
}

PIF_DEF uint8_t PIF_rgbToColor(PIF_Rgb rgb, PIF_Image *rgbmap) {
	PIF_assert(rgbmap->h == rgbmap->w * rgbmap->w);

	rgb.r = (float)rgb.r / 255 * (rgbmap->w - 1);
	rgb.g = (float)rgb.g / 255 * (rgbmap->w - 1);
	rgb.b = (float)rgb.b / 255 * (rgbmap->w - 1);
	return *PIF_imageAt(rgbmap, rgb.r, (int)rgb.g + (int)rgb.b * rgbmap->w);
}

PIF_DEF uint32_t PIF_rgbToPixelRgba32(PIF_Rgb self) {
	uint8_t bytes[4] = {
		255,
		self.b,
		self.g,
		self.r,
	};
	return PIF_bytesToU32(bytes);
}

PIF_DEF uint32_t PIF_rgbToPixelAbgr32(PIF_Rgb self) {
	uint8_t bytes[4] = {
		self.r,
		self.g,
		self.b,
		255,
	};
	return PIF_bytesToU32(bytes);
}

PIF_DEF PIF_Rgb PIF_rgbFromPixelRgba32(uint32_t pixel) {
	uint8_t bytes[4];
	PIF_u32ToBytes(pixel, bytes);

	PIF_Rgb rgb;
	rgb.r = bytes[3];
	rgb.g = bytes[2];
	rgb.b = bytes[1];
	return rgb;
}


PIF_DEF PIF_Rgb PIF_rgbFromPixelAbgr32(uint32_t pixel) {
	uint8_t bytes[4];
	PIF_u32ToBytes(pixel, bytes);

	PIF_Rgb rgb;
	rgb.r = bytes[0];
	rgb.g = bytes[1];
	rgb.b = bytes[2];
	return rgb;
}

PIF_DEF PIF_Palette *PIF_paletteNew(int size) {
	PIF_assert(size <= PIF_COLORS);

	int          cap  = sizeof(PIF_Palette) + (size - 1) * sizeof(PIF_Rgb);
	PIF_Palette *self = (PIF_Palette*)PIF_alloc(cap);
	PIF_checkAlloc(self);

	memset(self, 0, cap);
	self->size = size;
	return self;
}

PIF_DEF PIF_Palette *PIF_paletteRead(FILE *file, const char **err) {
	PIF_assert(file != NULL);

	/* Verify magic bytes */
	char magic[sizeof(PIF_PALETTE_MAGIC) - 1];
	if (fread(magic, 1, sizeof(magic), file) != sizeof(magic))
		return (PIF_Palette*)PIF_error(err, "Failed to read magic bytes");

	if (strncmp(magic, PIF_PALETTE_MAGIC, sizeof(magic)) != 0)
		return (PIF_Palette*)PIF_error(err, "File is not a PIF palette");

	/* Read header */
	int maxColor = fgetc(file);
	if (maxColor == EOF)
		return (PIF_Palette*)PIF_error(err, "Failed to read PIF palette maximum color index");

	PIF_Palette *self = PIF_paletteNew(maxColor + 1);
	PIF_assert(self != NULL);

	/* Read body */
	for (int i = 0; i < self->size; ++ i) {
		uint8_t bytes[3];
		if (fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) {
			PIF_paletteFree(self);
			return (PIF_Palette*)PIF_error(err, "Failed to read PIF palette body");
		}

		self->map[i].r = bytes[0];
		self->map[i].g = bytes[1];
		self->map[i].b = bytes[2];
	}
	return self;
}

PIF_DEF PIF_Palette *PIF_paletteLoad(const char *path, const char **err) {
	PIF_assert(path != NULL);

	FILE *file = fopen(path, "rb");
	if (file == NULL)
		return (PIF_Palette*)PIF_error(err, "Could not open file");

	PIF_Palette *self = PIF_paletteRead(file, err);

	fclose(file);
	return self;
}

PIF_DEF void PIF_paletteWrite(PIF_Palette *self, FILE *file) {
	PIF_assert(self != NULL);
	PIF_assert(file != NULL);

	/* Write magic bytes */
	fwrite(PIF_PALETTE_MAGIC, 1, sizeof(PIF_PALETTE_MAGIC) - 1, file);

	/* Write header */
	fputc(self->size - 1, file);

	/* Write body */
	for (int i = 0; i < self->size; ++ i) {
		uint8_t bytes[3] = {
			self->map[i].r,
			self->map[i].g,
			self->map[i].b,
		};
		fwrite(bytes, 1, sizeof(bytes), file);
	}
}

PIF_DEF int PIF_paletteSave(PIF_Palette *self, const char *path) {
	PIF_assert(self != NULL);
	PIF_assert(path != NULL);

	FILE *file = fopen(path, "wb");
	if (file == NULL)
		return -1;

	PIF_paletteWrite(self, file);

	fclose(file);
	return 0;
}

PIF_DEF void PIF_paletteFree(PIF_Palette *self) {
	PIF_assert(self != NULL);

	PIF_free(self);
}

PIF_DEF uint8_t PIF_paletteClosest(PIF_Palette *self, PIF_Rgb rgb) {
	PIF_assert(self != NULL);

	uint8_t color   =  0;
	int     minDiff = -1;
	for (int i = 0; i < self->size; ++ i) {
		if (i == PIF_TRANSPARENT)
			continue;

		int diff = PIF_rgbDiff(rgb, self->map[i]);
		if (minDiff == -1 || diff < minDiff) {
			minDiff = diff;
			color   = i;
		}
	}
	return color;
}

PIF_DEF PIF_Image *PIF_paletteCreateColormap(PIF_Palette *self, int shades, float t) {
	PIF_assert(self != NULL);

	PIF_Image *colormap = PIF_imageNew(self->size, self->size + shades);
	for (int x = 0; x < self->size; ++ x) {
		for (int y = 0; y < shades; ++ y) {
			uint8_t color;
			if (x == PIF_TRANSPARENT)
				color = PIF_TRANSPARENT;
			else {
				float darken = (float)(shades - y) / (float)(shades / 2);

				int r = (int)self->map[x].r * darken;
				int g = (int)self->map[x].g * darken;
				int b = (int)self->map[x].b * darken;

				if (r > 255) r = 255;
				if (g > 255) g = 255;
				if (b > 255) b = 255;

				PIF_Rgb rgb;
				rgb.r = r;
				rgb.g = g;
				rgb.b = b;
				color = PIF_paletteClosest(self, rgb);
			}

			*PIF_imageAt(colormap, x, y) = color;
		}
	}

	for (int y = 0; y < self->size; ++ y) {
		for (int x = 0; x < self->size; ++ x) {
			uint8_t color;
			if (x == PIF_TRANSPARENT || y == PIF_TRANSPARENT)
				color = PIF_TRANSPARENT;
			else
				color = PIF_paletteClosest(self, PIF_rgbLerp(self->map[x], self->map[y], t));

			*PIF_imageAt(colormap, x, y + shades) = color;
		}
	}
	return colormap;
}

PIF_DEF PIF_Image *PIF_paletteCreateRgbmap(PIF_Palette *self, uint8_t size) {
	PIF_assert(self != NULL);

	PIF_Image *rgbmap = PIF_imageNew(size, (int)size * size);
	for (int r = 0; r < size; ++ r) {
		for (int g = 0; g < size; ++ g) {
			for (int b = 0; b < size; ++ b) {
				PIF_Rgb rgb;
				rgb.r = (float)r / (size - 1) * 255;
				rgb.g = (float)g / (size - 1) * 255;
				rgb.b = (float)b / (size - 1) * 255;
				*PIF_imageAt(rgbmap, r, g + b * size) = PIF_paletteClosest(self, rgb);
			}
		}
	}
	return rgbmap;
}

PIF_DEF void PIF_blendShader(int x, int y, uint8_t *pixel, uint8_t color, PIF_Image *img) {
	(void)x; (void)y;
	PIF_Image *colormap = (PIF_Image*)img->data;
	PIF_assert(colormap != NULL);

	*pixel = PIF_blendColor(*pixel, color, colormap);
}

PIF_DEF void PIF_ditherShader(int x, int y, uint8_t *pixel, uint8_t color, PIF_Image *img) {
	(void)img;

	if (x % 2 == (y % 2 == 0))
		*pixel = color;
}

PIF_DEF void PIF_copyShader(int x, int y, uint8_t *pixel, uint8_t color, PIF_Image *img) {
	(void)color;
	PIF_Image *src = (PIF_Image*)img->data;
	PIF_assert(src != NULL);

	*pixel = *PIF_imageAt(src, (float)x / img->w * src->w, (float)y / img->h * src->h);
}

PIF_DEF void PIF_exCopyShader(int x, int y, uint8_t *pixel, uint8_t color, PIF_Image *img) {
	(void)color;
	PIF_CopyInfo *info = (PIF_CopyInfo*)img->data;
	PIF_assert(info != NULL);

	int xSrc = x - (info->destRect.x + info->destRect.w / 2);
	int ySrc = y - (info->destRect.y + info->destRect.h / 2);
	if (info->transform) {
		xSrc -= info->cx;
		ySrc -= info->cy;
		PIF_applyTransformMatrix2x2(&xSrc, &ySrc, info->mat);
		xSrc += info->cx;
		ySrc += info->cy;
	}
	xSrc = (float)(xSrc + info->ox) / info->destRect.w * info->srcRect.w + info->srcRect.w / 2;
	ySrc = (float)(ySrc + info->oy) / info->destRect.h * info->srcRect.h + info->srcRect.h / 2;

	uint8_t *src = PIF_imageAt(info->src, xSrc + info->srcRect.x, ySrc + info->srcRect.y);
	if (src != NULL)
		*pixel = *src;
}

PIF_DEF PIF_Image *PIF_imageNew(int w, int h) {
	int        cap  = sizeof(PIF_Image) + w * h - 1;
	PIF_Image *self = (PIF_Image*)PIF_alloc(cap);
	PIF_checkAlloc(self);

	memset(self, 0, cap);
	self->w    = w;
	self->h    = h;
	self->size = w * h;
	self->skipTransparent = true;
	return self;
}

PIF_DEF PIF_Image *PIF_imageRead(FILE *file, const char **err) {
	PIF_assert(file != NULL);

	/* Verify magic bytes */
	char magic[sizeof(PIF_IMAGE_MAGIC) - 1];
	if (fread(magic, 1, sizeof(magic), file) != sizeof(magic))
		return (PIF_Image*)PIF_error(err, "Failed to read magic bytes");

	if (strncmp(magic, PIF_IMAGE_MAGIC, sizeof(magic)) != 0)
		return (PIF_Image*)PIF_error(err, "File is not a PIF image");

	/* Read header */
	uint16_t w, h;
	if (PIF_read16(file, &w) != 0 || PIF_read16(file, &h) != 0)
		return (PIF_Image*)PIF_error(err, "Failed to read PIF image size");

	PIF_Image *self = PIF_imageNew(w, h);

	/* Read body */
	if (fread(self->buf, 1, self->size, file) != (size_t)self->size) {
		PIF_imageFree(self);
		return (PIF_Image*)PIF_error(err, "Failed to read PIF image body");
	}
	return self;
}

PIF_DEF PIF_Image *PIF_imageLoad(const char *path, const char **err) {
	PIF_assert(path != NULL);

	FILE *file = fopen(path, "rb");
	if (file == NULL)
		return (PIF_Image*)PIF_error(err, "Could not open file");

	PIF_Image *self = PIF_imageRead(file, err);

	fclose(file);
	return self;
}

PIF_DEF void PIF_imageWrite(PIF_Image *self, FILE *file) {
	PIF_assert(self != NULL);
	PIF_assert(file != NULL);

	/* Write magic bytes */
	fwrite(PIF_IMAGE_MAGIC, 1, sizeof(PIF_IMAGE_MAGIC) - 1, file);

	/* Write header */
	PIF_assert(self->w <= USHRT_MAX && self->h <= USHRT_MAX);
	PIF_write16(file, self->w);
	PIF_write16(file, self->h);

	/* Write body */
	fwrite(self->buf, 1, self->size, file);
}

PIF_DEF int PIF_imageSave(PIF_Image *self, const char *path) {
	PIF_assert(self != NULL);
	PIF_assert(path != NULL);

	FILE *file = fopen(path, "wb");
	if (file == NULL)
		return -1;

	PIF_imageWrite(self, file);

	fclose(file);
	return 0;
}

PIF_DEF void PIF_imageFree(PIF_Image *self) {
	PIF_assert(self != NULL);

	PIF_free(self);
}

PIF_DEF void PIF_imageSkipTransparent(PIF_Image *self, bool enable) {
	PIF_assert(self != NULL);

	self->skipTransparent = enable;
}

PIF_DEF void PIF_imageSetShader(PIF_Image *self, PIF_Shader shader, void *data) {
	PIF_assert(self != NULL);

	self->shader = shader;
	self->data   = data;
}

PIF_DEF void PIF_imageSetShaderData(PIF_Image *self, void *data) {
	PIF_assert(self != NULL);

	self->data = data;
}

PIF_DEF void PIF_imageConvertPalette(PIF_Image *self, PIF_Palette *from, PIF_Palette *to) {
	PIF_assert(self != NULL);
	PIF_assert(from != NULL);
	PIF_assert(to   != NULL);

	for (int y = 0; y < self->h; ++ y) {
		for (int x = 0; x < self->w; ++ x) {
			uint8_t *pixel = PIF_imageAt(self, x, y);
			*pixel = PIF_paletteClosest(to, from->map[*pixel]);
		}
	}
}

PIF_DEF uint8_t *PIF_imageAt(PIF_Image *self, int x, int y) {
	PIF_assert(self != NULL);

	if (x < 0 || x >= self->w || y < 0 || y >= self->h)
		return NULL;

	return self->buf + self->w * y + x;
}

PIF_DEF PIF_Image *PIF_imageResize(PIF_Image *self, int w, int h, uint8_t color) {
	if (self->w == w && self->h == h)
		return self;

	uint8_t *prevBuf = (uint8_t*)PIF_alloc(self->size);
	PIF_checkAlloc(prevBuf);
	memcpy(prevBuf, self->buf, self->size);

	int prevW = self->w, prevH = self->h;

	self->w    = w;
	self->h    = h;
	self->size = w * h;
	self       = (PIF_Image*)PIF_realloc(self, sizeof(PIF_Image) + self->size - 1);
	PIF_checkAlloc(self);

	memset(self->buf, color, self->size);

	w = PIF_min(prevW, self->w);
	h = PIF_min(prevH, self->h);
	for (int y = 0; y < h; ++ y) {
		for (int x = 0; x < w; ++ x)
			*PIF_imageAt(self, x, y) = prevBuf[prevW * y + x];
	}

	PIF_free(prevBuf);
	return self;
}

PIF_DEF void PIF_imageClear(PIF_Image *self, uint8_t color) {
	PIF_assert(self != NULL);

	memset(self->buf, color, self->size);
}

PIF_DEF PIF_Image *PIF_imageCopy(PIF_Image *self, PIF_Image *from) {
	self->w    = from->w;
	self->h    = from->h;
	self->size = from->w * from->h;
	self       = (PIF_Image*)PIF_realloc(self, sizeof(PIF_Image) + self->size - 1);
	PIF_checkAlloc(self);

	memcpy(self->buf, from->buf, self->size);
	return self;
}

PIF_DEF PIF_Image *PIF_imageDup(PIF_Image *self) {
	int        cap   = sizeof(PIF_Image) + self->size - 1;
	PIF_Image *duped = (PIF_Image*)PIF_alloc(cap);
	memcpy(duped, self, cap);
	return duped;
}

PIF_DEF void PIF_imageBlit(PIF_Image *self, PIF_Rect *destRect, PIF_Image *src, PIF_Rect *srcRect) {
	PIF_assert(self != NULL);
	PIF_assert(src  != NULL);

	PIF_Rect destRect_ = {0, 0, self->w, self->h};
	PIF_Rect srcRect_  = {0, 0, src->w,  src->h};
	if (srcRect  == NULL) srcRect  = &srcRect_;
	if (destRect == NULL) destRect = &destRect_;

	float scaleX = (float)srcRect->w / destRect->w;
	float scaleY = (float)srcRect->h / destRect->h;

	for (int y = 0; y < destRect->h; ++ y) {
		int destY = destRect->y + y;
		if (destY <  0)       continue;
		if (destY >= self->h) break;

		for (int x = 0; x < destRect->w; ++ x) {
			int destX = destRect->x + x;
			if (destX <  0)       continue;
			if (destX >= self->w) break;

			uint8_t *color = PIF_imageAt(src, scaleX * x + srcRect->x, scaleY * y + srcRect->y);
			PIF_assert(color != NULL);

			if (*color == PIF_TRANSPARENT && self->skipTransparent)
				continue;

			PIF_imageDrawPoint(self, destX, destY, *color);
		}
	}
}

PIF_DEF void PIF_imageTransformBlit(PIF_Image *self, PIF_Rect *destRect, PIF_Image *src,
                                    PIF_Rect *srcRect, float mat[2][2], int cx, int cy) {
	PIF_assert(self != NULL);
	PIF_assert(src  != NULL);

	PIF_Rect destRect_ = {0, 0, self->w, self->h};
	PIF_Rect srcRect_  = {0, 0, src->w,  src->h};
	if (srcRect  == NULL) srcRect  = &srcRect_;
	if (destRect == NULL) destRect = &destRect_;

	PIF_CopyInfo copyInfo;
	PIF_zeroStruct(&copyInfo);
	copyInfo.src       = src;
	copyInfo.srcRect   = *srcRect;
	copyInfo.destRect  = *destRect;
	copyInfo.ox        = -cx;
	copyInfo.oy        = -cy;
	copyInfo.transform = true;
	PIF_invertMatrix2x2(mat, copyInfo.mat);

	PIF_Shader prevShader = self->shader;
	void      *prevData   = self->data;
	PIF_imageSetShader(self, PIF_exCopyShader, &copyInfo);
	PIF_imageFillTransformRect(self, destRect, 1, mat, cx, cy);
	PIF_imageSetShader(self, prevShader, prevData);
}

PIF_DEF void PIF_imageRotateBlit(PIF_Image *self, PIF_Rect *destRect, PIF_Image *src,
                                 PIF_Rect *srcRect, float angle, int cx, int cy) {
	float rotMat[2][2];
	PIF_makeRotationMatrix2x2(angle, rotMat);
	PIF_imageTransformBlit(self, destRect, src, srcRect, rotMat, cx, cy);
}

PIF_DEF void PIF_imageDrawPoint(PIF_Image *self, int x, int y, uint8_t color) {
	PIF_assert(self != NULL);

	if (color == PIF_TRANSPARENT && self->skipTransparent)
		return;

	uint8_t *pixel = PIF_imageAt(self, x, y);
	if (pixel == NULL)
		return;

	if (self->shader == NULL) {
		*pixel = color;
		return;
	}

	self->shader(x, y, pixel, color, self);
}

/* Bresenham's line algorithm */
PIF_DEF void PIF_imageDrawLine(PIF_Image *self, int x1, int y1, int x2, int y2,
                               int n, uint8_t color) {
	PIF_assert(self != NULL);

	if (color == PIF_TRANSPARENT && self->skipTransparent)
		return;

	bool swap = abs(y2 - y1) > abs(x2 - x1);
	if (swap) {
		PIF_swap(x1, y1);
		PIF_swap(x2, y2);
	}

	if (x1 > x2) {
		PIF_swap(x1, x2);
		PIF_swap(y1, y2);
	}

	int   distX = x2 - x1;
	int   distY = abs(y2 - y1);
	float err   = distX / 2;
	int   stepY = y1 < y2? 1 : -1;

	bool draw = true;
	for (int i = 0; x1 <= x2; ++ i) {
		if (n > 0 && i % n == 0)
			draw = !draw;

		if (draw) {
			int x = x1, y = y1;
			if (swap)
				PIF_swap(x, y);

			PIF_imageDrawPoint(self, x, y, color);
		}

		err -= distY;
		if (err < 0) {
			y1  += stepY;
			err += distX;
		}
		++ x1;
	}
}

PIF_DEF void PIF_imageDrawRect(PIF_Image *self, PIF_Rect *rect, int n, uint8_t color) {
	PIF_assert(self != NULL);

	if (color == PIF_TRANSPARENT && self->skipTransparent)
		return;

	PIF_Rect rect_ = {0, 0, self->w, self->h};
	if (rect == NULL)
		rect = &rect_;

	PIF_imageDrawLine(self, rect->x, rect->y, rect->x + rect->w - 2, rect->y, n, color);
	PIF_imageDrawLine(self, rect->x, rect->y + rect->h - 1, rect->x, rect->y + 1, n, color);
	PIF_imageDrawLine(self, rect->x + rect->w - 1, rect->y,
	                  rect->x + rect->w - 1, rect->y + rect->h - 2, n, color);
	PIF_imageDrawLine(self, rect->x + rect->w - 1, rect->y + rect->h - 1,
	                  rect->x + 1, rect->y + rect->h - 1, n, color);
}

/* Midpoint circle algorithm */
PIF_DEF void PIF_imageDrawCircle(PIF_Image *self, int cx, int cy, int r, int n, uint8_t color) {
	PIF_assert(self != NULL);

	if (color == PIF_TRANSPARENT && self->skipTransparent)
		return;

	int x   = r - 1;
	int y   = 0;
	int dx  = 1;
	int dy  = 1;
	int err = dx - (r << 1);

	bool draw = true;
	for (int i = 0; x >= y; ++ i) {
		if (n > 0 && i % n == 0)
			draw = !draw;

		if (draw) {
			/* TODO: Fix overdrawing pixels */
			PIF_imageDrawPoint(self, cx + x, cy + y, color);
			PIF_imageDrawPoint(self, cx + y, cy + x, color);
			PIF_imageDrawPoint(self, cx - y, cy + x, color);
			PIF_imageDrawPoint(self, cx - x, cy + y, color);
			PIF_imageDrawPoint(self, cx - x, cy - y, color);
			PIF_imageDrawPoint(self, cx - y, cy - x, color);
			PIF_imageDrawPoint(self, cx + y, cy - x, color);
			PIF_imageDrawPoint(self, cx + x, cy - y, color);
		}

		if (err <= 0) {
			y   ++;
			err += dy;
			dy  += 2;
		}

		if (err > 0) {
			x   --;
			dx  += 2;
			err += dx - (r << 1);
		}
	}
}

PIF_DEF void PIF_imageDrawTriangle(PIF_Image *self, int x1, int y1, int x2, int y2,
                                   int x3, int y3, int n, uint8_t color) {
	PIF_assert(self != NULL);

	if (color == PIF_TRANSPARENT && self->skipTransparent)
		return;

	PIF_imageDrawLine(self, x1, y1, x2, y2, n, color);
	PIF_imageDrawLine(self, x2, y2, x3, y3, n, color);
	PIF_imageDrawLine(self, x3, y3, x1, y1, n, color);
}

PIF_DEF void PIF_imageDrawTransformRect(PIF_Image *self, PIF_Rect *rect, int n, uint8_t color,
                                        float mat[2][2], int cx, int cy) {
	PIF_assert(self != NULL);

	if (color == PIF_TRANSPARENT && self->skipTransparent)
		return;

	PIF_Rect rect_ = {0, 0, self->w, self->h};
	if (rect == NULL)
		rect = &rect_;

	int ps[4][2];
	PIF_transformRect(rect, mat, cx, cy, ps);

	PIF_imageDrawLine(self, ps[0][0], ps[0][1], ps[1][0], ps[1][1], n, color);
	PIF_imageDrawLine(self, ps[1][0], ps[1][1], ps[2][0], ps[2][1], n, color);
	PIF_imageDrawLine(self, ps[2][0], ps[2][1], ps[3][0], ps[3][1], n, color);
	PIF_imageDrawLine(self, ps[3][0], ps[3][1], ps[0][0], ps[0][1], n, color);
}

PIF_DEF void PIF_imageDrawRotateRect(PIF_Image *self, PIF_Rect *rect, int n, uint8_t color,
                                     float angle, int cx, int cy) {
	float rotMat[2][2];
	PIF_makeRotationMatrix2x2(angle, rotMat);
	PIF_imageDrawTransformRect(self, rect, n, color, rotMat, cx, cy);
}

PIF_DEF void PIF_imageFillRect(PIF_Image *self, PIF_Rect *rect, uint8_t color) {
	PIF_assert(self != NULL);

	if (color == PIF_TRANSPARENT && self->skipTransparent)
		return;

	PIF_Rect rect_ = {0, 0, self->w, self->h};
	if (rect == NULL)
		rect = &rect_;

	for (int y = rect->y; y < rect->y + rect->h; ++ y) {
		if (y <  0)       continue;
		if (y >= self->h) break;

		for (int x = rect->x; x < rect->x + rect->w; ++ x) {
			if (x <  0)       continue;
			if (x >= self->w) break;

			PIF_imageDrawPoint(self, x, y, color);
		}
	}
}

PIF_DEF void PIF_imageFillCircle(PIF_Image *self, int cx, int cy, int r, uint8_t color) {
	PIF_assert(self != NULL);

	if (color == PIF_TRANSPARENT && self->skipTransparent)
		return;

	int d  = r * 2;
	int rr = r * r;
	for (int y = 0; y < d; ++ y) {
		int dy    = r - y;
		int destY = cy + dy;
		if (destY <  0)       break;
		if (destY >= self->h) continue;

		for (int x = 0; x < d; ++ x) {
			int dx    = r - x;
			int destX = cx + dx;
			if (destX <  0)       break;
			if (destX >= self->w) continue;

			int dy = r - y;
			if (dx * dx + dy * dy < rr)
				PIF_imageDrawPoint(self, destX, destY, color);
		}
	}
}

static void PIF_imageFillFlatSideTriangle(PIF_Image *self, int x1, int y1, int x2, int y2,
                                          int x3, uint8_t color, bool skipLast) {
	if (x2 > x3)
		PIF_swap(x2, x3);

	float slopeA = (float)(x2 - x1) / (y2 - y1);
	float slopeB = (float)(x3 - x1) / (y2 - y1);

	float xStart = x1, xEnd = x1, yStep = 1;
	if (y2 == y1) {
		xStart = PIF_min(xStart, x2);
		xEnd   = PIF_max(xEnd,   x3);
	}

	if (y1 > y2) {
		slopeA *= -1;
		slopeB *= -1;
		yStep  *= -1;
	}

	float xPrevEnd = xStart, xPrevStart = xEnd;
	int   yStop    = skipLast? y2 : y2 + yStep;
	/* skipLast is required to avoid an overlap with the 2 flat side triangles used to render a
	   normal triangle */
	for (int y = y1; y != yStop; y += yStep) {
		if ((yStep > 0 && y >= self->h) || (yStep < 0 && y < 0))
			break;

		/* Fix gaps */
		if      (xPrevEnd   + 1 < xStart) xStart = xPrevEnd   + 1;
		else if (xPrevStart - 1 > xEnd)   xEnd   = xPrevStart - 1;

		/* Scanline */
		if (y >= 0) {
			for (int x = PIF_max(round(xStart), 0); x <= PIF_min(round(xEnd), self->w); ++ x)
				PIF_imageDrawPoint(self, x, y, color);
		}

		/* Step */
		xPrevEnd   = xEnd;
		xPrevStart = xStart;
		xStart    += slopeA;
		xEnd      += slopeB;
	}
}

/* TODO: Make triangle filling more precise? */
PIF_DEF void PIF_imageFillTriangle(PIF_Image *self, int x1, int y1, int x2, int y2,
                                   int x3, int y3, uint8_t color) {
	PIF_assert(self != NULL);

	/* Sort points */
	if (y1 > y3) {
		PIF_swap(x1, x3);
		PIF_swap(y1, y3);
	}
	if (y1 > y2) {
		PIF_swap(x1, x2);
		PIF_swap(y1, y2);
	}
	if (y2 > y3) {
		PIF_swap(x2, x3);
		PIF_swap(y2, y3);
	}

	/* Is it a flat-sided triangle? */
	if      (y2 == y3) PIF_imageFillFlatSideTriangle(self, x1, y1, x2, y2, x3, color, false);
	else if (y1 == y2) PIF_imageFillFlatSideTriangle(self, x3, y3, x1, y1, x2, color, false);
	else {
		/* Otherwise split the triangle into 2 flat sided triangles */
		float slope = (float)(x3 - x1) / (y3 - y1);
		int   x4    = round((float)x3 - slope * (y3 - y2));

		PIF_imageFillFlatSideTriangle(self, x1, y1, x2, y2, x4, color, false);
		PIF_imageFillFlatSideTriangle(self, x3, y3, x2, y2, x4, color, true);
	}
}

PIF_DEF void PIF_imageFillTransformRect(PIF_Image *self, PIF_Rect *rect, uint8_t color,
                                        float mat[2][2], int cx, int cy) {
	PIF_assert(self != NULL);

	if (color == PIF_TRANSPARENT && self->skipTransparent)
		return;

	PIF_Rect rect_ = {0, 0, self->w, self->h};
	if (rect == NULL)
		rect = &rect_;

	int ps[4][2];
	PIF_transformRect(rect, mat, cx, cy, ps);

	PIF_imageFillTriangle(self, ps[0][0], ps[0][1], ps[1][0], ps[1][1], ps[3][0], ps[3][1], color);
	PIF_imageFillTriangle(self, ps[2][0], ps[2][1], ps[1][0], ps[1][1], ps[3][0], ps[3][1], color);
}

PIF_DEF void PIF_imageFillRotateRect(PIF_Image *self, PIF_Rect *rect, uint8_t color,
                                     float angle, int cx, int cy) {
	float rotMat[2][2];
	PIF_makeRotationMatrix2x2(angle, rotMat);
	PIF_imageFillTransformRect(self, rect, color, rotMat, cx, cy);
}

PIF_DEF PIF_Font *PIF_fontNew(int chHeight, uint8_t *chWidths, PIF_Image *sheet,
                              uint8_t chSpacing, uint8_t lineSpacing) {
	PIF_assert(chWidths != NULL);
	PIF_assert(sheet    != NULL);

	PIF_Font *self = (PIF_Font*)PIF_alloc(sizeof(PIF_Font));
	PIF_checkAlloc(self);

	/* Calculate character positions */
	int x = 0, y = 0;
	for (int i = 0; i < 256; ++ i) {
		int w = chWidths[i];
		if (x + w > sheet->w) {
			x  = 0;
			y += chHeight;
		}

		self->chInfo[i].x = x;
		self->chInfo[i].y = y;
		self->chInfo[i].w = w;
		x += w;
	}

	self->chHeight    = chHeight;
	self->sheet       = sheet;
	self->chSpacing   = chSpacing;
	self->lineSpacing = lineSpacing;
	self->scale       = 1;
	return self;
}

PIF_DEF PIF_Font *PIF_fontRead(FILE *file, const char **err) {
	PIF_assert(file != NULL);

	/* Verify magic bytes */
	char magic[sizeof(PIF_FONT_MAGIC) - 1];
	if (fread(magic, 1, sizeof(magic), file) != sizeof(magic))
		return (PIF_Font*)PIF_error(err, "Failed to read magic bytes");

	if (strncmp(magic, PIF_FONT_MAGIC, sizeof(magic)) != 0)
		return (PIF_Font*)PIF_error(err, "File is not a PIF font");

	/* Read spacing */
	int chSpacing = fgetc(file);
	if (chSpacing == EOF)
		return (PIF_Font*)PIF_error(err, "Failed to read PIF font character spacing");

	int lineSpacing = fgetc(file);
	if (lineSpacing == EOF)
		return (PIF_Font*)PIF_error(err, "Failed to read PIF font line spacing");

	/* Read character height */
	int chHeight = fgetc(file);
	if (chHeight == EOF)
		return (PIF_Font*)PIF_error(err, "Failed to read PIF font character height");

	/* Read character widths */
	uint8_t chWidths[256];
	if (fread(chWidths, 1, sizeof(chWidths), file) != sizeof(chWidths))
		return (PIF_Font*)PIF_error(err, "Failed to read PIF font character widths");

	/* Read sheet */
	PIF_Image *sheet = PIF_imageRead(file, err);
	if (sheet == NULL)
		return NULL;

	/* Create font */
	PIF_Font *self = PIF_fontNew(chHeight, chWidths, sheet, chSpacing, lineSpacing);
	PIF_assert(self != NULL);
	return self;
}

PIF_DEF PIF_Font *PIF_fontLoad(const char *path, const char **err) {
	PIF_assert(path != NULL);

	FILE *file = fopen(path, "rb");
	if (file == NULL)
		return (PIF_Font*)PIF_error(err, "Could not open file");

	PIF_Font *self = PIF_fontRead(file, err);

	fclose(file);
	return self;
}

PIF_DEF void PIF_fontWrite(PIF_Font *self, FILE *file) {
	PIF_assert(self        != NULL);
	PIF_assert(file        != NULL);
	PIF_assert(self->sheet != NULL);

	/* Write magic bytes */
	fwrite(PIF_FONT_MAGIC, 1, sizeof(PIF_FONT_MAGIC) - 1, file);

	/* Write spacing */
	fputc(self->chSpacing,   file);
	fputc(self->lineSpacing, file);

	/* Write character height */
	fputc(self->chHeight, file);

	/* Write character widths */
	for (int i = 0; i < 256; ++ i)
		fputc(self->chInfo[i].w, file);

	/* Write sheet */
	PIF_imageWrite(self->sheet, file);
}

PIF_DEF int PIF_fontSave(PIF_Font *self, const char *path) {
	PIF_assert(self != NULL);
	PIF_assert(path != NULL);

	FILE *file = fopen(path, "wb");
	if (file == NULL)
		return -1;

	PIF_fontWrite(self, file);

	fclose(file);
	return 0;
}

PIF_DEF void PIF_fontFree(PIF_Font *self) {
	PIF_assert(self != NULL);

	PIF_imageFree(self->sheet);
	PIF_free(self);
}

PIF_DEF void PIF_fontSetSpacing(PIF_Font *self, uint8_t chSpacing, uint8_t lineSpacing) {
	PIF_assert(self != NULL);

	self->chSpacing   = chSpacing;
	self->lineSpacing = lineSpacing;
}

PIF_DEF void PIF_fontSetScale(PIF_Font *self, float scale) {
	PIF_assert(self != NULL);

	self->scale = scale;
}

PIF_DEF void PIF_fontCharSize(PIF_Font *self, char ch, int *w, int *h) {
	PIF_assert(self != NULL);

	if (w != NULL) *w = round((float)self->chInfo[(int)ch].w * self->scale);
	if (h != NULL) *h = round((float)self->chHeight          * self->scale);
}

PIF_DEF void PIF_fontTextSize(PIF_Font *self, const char *text, int *w, int *h) {
	PIF_assert(self != NULL);
	PIF_assert(text != NULL);

	int w_, h_;
	if (w == NULL) w = &w_;
	if (h == NULL) h = &h_;

	*w = 0;
	*h = round((float)self->chHeight * self->scale);
	for (int rowWidth = 0; *text != '\0'; ++ text) {
		if (*text == '\n') {
			rowWidth = 0;
			*h += round((float)(self->chHeight + self->lineSpacing) * self->scale);
			continue;
		}

		int space = rowWidth > 0? self->chSpacing : 0;
		rowWidth += round((float)(self->chInfo[(int)*text].w + space) * self->scale);
		if (rowWidth > *w)
			*w = rowWidth;
	}
}

PIF_DEF void PIF_fontRenderChar(PIF_Font *self, char ch, PIF_Image *img,
                                int xStart, int yStart, uint8_t color) {
	PIF_assert(self != NULL);
	PIF_assert(img  != NULL);
	PIF_assert(ch   != '\0');

	PIF_FontCharInfo chInfo = self->chInfo[(int)ch];
	if (chInfo.w == 0)
		return;

	for (int y = 0; y < round((float)self->chHeight * self->scale); ++ y) {
		int srcY  = chInfo.y + (float)y / self->scale;
		int destY = yStart + y;
		if (destY <  0)      continue;
		if (destY >= img->h) break;

		for (int x = 0; x < round((float)chInfo.w * self->scale); ++ x) {
			int srcX = chInfo.x + (float)x / self->scale;

			uint8_t *pixel = PIF_imageAt(self->sheet, srcX, srcY);
			PIF_assert(pixel != NULL);

			if (*pixel == PIF_TRANSPARENT)
				continue;

			int destX = xStart + x;
			if (destX <  0)      continue;
			if (destX >= img->w) break;

			PIF_imageDrawPoint(img, destX, destY, color == PIF_TRANSPARENT? *pixel : color);
		}
	}
}

PIF_DEF void PIF_fontRenderText(PIF_Font *self, const char *text, PIF_Image *img,
                                int xStart, int yStart, uint8_t color) {
	PIF_assert(self != NULL);
	PIF_assert(text != NULL);
	PIF_assert(img  != NULL);

	for (int x = xStart, y = yStart; *text != '\0'; ++ text) {
		if (*text == '\n') {
			x  = xStart;
			y += round((float)(self->chHeight + self->lineSpacing) * self->scale);
			continue;
		}

		PIF_fontRenderChar(self, *text, img, x, y, color);
		x += round((float)(self->chInfo[(int)*text].w + self->chSpacing) * self->scale);
	}
}

#define PIF_DEFAULT_FONT_W 31
#define PIF_DEFAULT_FONT_H 60
#define PIF_DEFAULT_FONT_CHAR_H 6

uint8_t PIF_defaultFontWidths[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 1, 3, 5, 3, 3, 4, 1, 2, 2, 3, 3, 1, 2, 1, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 2, 2, 2, 3,
	3, 3, 3, 3, 3, 3, 3, 4, 3, 1, 3, 3, 3, 5, 4, 3, 3, 3, 3, 3, 3, 3, 3, 5, 3, 3, 3, 2, 3, 2, 3, 2,
	2, 3, 3, 3, 3, 3, 3, 4, 3, 1, 3, 3, 3, 5, 4, 3, 3, 3, 3, 3, 3, 3, 3, 5, 3, 3, 3, 3, 1, 3, 4,
};

/* Yes, i wrote all of this out manually (not joking) */
uint8_t PIF_defaultFontPixels[PIF_DEFAULT_FONT_H][PIF_DEFAULT_FONT_W] = {
	{0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0},
	{0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0},
	{0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0},
	{0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0},
	{1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0},
	{0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0},
	{0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0},
	{0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

	{1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
	{0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0},
	{0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
	{0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0},
	{0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

	{1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0},
	{1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0},
	{1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0},
	{1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0},
	{1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

	{1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0},
	{1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0},
	{1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0},
	{1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0},
	{1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

	{1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0},
	{1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1},
	{1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
	{1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0},
	{0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

	{1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0},
	{1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0},
	{1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0},
	{1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0},
	{1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

	{1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
	{1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0},
	{1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0},
	{1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0},
	{1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},

	{1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0},
	{0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0},
	{0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0},
	{0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0},
	{0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

PIF_DEF PIF_Font *PIF_fontNewDefault(void) {
	PIF_Image *sheet = PIF_imageNew(PIF_DEFAULT_FONT_W, PIF_DEFAULT_FONT_H);
	for (int y = 0; y < sheet->h; ++ y) {
		for (int x = 0; x < sheet->w; ++ x)
			*PIF_imageAt(sheet, x, y) = PIF_defaultFontPixels[y][x];
	}

	return PIF_fontNew(PIF_DEFAULT_FONT_CHAR_H, PIF_defaultFontWidths, sheet, 1, 1);
}

#undef PIF_DEFAULT_FONT_W
#undef PIF_DEFAULT_FONT_H
#undef PIF_DEFAULT_FONT_CHAR_H

#undef PIF_error
#undef PIF_checkAlloc

#ifdef __cplusplus
}
#endif
