/**
 *  Copyright 2022 Zack Schrage
 */

#include "GShader.h"
#include "GBitmap.h"
#include "GPoint.h"
#include "GMatrix.h"
#include <vector>
#include <iostream>

class ZTriColorShader : public GShader {

public:

    ZTriColorShader(const GPoint pts[], const GColor colors[]) {
        int v1X = pts[1].x() - pts[0].x();
        int v1Y = pts[1].y() - pts[0].y();
        int v2X = pts[2].x() - pts[0].x();
        int v2Y = pts[2].y() - pts[0].y();
        lm = GMatrix(v1X, v2X, pts[0].x(), v1Y, v2Y, pts[0].y());
        tm = lm;
        this->colors[0] = colors[0];
        this->colors[1] = colors[1];
        this->colors[2] = colors[2];
    }

    bool isOpaque() {
        return false;
    }

    bool setContext(const GMatrix& ctm) {
        return GMatrix::Concat(ctm, lm).invert(&tm);
    }

    void shadeRow(int x, int y, int count, GPixel row[]) {
        GPoint ptsSrc[count];
        GPoint ptsDst[count];
        for (int i = 0; i < count; i++) {
            ptsSrc[i] = GPoint::Make(x + i + 0.5, y + 0.5);
        }
        tm.mapPoints(ptsDst, ptsSrc, count);
        for (int i = 0; i < count; i++) {
            row[i] = colorToPixel(interpolate(ptsDst[i].x(), ptsDst[i].y()));
        }
    }

    GColor interpolate(float x, float y) {
        GColor color;
        color.a = (x * colors[1].a) + (y * colors[2].a) + ((1-x-y)*colors[0].a);
        color.r = (x * colors[1].r) + (y * colors[2].r) + ((1-x-y)*colors[0].r);
        color.g = (x * colors[1].g) + (y * colors[2].g) + ((1-x-y)*colors[0].g);
        color.b = (x * colors[1].b) + (y * colors[2].b) + ((1-x-y)*colors[0].b);
        return color;
    }

    GPixel colorToPixel(GColor c) {
        c = c.pinToUnit();
        unsigned a = GRoundToInt(c.a * 255);
        unsigned r = GRoundToInt(c.a * c.r * 255);
        unsigned g = GRoundToInt(c.a * c.g * 255);
        unsigned b = GRoundToInt(c.a * c.b * 255);
        return GPixel_PackARGB(a, r, g, b);
    }

private:

    GMatrix lm;
    GMatrix tm;
    GColor colors[3];

};

std::unique_ptr<GShader> GCreateTriColorShader(const GPoint pts[], const GColor colors[]);

std::unique_ptr<GShader> GCreateTriColorShader(const GPoint pts[], const GColor colors[]) {
    return std::unique_ptr<GShader>(new ZTriColorShader(pts, colors));
}