#include "pif.h"

#define PIF_error(ERR, MSG) ((ERR) != NULL? (*(ERR) = MSG, NULL) : NULL)

#define PIF_checkAlloc(PTR)                                                         \
	do {                                                                            \
		if (PTR == NULL) {                                                          \
			fprintf(stderr, "%s:%i: PIF allocation failure\n", __FILE__, __LINE__); \
			abort();                                                                \
		}                                                                           \
	} while (0)

#define PIF_swap(A, B)                      \
	do {                                   \
		PIF_assert(sizeof(A) == sizeof(B)); \
		uint8_t tmp_[sizeof(A)];           \
		memcpy(tmp_, &(A), sizeof(A));     \
		A = B;                             \
		memcpy(&(B), tmp_, sizeof(B));     \
	} while (0)

#define PIF_max(A, B) ((A) > (B)? (A) : (B))
#define PIF_min(A, B) ((A) > (B)? (B) : (A))

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

static int PIF_read32(FILE *file, uint32_t *output) {
	uint8_t bytes[4];
	if (fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes))
		return -1;

	*output = PIF_bytesToU32(bytes);
	return 0;
}

static void PIF_write32(FILE *file, uint32_t input) {
	uint8_t bytes[4];
	PIF_u32ToBytes(input, bytes);
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
	return (PIF_Rgb){
		.r = PIF_lerp(a.r, b.r, t),
		.g = PIF_lerp(a.g, b.g, t),
		.b = PIF_lerp(a.b, b.b, t),
	};
}

PIF_DEF uint32_t PIF_rgbToPixelRgba32(PIF_Rgb this) {
	uint8_t bytes[4] = {
		255,
		this.b,
		this.g,
		this.r,
	};
	return PIF_bytesToU32(bytes);
}

PIF_DEF uint32_t PIF_rgbToPixelAbgr32(PIF_Rgb this) {
	uint8_t bytes[4] = {
		this.r,
		this.g,
		this.b,
		255,
	};
	return PIF_bytesToU32(bytes);
}

PIF_DEF PIF_Rgb PIF_rgbFromPixelRgba32(uint32_t pixel) {
	uint8_t bytes[4];
	PIF_u32ToBytes(pixel, bytes);
	return (PIF_Rgb){
		.r = bytes[3],
		.g = bytes[2],
		.b = bytes[1],
	};
}


PIF_DEF PIF_Rgb PIF_rgbFromPixelAbgr32(uint32_t pixel) {
	uint8_t bytes[4];
	PIF_u32ToBytes(pixel, bytes);
	return (PIF_Rgb){
		.r = bytes[0],
		.g = bytes[1],
		.b = bytes[2],
	};
}

PIF_DEF PIF_Palette *PIF_paletteNew(int size) {
	PIF_assert(size <= PIF_COLORS);

	int          cap  = sizeof(PIF_Palette) + (size - 1) * sizeof(PIF_Rgb);
	PIF_Palette *this = (PIF_Palette*)PIF_alloc(cap);
	PIF_checkAlloc(this);

	memset(this, 0, cap);
	this->size = size;
	return this;
}

PIF_DEF PIF_Palette *PIF_paletteRead(FILE *file, const char **err) {
	PIF_assert(file != NULL);

	/* Verify magic bytes */
	char magic[sizeof(PIF_PALETTE_MAGIC) - 1];
	if (fread(magic, 1, sizeof(magic), file) != sizeof(magic))
		return PIF_error(err, "Failed to read magic bytes");

	if (strncmp(magic, PIF_PALETTE_MAGIC, sizeof(magic)) != 0)
		return PIF_error(err, "File is not a PIF palette");

	/* Read header */
	int maxColor = fgetc(file);
	if (maxColor == EOF)
		return PIF_error(err, "Failed to read PIF palette maximum color index");

	PIF_Palette *this = PIF_paletteNew(maxColor + 1);
	PIF_assert(this != NULL);

	/* Read body */
	for (int i = 0; i < this->size; ++ i) {
		uint8_t bytes[3];
		if (fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) {
			PIF_paletteFree(this);
			return PIF_error(err, "Failed to read PIF palette body");
		}

		this->map[i].r = bytes[0];
		this->map[i].g = bytes[1];
		this->map[i].b = bytes[2];
	}
	return this;
}

