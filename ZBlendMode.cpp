/**
 *  Copyright 2022 Zack Schrage
 */

#include "GPixel.h"
#include "GMath.h"
#include "GBlendMode.h"
#include "ZBlendMode.h"

class ZBlendMode {

public:

    //0
    static GPixel clear(GPixel src, GPixel dest) {
        return GPixel_PackARGB(0, 0, 0, 0);
    }

    //S
    static GPixel src(GPixel src, GPixel dest) {
        return src;
    }

    //D
    static GPixel dst(GPixel src, GPixel dest) {
        return dest;
    }

    //S + (1-Sa)*D

    static GPixel srcOver(GPixel src, GPixel dest) {
        GPixel pixel = scale(GPixel_GetA(src), dest);
        return pixel + src;
    }

    //D + (1 - Da)*S
    static GPixel dstOver(GPixel src, GPixel dest) {
        GPixel pixel = scale(GPixel_GetA(dest), src);
        return pixel + dest;
    }

    //Da * S
    static GPixel srcIn(GPixel src, GPixel dest) {
        return scale2(GPixel_GetA(dest), src);
    }

    //Sa * D
    static GPixel dstIn(GPixel src, GPixel dest) {
        return scale2(GPixel_GetA(src), dest);
    }

    //(1 - Da)*S
    static GPixel srcOut(GPixel src, GPixel dest) {
        return scale(GPixel_GetA(dest), src);
    }

    //(1 - Sa)*D
    static GPixel dstOut(GPixel src, GPixel dest) {
        return scale(GPixel_GetA(src), dest);
    }

    //Da*S + (1 - Sa)*D
    static GPixel srcATop(GPixel src, GPixel dest) {
        GPixel pixel = scale(GPixel_GetA(src), dest);
        GPixel pixel2 = scale2(GPixel_GetA(dest), src);
        return pixel + pixel2;
    }

    //Sa*D + (1 - Da)*S
    static GPixel dstATop(GPixel src, GPixel dest) {
        GPixel pixel = scale(GPixel_GetA(dest), src);
        GPixel pixel2 = scale2(GPixel_GetA(src), dest);
        return pixel + pixel2;
    }

    //(1 - Sa)*D + (1 - Da)*S
    static GPixel xOr(GPixel src, GPixel dest) {
        GPixel pixel = scale(GPixel_GetA(src), dest);
        GPixel pixel2 = scale(GPixel_GetA(dest), src);
        return pixel + pixel2;
    }

    static unsigned div255(unsigned x) {
        return (x * 65793 + (1 << 23)) >> 24;
    }

    //(1 - alpha) * pixel
    static GPixel scale(int alpha, GPixel pixel) {
        int a = div255((255-alpha) * GPixel_GetA(pixel));
        int r = div255((255-alpha) * GPixel_GetR(pixel));
        int g = div255((255-alpha) * GPixel_GetG(pixel));
        int b = div255((255-alpha) * GPixel_GetB(pixel));
        return GPixel_PackARGB(a, r, g ,b);
    }

    //alpha * pixel
    static GPixel scale2(int alpha, GPixel pixel) {
        int a = div255(alpha * GPixel_GetA(pixel));
        int r = div255(alpha * GPixel_GetR(pixel));
        int g = div255(alpha * GPixel_GetG(pixel));
        int b = div255(alpha * GPixel_GetB(pixel));
        return GPixel_PackARGB(a, r, g ,b);
    }
};


BlendFunction pickBlend(GBlendMode blendMode, unsigned int srcA) {
    switch(blendMode) {
        case GBlendMode::kClear:
            return &clearRow;;
        case GBlendMode::kSrc:
            return &srcRow;
        case GBlendMode::kDst:
            return &dstRow;
        case GBlendMode::kSrcOver:
            if (srcA == 0) return &dstRow;
            else if (srcA == 255) return &srcRow;
            return &srcOverRow;
        case GBlendMode::kDstOver:
            if (srcA == 0) return &dstRow;
            return &dstOverRow;
        case GBlendMode::kSrcIn:
            return &srcInRow;
        case GBlendMode::kDstIn:
            return &dstInRow;
        case GBlendMode::kSrcOut:
            return &srcOutRow;
        case GBlendMode::kDstOut:
            if (srcA == 0) return &dstRow;
            else if (srcA == 255) return clearRow;
            return &dstOutRow;
        case GBlendMode::kSrcATop:
            return &srcATopRow;
        case GBlendMode::kDstATop:
            return &dstATopRow;
        case GBlendMode::kXor:
            return &xOrRow;
        default:
            return &clearRow;
    }
}

static void clearRow(GPixel* src, GPixel* dest, int count, bool isShader) {
    // if (isShader) {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::clear(src[i], dest[i]);
    //     }   
    // }
    // else {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::clear(*src, dest[i]);
    //     }
    // }
    for (int i = 0; i < count; i++) {
        dest[i] = ZBlendMode::clear(isShader ? src[i] : *src, dest[i]);
    }   
}

