#ifndef PIF_H_HEADER_GUARD
#define PIF_H_HEADER_GUARD

#include <stdio.h>   /* fopen, fclose, FILE, EOF, fprintf, stderr */
#include <stdlib.h>  /* malloc, realloc, free, abort */
#include <string.h>  /* memset, memcpy */
#include <stdint.h>  /* uint8_t, uint32_t */
#include <stdbool.h> /* bool, true, false */
#include <math.h>    /* max */

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

#define PIF_COLORS      256
#define PIF_TRANSPARENT 0

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
PIF_DEF uint32_t PIF_rgbToPixelRgba32(PIF_Rgb this);
PIF_DEF uint32_t PIF_rgbToPixelAbgr32(PIF_Rgb this);
PIF_DEF PIF_Rgb  PIF_rgbFromPixelRgba32(uint32_t pixel);
PIF_DEF PIF_Rgb  PIF_rgbFromPixelAbgr32(uint32_t pixel);

typedef struct {
	int     size;
	PIF_Rgb map[1];
} PIF_Palette;

PIF_DEF PIF_Palette *PIF_paletteNew  (int   size);
PIF_DEF PIF_Palette *PIF_paletteRead (FILE *file,        const char **err);
PIF_DEF PIF_Palette *PIF_paletteLoad (const char  *path, const char **err);
PIF_DEF void         PIF_paletteWrite(PIF_Palette *this, FILE        *file);
PIF_DEF int          PIF_paletteSave (PIF_Palette *this, const char  *path);
PIF_DEF void         PIF_paletteFree (PIF_Palette *this);

PIF_DEF uint8_t    PIF_paletteClosest(PIF_Palette *this, PIF_Rgb rgb);
PIF_DEF PIF_Image *PIF_paletteCreateColormap(PIF_Palette *this, int shades, float t);

typedef void (*PIF_Shader)(int, int, uint8_t*, uint8_t, PIF_Image*);

PIF_DEF void PIF_blendShader(int x, int y, uint8_t *pixel, uint8_t color, PIF_Image *img);
PIF_DEF void PIF_copyShader(int x, int y, uint8_t *pixel, uint8_t color, PIF_Image *img);

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

	int     w, h, size;
	uint8_t buf[1];
};

PIF_DEF PIF_Image *PIF_imageNew  (int w, int h);
PIF_DEF PIF_Image *PIF_imageRead (FILE       *file, const char **err);
PIF_DEF PIF_Image *PIF_imageLoad (const char *path, const char **err);
PIF_DEF void       PIF_imageWrite(PIF_Image  *this, FILE        *file);
PIF_DEF int        PIF_imageSave (PIF_Image  *this, const char  *path);
PIF_DEF void       PIF_imageFree (PIF_Image  *this);

PIF_DEF void       PIF_imageSetShader     (PIF_Image *this, PIF_Shader shader, void *data);
PIF_DEF void       PIF_imageConvertPalette(PIF_Image *this, PIF_Palette *from, PIF_Palette *to);
PIF_DEF uint8_t   *PIF_imageAt(PIF_Image *this, int x, int y);
PIF_DEF PIF_Image *PIF_imageResize(PIF_Image *this, int w, int h);
PIF_DEF PIF_Image *PIF_imageCopy  (PIF_Image *this, PIF_Image *from);
PIF_DEF PIF_Image *PIF_imageDup   (PIF_Image *this);

PIF_DEF void PIF_imageClear(PIF_Image *this, uint8_t color);
PIF_DEF void PIF_imageBlit (PIF_Image *this, PIF_Rect *destRect, PIF_Image *src, PIF_Rect *srcRect);
PIF_DEF void PIF_imageTransformBlit(PIF_Image *this, PIF_Rect *destRect, PIF_Image *src,
                                    PIF_Rect *srcRect, float mat[2][2], int cx, int cy);
PIF_DEF void PIF_imageRotateBlit(PIF_Image *this, PIF_Rect *destRect, PIF_Image *src,
                                 PIF_Rect *srcRect, float angle, int cx, int cy);

PIF_DEF void PIF_imageDrawPoint(PIF_Image *this, int x, int y, uint8_t color);
PIF_DEF void PIF_imageDrawLine (PIF_Image *this, int x1, int y1, int x2, int y2,
                                int n, uint8_t color);

PIF_DEF void PIF_imageDrawRect    (PIF_Image *this, PIF_Rect *rect, int n, uint8_t color);
PIF_DEF void PIF_imageDrawCircle  (PIF_Image *this, int cx, int cy, int r, int n, uint8_t color);
PIF_DEF void PIF_imageDrawTriangle(PIF_Image *this, int x1, int y1, int x2, int y2,
                                   int x3, int y3, int n, uint8_t color);
PIF_DEF void PIF_imageDrawTransformRect(PIF_Image *this, PIF_Rect *rect, int n, uint8_t color,
                                        float mat[2][2], int cx, int cy);
PIF_DEF void PIF_imageDrawRotateRect(PIF_Image *this, PIF_Rect *rect, int n, uint8_t color,
                                     float angle, int cx, int cy);

PIF_DEF void PIF_imageFillRect    (PIF_Image *this, PIF_Rect *rect, uint8_t color);
PIF_DEF void PIF_imageFillCircle  (PIF_Image *this, int cx, int cy, int r, uint8_t color);
PIF_DEF void PIF_imageFillTriangle(PIF_Image *this, int x1, int y1, int x2, int y2,
                                   int x3, int y3, uint8_t color);
PIF_DEF void PIF_imageFillTransformRect(PIF_Image *this, PIF_Rect *rect, uint8_t color,
                                        float mat[2][2], int cx, int cy);
PIF_DEF void PIF_imageFillRotateRect(PIF_Image *this, PIF_Rect *rect, uint8_t color,
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
PIF_DEF void      PIF_fontWrite(PIF_Font   *this,  FILE        *file);
PIF_DEF int       PIF_fontSave (PIF_Font   *this,  const char  *path);
PIF_DEF void      PIF_fontFree (PIF_Font   *this);

PIF_DEF void PIF_fontSetSpacing(PIF_Font *this, uint8_t chSpacing, uint8_t lineSpacing);
PIF_DEF void PIF_fontSetScale  (PIF_Font *this, float scale);

PIF_DEF void PIF_fontCharSize(PIF_Font *this, char ch, int *w, int *h);
PIF_DEF void PIF_fontTextSize(PIF_Font *this, const char *text, int *w, int *h);

PIF_DEF void PIF_fontRenderChar(PIF_Font *this, char ch, PIF_Image *img,
                                int xStart, int yStart, uint8_t color);
PIF_DEF void PIF_fontRenderText(PIF_Font *this, const char *text, PIF_Image *img,
                                int xStart, int yStart, uint8_t color);

PIF_DEF PIF_Font *PIF_fontNewDefault(void);

#endif