PIF_DEF PIF_Palette *PIF_paletteLoad(const char *path, const char **err) {
	PIF_assert(path != NULL);

	FILE *file = fopen(path, "rb");
	if (file == NULL)
		return PIF_error(err, "Could not open file");

	PIF_Palette *this = PIF_paletteRead(file, err);

	fclose(file);
	return this;
}

PIF_DEF void PIF_paletteWrite(PIF_Palette *this, FILE *file) {
	PIF_assert(this != NULL);
	PIF_assert(file != NULL);

	/* Write magic bytes */
	fwrite(PIF_PALETTE_MAGIC, 1, sizeof(PIF_PALETTE_MAGIC) - 1, file);

	/* Write header */
	fputc(this->size - 1, file);

	/* Write body */
	for (int i = 0; i < this->size; ++ i) {
		uint8_t bytes[3] = {
			this->map[i].r,
			this->map[i].g,
			this->map[i].b,
		};
		fwrite(bytes, 1, sizeof(bytes), file);
	}
}

PIF_DEF int PIF_paletteSave(PIF_Palette *this, const char *path) {
	PIF_assert(this != NULL);
	PIF_assert(path != NULL);

	FILE *file = fopen(path, "wb");
	if (file == NULL)
		return -1;

	PIF_paletteWrite(this, file);

	fclose(file);
	return 0;
}

PIF_DEF void PIF_paletteFree(PIF_Palette *this) {
	PIF_assert(this != NULL);

	PIF_free(this);
}

PIF_DEF uint8_t PIF_paletteClosest(PIF_Palette *this, PIF_Rgb rgb) {
	PIF_assert(this != NULL);

	uint8_t color   =  0;
	int     minDiff = -1;
	for (int i = 0; i < this->size; ++ i) {
		if (i == PIF_TRANSPARENT)
			continue;

		int diff = PIF_rgbDiff(rgb, this->map[i]);
		if (minDiff == -1 || diff < minDiff) {
			minDiff = diff;
			color   = i;
		}
	}
	return color;
}

PIF_DEF PIF_Image *PIF_paletteCreateColormap(PIF_Palette *this, int shades, float t) {
	PIF_assert(this != NULL);

	PIF_Image *colormap = PIF_imageNew(this->size, this->size + shades);
	for (int x = 0; x < this->size; ++ x) {
		for (int y = 0; y < shades; ++ y) {
			uint8_t color;
			if (x == PIF_TRANSPARENT)
				color = PIF_TRANSPARENT;
			else {
				float darken = (float)(shades - y) / (float)(shades / 2);

				int r = (int)this->map[x].r * darken;
				int g = (int)this->map[x].g * darken;
				int b = (int)this->map[x].b * darken;

				if (r > 255) r = 255;
				if (g > 255) g = 255;
				if (b > 255) b = 255;

				color = PIF_paletteClosest(this, (PIF_Rgb){.r = r, .g = g, .b = b});
			}

			*PIF_imageAt(colormap, x, y) = color;
		}
	}

	for (int y = 0; y < this->size; ++ y) {
		for (int x = 0; x < this->size; ++ x) {
			uint8_t color;
			if (x == PIF_TRANSPARENT || y == PIF_TRANSPARENT)
				color = PIF_TRANSPARENT;
			else
				color = PIF_paletteClosest(this, PIF_rgbLerp(this->map[x], this->map[y], t));

			*PIF_imageAt(colormap, x, y + shades) = color;
		}
	}
	return colormap;
}

PIF_DEF void PIF_blendShader(int x, int y, uint8_t *pixel, uint8_t color, PIF_Image *img) {
	(void)x; (void)y;
	PIF_Image *colormap = (PIF_Image*)img->data;
	PIF_assert(colormap != NULL);

	*pixel = PIF_blendColor(*pixel, color, colormap);
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
	PIF_Image *this = (PIF_Image*)PIF_alloc(cap);
	PIF_checkAlloc(this);

	memset(this, 0, cap);
	this->w    = w;
	this->h    = h;
	this->size = w * h;
	return this;
}

