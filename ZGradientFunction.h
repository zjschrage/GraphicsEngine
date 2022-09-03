/**
 * Zack Schrage 2022
 */

typedef float (*GradientFunction) (float, float);

static float linear(float x, float y);
static float radial(float x, float y);
static float angular(float x, float y);

static float linear(float x, float y) {
    return x;
}

static float radial(float x, float y) {
    return std::sqrt(x*x + y*y);
}

static float angular(float x, float y) {
    return (atan2(y, x) + M_PI)/ (2 * M_PI);
}