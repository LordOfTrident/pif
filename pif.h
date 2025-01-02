#ifndef PIF_H_HEADER_GUARD
#define PIF_H_HEADER_GUARD

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>   /* fopen, fclose, FILE, EOF, fprintf, stderr, fwrite, fread */
#include <stdlib.h>  /* malloc, realloc, free, abort */
#include <string.h>  /* memset, memcpy, strncpy */
#include <stdint.h>  /* uint8_t, uint16_t, uint32_t */
#include <stdbool.h> /* bool, true, false */
#include <math.h>    /* sin, cos, round */
#include <limits.h>  /* USHRT_MAX */

#ifndef PIF_alloc
#	define PIF_alloc(SIZE) malloc(SIZE)
#endif

#ifndef PIF_realloc
#	define PIF_realloc(PTR, SIZE) realloc(PTR, SIZE)
#endif

#ifndef PIF_free
#	define PIF_free(PTR) free(PTR)
#endif

#ifndef PIF_assert
#	include <assert.h> /* assert */

#	define PIF_assert(X) assert(X)
#endif

#ifndef PIF_DEF
#	define PIF_DEF
#endif

#define PIF_COLORS         256
#define PIF_TRANSPARENT    0
#define PIF_DEFAULT_SHADES 64

#define PIF_STD_BLACK 1
#define PIF_STD_WHITE 2

#define PIF_PALETTE_MAGIC "PIFP"
#define PIF_IMAGE_MAGIC   "PIFI"
#define PIF_FONT_MAGIC    "PIFF"

#define PIF_PALETTE_EXT "pal"
#define PIF_IMAGE_EXT   "pif" /* Palettized Image File */
#define PIF_FONT_EXT    "pbf" /* Palettized Bitmap Font */

typedef struct PIF_Image PIF_Image;

PIF_DEF uint8_t PIF_shadeColor(uint8_t color, float   shade, PIF_Image *colormap);
PIF_DEF uint8_t PIF_blendColor(uint8_t from,  uint8_t to,    PIF_Image *colormap);

typedef struct {
	int x, y, w, h;
} PIF_Rect;

typedef struct {
	uint8_t r, g, b;
} PIF_Rgb;

PIF_DEF int      PIF_rgbDiff(PIF_Rgb a, PIF_Rgb b);
PIF_DEF PIF_Rgb  PIF_rgbLerp(PIF_Rgb a, PIF_Rgb b, float t);
PIF_DEF uint8_t  PIF_rgbToColor(PIF_Rgb rgb, PIF_Image *rgbmap);
PIF_DEF uint32_t PIF_rgbToPixelRgba32(PIF_Rgb self);
PIF_DEF uint32_t PIF_rgbToPixelAbgr32(PIF_Rgb self);
PIF_DEF PIF_Rgb  PIF_rgbFromPixelRgba32(uint32_t pixel);
PIF_DEF PIF_Rgb  PIF_rgbFromPixelAbgr32(uint32_t pixel);

typedef struct {
	int     size;
	PIF_Rgb map[1];
} PIF_Palette;

PIF_DEF PIF_Palette *PIF_paletteNew  (int   size);
PIF_DEF PIF_Palette *PIF_paletteRead (FILE *file,        const char **err);
PIF_DEF PIF_Palette *PIF_paletteLoad (const char  *path, const char **err);
PIF_DEF void         PIF_paletteWrite(PIF_Palette *self, FILE        *file);
PIF_DEF int          PIF_paletteSave (PIF_Palette *self, const char  *path);
PIF_DEF void         PIF_paletteFree (PIF_Palette *self);

#define PIF_palettesFree(...)                                            \
	do {                                                                 \
		PIF_Palette *pals_[] = {__VA_ARGS__};                            \
		for (int i = 0; i < (int)(sizeof(pals_) / sizeof(*pals_)); ++ i) \
			PIF_paletteFree(pals_[i]);                                   \
	} while (0)