PIF_DEF PIF_Image *PIF_imageRead(FILE *file, const char **err) {
	PIF_assert(file != NULL);

	/* Verify magic bytes */
	char magic[sizeof(PIF_IMAGE_MAGIC) - 1];
	if (fread(magic, 1, sizeof(magic), file) != sizeof(magic))
		return PIF_error(err, "Failed to read magic bytes");

	if (strncmp(magic, PIF_IMAGE_MAGIC, sizeof(magic)) != 0)
		return PIF_error(err, "File is not a PIF image");

	/* Read header */
	uint32_t w, h;
	if (PIF_read32(file, &w) != 0 || PIF_read32(file, &h) != 0)
		return PIF_error(err, "Failed to read PIF image size");

	PIF_Image *this = PIF_imageNew(w, h);

	/* Read body */
	if (fread(this->buf, 1, this->size, file) != (size_t)this->size) {
		PIF_imageFree(this);
		return PIF_error(err, "Failed to read PIF image body");
	}
	return this;
}

PIF_DEF PIF_Image *PIF_Image_load(const char *path, const char **err) {
	PIF_assert(path != NULL);

	FILE *file = fopen(path, "rb");
	if (file == NULL)
		return PIF_error(err, "Could not open file");

	PIF_Image *this = PIF_imageRead(file, err);

	fclose(file);
	return this;
}

PIF_DEF void PIF_imageWrite(PIF_Image *this, FILE *file) {
	PIF_assert(this != NULL);
	PIF_assert(file != NULL);

	/* Write magic bytes */
	fwrite(PIF_IMAGE_MAGIC, 1, sizeof(PIF_IMAGE_MAGIC) - 1, file);

	/* Write header */
	PIF_write32(file, this->w);
	PIF_write32(file, this->h);

	/* Write body */
	fwrite(this->buf, 1, this->size, file);
}

PIF_DEF int PIF_imageSave(PIF_Image *this, const char *path) {
	PIF_assert(this != NULL);
	PIF_assert(path != NULL);

	FILE *file = fopen(path, "wb");
	if (file == NULL)
		return -1;

	PIF_imageWrite(this, file);

	fclose(file);
	return 0;
}

PIF_DEF void PIF_imageFree(PIF_Image *this) {
	PIF_assert(this != NULL);

	PIF_free(this);
}

PIF_DEF void PIF_imageSetShader(PIF_Image *this, PIF_Shader shader, void *data) {
	PIF_assert(this != NULL);

	this->shader = shader;
	this->data   = data;
}

PIF_DEF void PIF_imageConvertPalette(PIF_Image *this, PIF_Palette *from, PIF_Palette *to) {
	PIF_assert(this != NULL);
	PIF_assert(from != NULL);
	PIF_assert(to   != NULL);

	for (int y = 0; y < this->h; ++ y) {
		for (int x = 0; x < this->w; ++ x) {
			uint8_t *pixel = PIF_imageAt(this, x, y);
			*pixel = PIF_paletteClosest(to, from->map[*pixel]);
		}
	}
}

PIF_DEF uint8_t *PIF_imageAt(PIF_Image *this, int x, int y) {
	PIF_assert(this != NULL);

	if (x < 0 || x >= this->w || y < 0 || y >= this->h)
		return NULL;

	return this->buf + this->w * y + x;
}

PIF_DEF PIF_Image *PIF_imageResize(PIF_Image *this, int w, int h) {
	uint8_t *prevBuf = (uint8_t*)PIF_alloc(this->size);
	PIF_checkAlloc(prevBuf);
	memcpy(prevBuf, this->buf, this->size);

	int prevW = this->w, prevH = this->h;

	this->w    = w;
	this->h    = h;
	this->size = w * h;
	this       = (PIF_Image*)PIF_realloc(this, sizeof(PIF_Image) + this->size - 1);
	PIF_checkAlloc(this);

	memset(this->buf, 0, this->size);

	w = PIF_min(prevW, this->w);
	h = PIF_min(prevH, this->h);
	for (int y = 0; y < h; ++ y) {
		for (int x = 0; x < w; ++ x)
			*PIF_imageAt(this, x, y) = prevBuf[prevW * y + x];
	}

	PIF_free(prevBuf);
	return this;
}

