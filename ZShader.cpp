/**
 *  Copyright 2022 Zack Schrage
 */

#include "GShader.h"
#include "GBitmap.h"
#include "GPoint.h"
#include "GMatrix.h"
#include "ZTileable.h"

class ZShader : public GShader {

public:

    ZShader(const GBitmap& localBm, const GMatrix& localM, GShader::TileMode tileMode) {
        bm = localBm;
        lm = localM;
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
    }

    bool isOpaque() {
        return false;
    }

    bool setContext(const GMatrix& ctm) {
        GMatrix actualTm = GMatrix::Concat(ctm, lm);
        return GMatrix::Concat(actualTm, GMatrix(bm.width(), 0, 0, 0, bm.height(), 0)).invert(&tm);
    }

    void shadeRow(int x, int y, int count, GPixel row[]) {
        GPoint ptsSrc[count];
        GPoint ptsDst[count];
        for (int i = 0; i < count; i++) {
            ptsSrc[i] = GPoint::Make(x + i + 0.5, y + 0.5);
        }
        tm.mapPoints(ptsDst, ptsSrc, count);
        for (int i = 0; i < count; i++) {
            float x = tileFunction(ptsDst[i].x()) * (bm.width());
            float y = tileFunction(ptsDst[i].y()) * (bm.height());
            row[i] = *bm.getAddr(floor(x), floor(y));
        }
    }

private:

    GMatrix tm;
    GMatrix lm;
    GBitmap bm;
    TileFunction tileFunction;

};

std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& localBm, const GMatrix& localM, GShader::TileMode tileMode);

std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& localBm, const GMatrix& localM, GShader::TileMode tileMode) {
    return std::unique_ptr<GShader>(new ZShader(localBm, localM, tileMode));
}