/**
 *  Copyright 2022 Zack Schrage
 */

typedef void (*BlendFunction)(GPixel* src, GPixel* dest, int count, bool isShader);

BlendFunction pickBlend(GBlendMode blendMode, unsigned int srcA);
static void clearRow(GPixel* src, GPixel* dest, int count, bool isShader);
static void srcRow(GPixel* src, GPixel* dest, int count, bool isShader);
static void dstRow(GPixel* src, GPixel* dest, int count, bool isShader);
static void srcOverRow(GPixel* src, GPixel* dest, int count, bool isShader);
static void dstOverRow(GPixel* src, GPixel* dest, int count, bool isShader);
static void srcInRow(GPixel* src, GPixel* dest, int count, bool isShader);
static void dstInRow(GPixel* src, GPixel* dest, int count, bool isShader);
static void srcOutRow(GPixel* src, GPixel* dest, int count, bool isShader);
static void dstOutRow(GPixel* src, GPixel* dest, int count, bool isShader);
static void srcATopRow(GPixel* src, GPixel* dest, int count, bool isShader);
static void dstATopRow(GPixel* src, GPixel* dest, int count, bool isShader);
static void xOrRow(GPixel* src, GPixel* dest, int count, bool isShader);