PIF_DEF void PIF_imageClear(PIF_Image *this, uint8_t color) {
	PIF_assert(this != NULL);

	memset(this->buf, color, this->size);
}

PIF_DEF PIF_Image *PIF_imageCopy(PIF_Image *this, PIF_Image *from) {
	this->w    = from->w;
	this->h    = from->h;
	this->size = from->w * from->h;
	this       = (PIF_Image*)PIF_realloc(this, sizeof(PIF_Image) + this->size - 1);
	PIF_checkAlloc(this);

	memcpy(this->buf, from->buf, this->size);
	return this;
}

PIF_DEF PIF_Image *PIF_imageDup(PIF_Image *this) {
	int        cap   = sizeof(PIF_Image) + this->size - 1;
	PIF_Image *duped = (PIF_Image*)PIF_alloc(cap);
	memcpy(duped, this, cap);
	return duped;
}

PIF_DEF void PIF_imageBlit(PIF_Image *this, PIF_Rect *destRect, PIF_Image *src, PIF_Rect *srcRect) {
	PIF_assert(this != NULL);
	PIF_assert(src  != NULL);

	PIF_Rect destRect_ = {0, 0, this->w, this->h};
	PIF_Rect srcRect_  = {0, 0, src->w,  src->h};
	if (srcRect  == NULL) srcRect  = &srcRect_;
	if (destRect == NULL) destRect = &destRect_;

	float scaleX = (float)srcRect->w / destRect->w;
	float scaleY = (float)srcRect->h / destRect->h;

	for (int y = 0; y < destRect->h; ++ y) {
		int destY = destRect->y + y;
		if (destY <  0)       continue;
		if (destY >= this->h) break;

		for (int x = 0; x < destRect->w; ++ x) {
			int destX = destRect->x + x;
			if (destX <  0)       continue;
			if (destX >= this->w) break;

			uint8_t *color = PIF_imageAt(src, scaleX * x + srcRect->x, scaleY * y + srcRect->y);
			PIF_assert(color != NULL);

			if (*color == PIF_TRANSPARENT)
				continue;

			PIF_imageDrawPoint(this, destX, destY, *color);
		}
	}
}

PIF_DEF void PIF_imageTransformBlit(PIF_Image *this, PIF_Rect *destRect, PIF_Image *src,
                                    PIF_Rect *srcRect, float mat[2][2], int cx, int cy) {
	PIF_assert(this != NULL);
	PIF_assert(src  != NULL);

	PIF_Rect destRect_ = {0, 0, this->w, this->h};
	PIF_Rect srcRect_  = {0, 0, src->w,  src->h};
	if (srcRect  == NULL) srcRect  = &srcRect_;
	if (destRect == NULL) destRect = &destRect_;

	PIF_CopyInfo copyInfo = {
		.src       = src,
		.srcRect   = *srcRect,
		.destRect  = *destRect,
		.ox        = -cx,
		.oy        = -cy,
		.transform = true,
	};
	PIF_invertMatrix2x2(mat, copyInfo.mat);

	PIF_Shader prevShader = this->shader;
	void      *prevData   = this->data;
	PIF_imageSetShader(this, PIF_exCopyShader, &copyInfo);
	PIF_imageFillTransformRect(this, destRect, 1, mat, cx, cy);
	PIF_imageSetShader(this, prevShader, prevData);
}

PIF_DEF void PIF_imageRotateBlit(PIF_Image *this, PIF_Rect *destRect, PIF_Image *src,
                                 PIF_Rect *srcRect, float angle, int cx, int cy) {
	float rotMat[2][2];
	PIF_makeRotationMatrix2x2(angle, rotMat);
	PIF_imageTransformBlit(this, destRect, src, srcRect, rotMat, cx, cy);
}

