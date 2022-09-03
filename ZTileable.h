/**
 * Zack Schrage 2022
 */

typedef float (*TileFunction) (float);

static float clamp(float t);
static float repeat(float t);
static float mirror(float t);

static float almostOne = 0.999f;

static float clamp(float t) {
    if (t > almostOne) return almostOne;
    else if (t < 0) return 0;
    else return t;
}

static float repeat(float t) {
    return t - floor(t);
}

static float mirror(float t) {
    t *= 0.5;
    t = t - floor(t);
    t *= 2;
    if (t < 1) return t;
    else return 2-t;
}