/**
 *  Copyright 2022 Zack Schrage
 */

#include "GShader.h"
#include "GBitmap.h"
#include "GPoint.h"
#include "GMatrix.h"
#include <vector>

class ZTriProxyShader : public GShader {

public:

    ZTriProxyShader(const GPoint pts[], const GPoint texs[], GShader* shader) {
        float v1X = pts[1].x() - pts[0].x();
        float v1Y = pts[1].y() - pts[0].y();
        float v2X = pts[2].x() - pts[0].x();
        float v2Y = pts[2].y() - pts[0].y();
        GMatrix p = GMatrix(v1X, v2X, pts[0].x(), v1Y, v2Y, pts[0].y());
        float tv1X = texs[1].x() - texs[0].x();
        float tv1Y = texs[1].y() - texs[0].y();
        float tv2X = texs[2].x() - texs[0].x();
        float tv2Y = texs[2].y() - texs[0].y();
        GMatrix t = GMatrix(tv1X, tv2X, texs[0].x(), tv1Y, tv2Y, texs[0].y());
        GMatrix tInv;
        t.invert(&tInv);
        lm = GMatrix::Concat(p, tInv);
        this->shader = shader;
    }

    bool isOpaque() {
        return false;
    }

    bool setContext(const GMatrix& ctm) {
        return shader->setContext(GMatrix::Concat(ctm, lm));
    }

    void shadeRow(int x, int y, int count, GPixel row[]) {
        shader->shadeRow(x, y, count, row);
    }

private:

    GMatrix lm;
    GShader* shader;

};

std::unique_ptr<GShader> GCreateTriProxyShader(const GPoint pts[], const GPoint texs[], GShader* shader);

std::unique_ptr<GShader> GCreateTriProxyShader(const GPoint pts[], const GPoint texs[], GShader* shader) {
    return std::unique_ptr<GShader>(new ZTriProxyShader(pts, texs, shader));
}