PIF_DEF void PIF_imageDrawPoint(PIF_Image *this, int x, int y, uint8_t color) {
	PIF_assert(this != NULL);

	if (color == PIF_TRANSPARENT)
		return;

	uint8_t *pixel = PIF_imageAt(this, x, y);
	if (pixel == NULL)
		return;

	if (this->shader == NULL) {
		*pixel = color;
		return;
	}

	this->shader(x, y, pixel, color, this);
}

/* Bresenham's line algorithm */
PIF_DEF void PIF_imageDrawLine(PIF_Image *this, int x1, int y1, int x2, int y2,
                               int n, uint8_t color) {
	PIF_assert(this != NULL);

	if (color == PIF_TRANSPARENT)
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

			PIF_imageDrawPoint(this, x, y, color);
		}

		err -= distY;
		if (err < 0) {
			y1  += stepY;
			err += distX;
		}
		++ x1;
	}
}

PIF_DEF void PIF_imageDrawRect(PIF_Image *this, PIF_Rect *rect, int n, uint8_t color) {
	PIF_assert(this != NULL);

	if (color == PIF_TRANSPARENT)
		return;

	PIF_Rect rect_ = {0, 0, this->w, this->h};
	if (rect == NULL)
		rect = &rect_;

	PIF_imageDrawLine(this, rect->x, rect->y, rect->x + rect->w - 1, rect->y, n, color);
	PIF_imageDrawLine(this, rect->x, rect->y + rect->h, rect->x, rect->y + 1, n, color);
	PIF_imageDrawLine(this, rect->x + rect->w, rect->y,
	                  rect->x + rect->w, rect->y + rect->h - 1, n, color);
	PIF_imageDrawLine(this, rect->x + rect->w, rect->y + rect->h,
	                  rect->x + 1, rect->y + rect->h, n, color);
}