static void srcRow(GPixel* src, GPixel* dest, int count, bool isShader) {
    // if (isShader) {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::src(src[i], dest[i]);
    //     }   
    // }
    // else {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::src(*src, dest[i]);
    //     }
    // }
    for (int i = 0; i < count; i++) {
        dest[i] = ZBlendMode::src(isShader ? src[i] : *src, dest[i]);
    }   
}

static void dstRow(GPixel* src, GPixel* dest, int count, bool isShader) {
    // if (isShader) {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::dst(src[i], dest[i]);
    //     }   
    // }
    // else {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::dst(*src, dest[i]);
    //     }
    // }
    for (int i = 0; i < count; i++) {
        dest[i] = ZBlendMode::dst(isShader ? src[i] : *src, dest[i]);
    } 
}

static void srcOverRow(GPixel* src, GPixel* dest, int count, bool isShader) {
    // if (isShader) {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::srcOver(src[i], dest[i]);
    //     }   
    // }
    // else {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::srcOver(*src, dest[i]);
    //     }
    // }
    for (int i = 0; i < count; i++) {
        dest[i] = ZBlendMode::srcOver(isShader ? src[i] : *src, dest[i]);
    } 
}

static void dstOverRow(GPixel* src, GPixel* dest, int count, bool isShader) {
    // if (isShader) {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::dstOver(src[i], dest[i]);
    //     }   
    // }
    // else {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::dstOver(*src, dest[i]);
    //     }
    // }
    for (int i = 0; i < count; i++) {
        dest[i] = ZBlendMode::dstOver(isShader ? src[i] : *src, dest[i]);
    } 
}

static void srcInRow(GPixel* src, GPixel* dest, int count, bool isShader) {
    // if (isShader) {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::srcIn(src[i], dest[i]);
    //     }   
    // }
    // else {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::srcIn(*src, dest[i]);
    //     }
    // }
    for (int i = 0; i < count; i++) {
        dest[i] = ZBlendMode::srcIn(isShader ? src[i] : *src, dest[i]);
    } 
}

static void dstInRow(GPixel* src, GPixel* dest, int count, bool isShader) {
    // if (isShader) {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::dstIn(src[i], dest[i]);
    //     }   
    // }
    // else {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::dstIn(*src, dest[i]);
    //     }
    // }
    for (int i = 0; i < count; i++) {
        dest[i] = ZBlendMode::dstIn(isShader ? src[i] : *src, dest[i]);
    } 
}

static void srcOutRow(GPixel* src, GPixel* dest, int count, bool isShader) {
    // if (isShader) {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::srcOut(src[i], dest[i]);
    //     }   
    // }
    // else {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::srcOut(*src, dest[i]);
    //     }
    // }
    for (int i = 0; i < count; i++) {
        dest[i] = ZBlendMode::srcOut(isShader ? src[i] : *src, dest[i]);
    } 
}

static void dstOutRow(GPixel* src, GPixel* dest, int count, bool isShader) {
    // if (isShader) {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::dstOut(src[i], dest[i]);
    //     }   
    // }
    // else {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::dstOut(*src, dest[i]);
    //     }
    // }
    for (int i = 0; i < count; i++) {
        dest[i] = ZBlendMode::dstOut(isShader ? src[i] : *src, dest[i]);
    } 
}

static void srcATopRow(GPixel* src, GPixel* dest, int count, bool isShader) {
    // if (isShader) {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::srcATop(src[i], dest[i]);
    //     }   
    // }
    // else {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::srcATop(*src, dest[i]);
    //     }
    // }
    for (int i = 0; i < count; i++) {
        dest[i] = ZBlendMode::srcATop(isShader ? src[i] : *src, dest[i]);
    } 
}

static void dstATopRow(GPixel* src, GPixel* dest, int count, bool isShader) {
    // if (isShader) {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::dstATop(src[i], dest[i]);
    //     }   
    // }
    // else {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::dstATop(*src, dest[i]);
    //     }
    // }
    for (int i = 0; i < count; i++) {
        dest[i] = ZBlendMode::dstATop(isShader ? src[i] : *src, dest[i]);
    } 
}

static void xOrRow(GPixel* src, GPixel* dest, int count, bool isShader) {
    // if (isShader) {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::xOr(src[i], dest[i]);
    //     }   
    // }
    // else {
    //     for (int i = 0; i < count; i++) {
    //         dest[i] = ZBlendMode::xOr(*src, dest[i]);
    //     }
    // }
    for (int i = 0; i < count; i++) {
        dest[i] = ZBlendMode::xOr(isShader ? src[i] : *src, dest[i]);
    } 
}   