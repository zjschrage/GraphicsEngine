/**
 *  Copyright 2022 Zack Schrage
 */

#include "GShader.h"
#include "GBitmap.h"
#include "GPoint.h"
#include "GMatrix.h"
#include <vector>
#include <iostream>

class ZComposedShader : public GShader {

public:

    ZComposedShader(GShader* triColorShader, GShader* triBMShader) {
        this->triColorShader = triColorShader;
        this->triBMShader = triBMShader;
    }

    bool isOpaque() {
        return false;
    }

    bool setContext(const GMatrix& ctm) {
        return (triColorShader->setContext(ctm) &&
                triBMShader->setContext(ctm));
    }

    void shadeRow(int x, int y, int count, GPixel row[]) {
        GPixel triColorRow[count];
        GPixel triBMRow[count];
        triColorShader->shadeRow(x, y, count, triColorRow);
        triBMShader->shadeRow(x, y, count, triBMRow);
        for (int i = 0; i < count; i++) {
            unsigned a = div255(GPixel_GetA(triColorRow[i]) * GPixel_GetA(triBMRow[i]));
            unsigned r = div255(GPixel_GetR(triColorRow[i]) * GPixel_GetR(triBMRow[i]));
            unsigned g = div255(GPixel_GetG(triColorRow[i]) * GPixel_GetG(triBMRow[i]));
            unsigned b = div255(GPixel_GetB(triColorRow[i]) * GPixel_GetB(triBMRow[i]));
            row[i] = GPixel_PackARGB(a, r, g, b);
        }
    }

    unsigned div255(unsigned x) {
        return (x * 65793 + (1 << 23)) >> 24;
    }

private:

    GShader* triColorShader;
    GShader* triBMShader;

};

std::unique_ptr<GShader> GCreateComposedShader(GShader* triColorShader, GShader* triBMShader);

std::unique_ptr<GShader> GCreateComposedShader(GShader* triColorShader, GShader* triBMShader) {
    return std::unique_ptr<GShader>(new ZComposedShader(triColorShader, triBMShader));
}