/* Midpoint circle algorithm */
PIF_DEF void PIF_imageDrawCircle(PIF_Image *this, int cx, int cy, int r, int n, uint8_t color) {
	PIF_assert(this != NULL);

	if (color == PIF_TRANSPARENT)
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
			PIF_imageDrawPoint(this, cx + x, cy + y, color);
			PIF_imageDrawPoint(this, cx + y, cy + x, color);
			PIF_imageDrawPoint(this, cx - y, cy + x, color);
			PIF_imageDrawPoint(this, cx - x, cy + y, color);
			PIF_imageDrawPoint(this, cx - x, cy - y, color);
			PIF_imageDrawPoint(this, cx - y, cy - x, color);
			PIF_imageDrawPoint(this, cx + y, cy - x, color);
			PIF_imageDrawPoint(this, cx + x, cy - y, color);
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

PIF_DEF void PIF_imageDrawTriangle(PIF_Image *this, int x1, int y1, int x2, int y2,
                                   int x3, int y3, int n, uint8_t color) {
	PIF_assert(this != NULL);

	if (color == PIF_TRANSPARENT)
		return;

	PIF_imageDrawLine(this, x1, y1, x2, y2, n, color);
	PIF_imageDrawLine(this, x2, y2, x3, y3, n, color);
	PIF_imageDrawLine(this, x3, y3, x1, y1, n, color);
}

PIF_DEF void PIF_imageDrawTransformRect(PIF_Image *this, PIF_Rect *rect, int n, uint8_t color,
                                        float mat[2][2], int cx, int cy) {
	PIF_assert(this != NULL);

	if (color == PIF_TRANSPARENT)
		return;

	PIF_Rect rect_ = {0, 0, this->w, this->h};
	if (rect == NULL)
		rect = &rect_;

	int ps[4][2];
	PIF_transformRect(rect, mat, cx, cy, ps);

	PIF_imageDrawLine(this, ps[0][0], ps[0][1], ps[1][0], ps[1][1], n, color);
	PIF_imageDrawLine(this, ps[1][0], ps[1][1], ps[2][0], ps[2][1], n, color);
	PIF_imageDrawLine(this, ps[2][0], ps[2][1], ps[3][0], ps[3][1], n, color);
	PIF_imageDrawLine(this, ps[3][0], ps[3][1], ps[0][0], ps[0][1], n, color);
}

PIF_DEF void PIF_imageDrawRotateRect(PIF_Image *this, PIF_Rect *rect, int n, uint8_t color,
                                     float angle, int cx, int cy) {
	float rotMat[2][2];
	PIF_makeRotationMatrix2x2(angle, rotMat);
	PIF_imageDrawTransformRect(this, rect, n, color, rotMat, cx, cy);
}

PIF_DEF void PIF_imageFillRect(PIF_Image *this, PIF_Rect *rect, uint8_t color) {
	PIF_assert(this != NULL);

	if (color == PIF_TRANSPARENT)
		return;

	PIF_Rect rect_ = {0, 0, this->w, this->h};
	if (rect == NULL)
		rect = &rect_;

	for (int y = rect->y; y < rect->y + rect->h; ++ y) {
		if (y <  0)       continue;
		if (y >= this->h) break;

		for (int x = rect->x; x < rect->x + rect->w; ++ x) {
			if (x <  0)       continue;
			if (x >= this->w) break;

			PIF_imageDrawPoint(this, x, y, color);
		}
	}
}

PIF_DEF void PIF_imageFillCircle(PIF_Image *this, int cx, int cy, int r, uint8_t color) {
	PIF_assert(this != NULL);

	if (color == PIF_TRANSPARENT)
		return;

	int d  = r * 2;
	int rr = r * r;
	for (int y = 0; y < d; ++ y) {
		int dy    = r - y;
		int destY = cy + dy;
		if (destY <  0)       break;
		if (destY >= this->h) continue;

		for (int x = 0; x < d; ++ x) {
			int dx    = r - x;
			int destX = cx + dx;
			if (destX <  0)       break;
			if (destX >= this->w) continue;

			int dy = r - y;
			if (dx * dx + dy * dy < rr)
				PIF_imageDrawPoint(this, destX, destY, color);
		}
	}
}

static void PIF_imageFillFlatSideTriangle(PIF_Image *this, int x1, int y1, int x2, int y2,
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
		if ((yStep > 0 && y >= this->h) || (yStep < 0 && y < 0))
			break;

		/* Fix gaps */
		if      (xPrevEnd   + 1 < xStart) xStart = xPrevEnd   + 1;
		else if (xPrevStart - 1 > xEnd)   xEnd   = xPrevStart - 1;

		/* Scanline */
		if (y >= 0) {
			for (int x = PIF_max(round(xStart), 0); x <= PIF_min(round(xEnd), this->w); ++ x)
				PIF_imageDrawPoint(this, x, y, color);
		}

		/* Step */
		xPrevEnd   = xEnd;
		xPrevStart = xStart;
		xStart    += slopeA;
		xEnd      += slopeB;
	}
}

/* TODO: Make triangle filling more precise? */
PIF_DEF void PIF_imageFillTriangle(PIF_Image *this, int x1, int y1, int x2, int y2,
                                   int x3, int y3, uint8_t color) {
	PIF_assert(this != NULL);

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
	if      (y2 == y3) PIF_imageFillFlatSideTriangle(this, x1, y1, x2, y2, x3, color, false);
	else if (y1 == y2) PIF_imageFillFlatSideTriangle(this, x3, y3, x1, y1, x2, color, false);
	else {
		/* Otherwise split the triangle into 2 flat sided triangles */
		float slope = (float)(x3 - x1) / (y3 - y1);
		int   x4    = round((float)x3 - slope * (y3 - y2));

		PIF_imageFillFlatSideTriangle(this, x1, y1, x2, y2, x4, color, false);
		PIF_imageFillFlatSideTriangle(this, x3, y3, x2, y2, x4, color, true);
	}
}

