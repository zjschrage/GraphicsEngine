/**
 *  Copyright 2022 Zack Schrage
 */

#include "GShader.h"
#include "GBitmap.h"
#include "GPoint.h"
#include "GMatrix.h"
#include "ZTileable.h"
#include "ZGradientFunction.h"
#include <vector>

class ZGradient : public GShader {

public:

    ZGradient(GPoint p0, GPoint p1, const GColor color[], int count, GShader::TileMode tileMode, GradientFunction gradientFunction) {
        int vX = p1.x() - p0.x();
        int vY = p1.y() - p0.y();
        lm = GMatrix(vX, vY, p0.x(), vY, -vX, p0.y());
        for (int i = 0; i < count; i++) {
            colors.push_back(color[i]);
        }
        colors.push_back(color[count - 1]);
        switch(tileMode) {
            default:
            case kClamp:
                tileFunction = &clamp;
                break;
            case kRepeat:
                tileFunction = &repeat;
                break;
            case kMirror:
                tileFunction = &mirror;
                break;
        }
        if (gradientFunction == nullptr) gradientFunction = &linear;
        this->gradientFunction = gradientFunction;
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
            float t = tileFunction(gradientFunction(ptsDst[i].x(), ptsDst[i].y()));
            float n = t * (colors.size() - 2);
            int colorIdx = floor(n);
            float weight = n - colorIdx;
            row[i] = colorToPixel(interpolate(colorIdx, weight));
        }
    }

    GColor interpolate(int i, float weight) {
        GColor color;
        color.a = colors[i].a + weight*(colors[i+1].a - colors[i].a);
        color.r = colors[i].r + weight*(colors[i+1].r - colors[i].r);
        color.g = colors[i].g + weight*(colors[i+1].g - colors[i].g);
        color.b = colors[i].b + weight*(colors[i+1].b - colors[i].b);
        return color;
    }

    GPixel colorToPixel(GColor c) {
        c = c.pinToUnit();
        int a = GRoundToInt(c.a * 255);
        int r = GRoundToInt(c.a * c.r * 255);
        int g = GRoundToInt(c.a * c.g * 255);
        int b = GRoundToInt(c.a * c.b * 255);
        return GPixel_PackARGB(a, r, g, b);
    }

private:

    GMatrix lm;
    GMatrix tm;
    std::vector<GColor> colors;
    TileFunction tileFunction;
    GradientFunction gradientFunction;

};

std::unique_ptr<GShader> GCreateGradient(GPoint p0, GPoint p1, const GColor color[], int count, GShader::TileMode tileMode, GradientFunction gradientFunction) {
    return std::unique_ptr<GShader>(new ZGradient(p0, p1, color, count, tileMode, gradientFunction));
}

std::unique_ptr<GShader> GCreateLinearGradient(GPoint p0, GPoint p1, const GColor color[], int count, GShader::TileMode tileMode);

std::unique_ptr<GShader> GCreateLinearGradient(GPoint p0, GPoint p1, const GColor color[], int count, GShader::TileMode tileMode) {
    return std::unique_ptr<GShader>(new ZGradient(p0, p1, color, count, tileMode, nullptr));
}