PIF_DEF uint8_t    PIF_paletteClosest(PIF_Palette *self, PIF_Rgb rgb);
PIF_DEF PIF_Image *PIF_paletteCreateColormap(PIF_Palette *self, int shades, float t);
PIF_DEF PIF_Image *PIF_paletteCreateRgbmap(PIF_Palette *self, uint8_t size);

typedef void (*PIF_Shader)(int, int, uint8_t*, uint8_t, PIF_Image*);

PIF_DEF void PIF_blendShader (int x, int y, uint8_t *pixel, uint8_t color, PIF_Image *img);
PIF_DEF void PIF_ditherShader(int x, int y, uint8_t *pixel, uint8_t color, PIF_Image *img);
PIF_DEF void PIF_copyShader  (int x, int y, uint8_t *pixel, uint8_t color, PIF_Image *img);

typedef struct {
	PIF_Image *src;
	PIF_Rect   srcRect, destRect;
	int        ox, oy;

	bool  transform;
	float mat[2][2];
	int   cx, cy;
} PIF_CopyInfo;

PIF_DEF void PIF_exCopyShader(int x, int y, uint8_t *pixel, uint8_t color, PIF_Image *img);

struct PIF_Image {
	PIF_Shader shader;
	void      *data;
	bool       skipTransparent;

	int     w, h, size;
	uint8_t buf[1];
};

PIF_DEF PIF_Image *PIF_imageNew  (int w, int h);
PIF_DEF PIF_Image *PIF_imageRead (FILE       *file, const char **err);
PIF_DEF PIF_Image *PIF_imageLoad (const char *path, const char **err);
PIF_DEF void       PIF_imageWrite(PIF_Image  *self, FILE        *file);
PIF_DEF int        PIF_imageSave (PIF_Image  *self, const char  *path);
PIF_DEF void       PIF_imageFree (PIF_Image  *self);

#define PIF_imagesFree(...)                                              \
	do {                                                                 \
		PIF_Image *imgs_[] = {__VA_ARGS__};                              \
		for (int i = 0; i < (int)(sizeof(imgs_) / sizeof(*imgs_)); ++ i) \
			PIF_imageFree(imgs_[i]);                                     \
	} while (0)

PIF_DEF void PIF_imageSkipTransparent(PIF_Image *self, bool enable);

PIF_DEF void       PIF_imageSetShader     (PIF_Image *self, PIF_Shader shader, void *data);
PIF_DEF void       PIF_imageSetShaderData (PIF_Image *self, void *data);
PIF_DEF void       PIF_imageConvertPalette(PIF_Image *self, PIF_Palette *from, PIF_Palette *to);
PIF_DEF uint8_t   *PIF_imageAt(PIF_Image *self, int x, int y);
PIF_DEF PIF_Image *PIF_imageResize(PIF_Image *self, int w, int h, uint8_t color);
PIF_DEF PIF_Image *PIF_imageCopy  (PIF_Image *self, PIF_Image *from);
PIF_DEF PIF_Image *PIF_imageDup   (PIF_Image *self);

PIF_DEF void PIF_imageClear(PIF_Image *self, uint8_t color);
PIF_DEF void PIF_imageBlit (PIF_Image *self, PIF_Rect *destRect, PIF_Image *src, PIF_Rect *srcRect);
PIF_DEF void PIF_imageTransformBlit(PIF_Image *self, PIF_Rect *destRect, PIF_Image *src,
                                    PIF_Rect *srcRect, float mat[2][2], int cx, int cy);
PIF_DEF void PIF_imageRotateBlit(PIF_Image *self, PIF_Rect *destRect, PIF_Image *src,
                                 PIF_Rect *srcRect, float angle, int cx, int cy);

PIF_DEF void PIF_imageDrawPoint(PIF_Image *self, int x, int y, uint8_t color);
PIF_DEF void PIF_imageDrawLine (PIF_Image *self, int x1, int y1, int x2, int y2,
                                int n, uint8_t color);

PIF_DEF void PIF_imageDrawRect    (PIF_Image *self, PIF_Rect *rect, int n, uint8_t color);
PIF_DEF void PIF_imageDrawCircle  (PIF_Image *self, int cx, int cy, int r, int n, uint8_t color);
PIF_DEF void PIF_imageDrawTriangle(PIF_Image *self, int x1, int y1, int x2, int y2,
                                   int x3, int y3, int n, uint8_t color);