PIF_DEF void PIF_imageFillTransformRect(PIF_Image *this, PIF_Rect *rect, uint8_t color,
                                        float mat[2][2], int cx, int cy) {
	PIF_assert(this != NULL);

	if (color == PIF_TRANSPARENT)
		return;

	PIF_Rect rect_ = {0, 0, this->w, this->h};
	if (rect == NULL)
		rect = &rect_;

	int ps[4][2];
	PIF_transformRect(rect, mat, cx, cy, ps);

	PIF_imageFillTriangle(this, ps[0][0], ps[0][1], ps[1][0], ps[1][1], ps[3][0], ps[3][1], color);
	PIF_imageFillTriangle(this, ps[2][0], ps[2][1], ps[1][0], ps[1][1], ps[3][0], ps[3][1], color);
}

PIF_DEF void PIF_imageFillRotateRect(PIF_Image *this, PIF_Rect *rect, uint8_t color,
                                     float angle, int cx, int cy) {
	float rotMat[2][2];
	PIF_makeRotationMatrix2x2(angle, rotMat);
	PIF_imageFillTransformRect(this, rect, color, rotMat, cx, cy);
}

PIF_DEF PIF_Font *PIF_fontNew(int chHeight, uint8_t *chWidths, PIF_Image *sheet,
                              uint8_t chSpacing, uint8_t lineSpacing) {
	PIF_assert(chWidths != NULL);
	PIF_assert(sheet    != NULL);

	PIF_Font *this = (PIF_Font*)PIF_alloc(sizeof(PIF_Font));
	PIF_checkAlloc(this);

	/* Calculate character positions */
	int x = 0, y = 0;
	for (int i = 0; i < 256; ++ i) {
		int w = chWidths[i];
		if (x + w > sheet->w) {
			x  = 0;
			y += chHeight;
		}

		this->chInfo[i].x = x;
		this->chInfo[i].y = y;
		this->chInfo[i].w = w;
		x += w;
	}

	this->chHeight    = chHeight;
	this->sheet       = sheet;
	this->chSpacing   = chSpacing;
	this->lineSpacing = lineSpacing;
	this->scale       = 1;
	return this;
}

PIF_DEF PIF_Font *PIF_fontRead(FILE *file, const char **err) {
	PIF_assert(file != NULL);

	/* Verify magic bytes */
	char magic[sizeof(PIF_FONT_MAGIC) - 1];
	if (fread(magic, 1, sizeof(magic), file) != sizeof(magic))
		return PIF_error(err, "Failed to read magic bytes");

	if (strncmp(magic, PIF_FONT_MAGIC, sizeof(magic)) != 0)
		return PIF_error(err, "File is not a PIF font");

	/* Read spacing */
	int chSpacing = fgetc(file);
	if (chSpacing == EOF)
		return PIF_error(err, "Failed to read PIF font character spacing");

	int lineSpacing = fgetc(file);
	if (lineSpacing == EOF)
		return PIF_error(err, "Failed to read PIF font line spacing");

	/* Read character height */
	int chHeight = fgetc(file);
	if (chHeight == EOF)
		return PIF_error(err, "Failed to read PIF font character height");

	/* Read character widths */
	uint8_t chWidths[256];
	if (fread(chWidths, 1, sizeof(chWidths), file) != sizeof(chWidths))
		return PIF_error(err, "Failed to read PIF font character widths");

	/* Read sheet */
	PIF_Image *sheet = PIF_imageRead(file, err);
	if (sheet == NULL)
		return NULL;

	/* Create font */
	PIF_Font *this = PIF_fontNew(chHeight, chWidths, sheet, chSpacing, lineSpacing);
	PIF_assert(this != NULL);
	return this;
}

PIF_DEF PIF_Font *PIF_fontLoad(const char *path, const char **err) {
	PIF_assert(path != NULL);

	FILE *file = fopen(path, "rb");
	if (file == NULL)
		return PIF_error(err, "Could not open file");

	PIF_Font *this = PIF_fontRead(file, err);

	fclose(file);
	return this;
}