PIF_DEF void PIF_imageDrawTransformRect(PIF_Image *self, PIF_Rect *rect, int n, uint8_t color,
                                        float mat[2][2], int cx, int cy);
PIF_DEF void PIF_imageDrawRotateRect(PIF_Image *self, PIF_Rect *rect, int n, uint8_t color,
                                     float angle, int cx, int cy);

PIF_DEF void PIF_imageFillRect    (PIF_Image *self, PIF_Rect *rect, uint8_t color);
PIF_DEF void PIF_imageFillCircle  (PIF_Image *self, int cx, int cy, int r, uint8_t color);
PIF_DEF void PIF_imageFillTriangle(PIF_Image *self, int x1, int y1, int x2, int y2,
                                   int x3, int y3, uint8_t color);
PIF_DEF void PIF_imageFillTransformRect(PIF_Image *self, PIF_Rect *rect, uint8_t color,
                                        float mat[2][2], int cx, int cy);
PIF_DEF void PIF_imageFillRotateRect(PIF_Image *self, PIF_Rect *rect, uint8_t color,
                                     float angle, int cx, int cy);

typedef struct {
	uint8_t x, y, w;
} PIF_FontCharInfo;

typedef struct {
	PIF_FontCharInfo chInfo[256];
	uint8_t          chHeight;
	PIF_Image       *sheet;

	uint8_t chSpacing, lineSpacing;
	float   scale;
} PIF_Font;

PIF_DEF PIF_Font *PIF_fontNew(int chHeight, uint8_t *chWidths, PIF_Image *sheet,
                              uint8_t chSpacing, uint8_t lineSpacing);
PIF_DEF PIF_Font *PIF_fontRead (FILE       *file,  const char **err);
PIF_DEF PIF_Font *PIF_fontLoad (const char *path,  const char **err);
PIF_DEF void      PIF_fontWrite(PIF_Font   *self,  FILE        *file);
PIF_DEF int       PIF_fontSave (PIF_Font   *self,  const char  *path);
PIF_DEF void      PIF_fontFree (PIF_Font   *self);

#define PIF_fontsFree(...)                                                 \
	do {                                                                   \
		PIF_Font *fonts_[] = {__VA_ARGS__};                                \
		for (int i = 0; i < (int)(sizeof(fonts_) / sizeof(*fonts_)); ++ i) \
			PIF_fontFree(fonts_[i]);                                       \
	} while (0)

PIF_DEF void PIF_fontSetSpacing(PIF_Font *self, uint8_t chSpacing, uint8_t lineSpacing);
PIF_DEF void PIF_fontSetScale  (PIF_Font *self, float scale);

PIF_DEF void PIF_fontCharSize(PIF_Font *self, char ch, int *w, int *h);
PIF_DEF void PIF_fontTextSize(PIF_Font *self, const char *text, int *w, int *h);

PIF_DEF void PIF_fontRenderChar(PIF_Font *self, char ch, PIF_Image *img,
                                int xStart, int yStart, uint8_t color);
PIF_DEF void PIF_fontRenderText(PIF_Font *self, const char *text, PIF_Image *img,
                                int xStart, int yStart, uint8_t color);

PIF_DEF PIF_Font *PIF_fontNewDefault(void);

#define PIF_swap(A, B)                      \
	do {                                    \
		PIF_assert(sizeof(A) == sizeof(B)); \
		uint8_t tmp_[sizeof(A)];            \
		memcpy(tmp_, &(A), sizeof(A));      \
		A = B;                              \
		memcpy(&(B), tmp_, sizeof(B));      \
	} while (0)

#define PIF_zeroStruct(STRUCT) memset(STRUCT, 0, sizeof(*(STRUCT)));

#define PIF_max(A, B) ((A) > (B)? (A) : (B))
#define PIF_min(A, B) ((A) > (B)? (B) : (A))

#ifdef __cplusplus
}
#endif

#endif