PIF_DEF void PIF_fontWrite(PIF_Font *this, FILE *file) {
	PIF_assert(this        != NULL);
	PIF_assert(file        != NULL);
	PIF_assert(this->sheet != NULL);

	/* Write magic bytes */
	fwrite(PIF_FONT_MAGIC, 1, sizeof(PIF_FONT_MAGIC) - 1, file);

	/* Write spacing */
	fputc(this->chSpacing,   file);
	fputc(this->lineSpacing, file);

	/* Write character height */
	fputc(this->chHeight, file);

	/* Write character widths */
	for (int i = 0; i < 256; ++ i)
		fputc(this->chInfo[i].w, file);

	/* Write sheet */
	PIF_imageWrite(this->sheet, file);
}

PIF_DEF int PIF_fontSave(PIF_Font *this, const char *path) {
	PIF_assert(this != NULL);
	PIF_assert(path != NULL);

	FILE *file = fopen(path, "wb");
	if (file == NULL)
		return -1;

	PIF_fontWrite(this, file);

	fclose(file);
	return 0;
}

PIF_DEF void PIF_fontFree(PIF_Font *this) {
	PIF_assert(this != NULL);

	PIF_imageFree(this->sheet);
	PIF_free(this);
}

PIF_DEF void PIF_fontSetSpacing(PIF_Font *this, uint8_t chSpacing, uint8_t lineSpacing) {
	PIF_assert(this != NULL);

	this->chSpacing   = chSpacing;
	this->lineSpacing = lineSpacing;
}

PIF_DEF void PIF_fontSetScale(PIF_Font *this, float scale) {
	PIF_assert(this != NULL);

	this->scale = scale;
}

PIF_DEF void PIF_fontCharSize(PIF_Font *this, char ch, int *w, int *h) {
	PIF_assert(this != NULL);

	if (w != NULL) *w = round((float)this->chInfo[(int)ch].w * this->scale);
	if (h != NULL) *h = round((float)this->chHeight          * this->scale);
}

PIF_DEF void PIF_fontTextSize(PIF_Font *this, const char *text, int *w, int *h) {
	PIF_assert(this != NULL);
	PIF_assert(text != NULL);

	int w_, h_;
	if (w == NULL) w = &w_;
	if (h == NULL) h = &h_;

	*w = 0;
	*h = round((float)this->chHeight * this->scale);
	for (int rowWidth = 0; *text != '\0'; ++ text) {
		if (*text == '\n') {
			rowWidth = 0;
			*h += round((float)(this->chHeight + this->lineSpacing) * this->scale);
			continue;
		}

		int space = rowWidth > 0? this->chSpacing : 0;
		rowWidth += round((float)(this->chInfo[(int)*text].w + space) * this->scale);
		if (rowWidth > *w)
			*w = rowWidth;
	}
}

PIF_DEF void PIF_fontRenderChar(PIF_Font *this, char ch, PIF_Image *img,
                                int xStart, int yStart, uint8_t color) {
	PIF_assert(this != NULL);
	PIF_assert(img  != NULL);
	PIF_assert(ch   != '\0');

	PIF_FontCharInfo chInfo = this->chInfo[(int)ch];
	if (chInfo.w == 0)
		return;

	for (int y = 0; y < round((float)this->chHeight * this->scale); ++ y) {
		int srcY  = chInfo.y + (float)y / this->scale;
		int destY = yStart + y;
		if (destY <  0)      continue;
		if (destY >= img->h) break;

		for (int x = 0; x < round((float)chInfo.w * this->scale); ++ x) {
			int srcX = chInfo.x + (float)x / this->scale;

			uint8_t *pixel = PIF_imageAt(this->sheet, srcX, srcY);
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

PIF_DEF void PIF_fontRenderText(PIF_Font *this, const char *text, PIF_Image *img,
                                int xStart, int yStart, uint8_t color) {
	PIF_assert(this != NULL);
	PIF_assert(text != NULL);
	PIF_assert(img  != NULL);

	for (int x = xStart, y = yStart; *text != '\0'; ++ text) {
		if (*text == '\n') {
			x  = xStart;
			y += round((float)(this->chHeight + this->lineSpacing) * this->scale);
			continue;
		}

		PIF_fontRenderChar(this, *text, img, x, y, color);
		x += round((float)(this->chInfo[(int)*text].w + this->chSpacing) * this->scale);
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
#undef PIF_swap
#undef PIF_max
#undef PIF_min
