/**
 * Copyright 2022 Zack Schrage
 */

#include "GPoint.h"

typedef GPoint (*BezierFunction) (GPoint[], float);
static GPoint quadBezier(GPoint pts[], float t);
static GPoint cubicBezier(GPoint pts[], float t);
static int numberOfQuadSegments(GPoint pts[]);
static int numberOfCubicSegments(GPoint pts[]);

static float tolerance = 0.25;

enum NumberOfPoints {
    kQuadNumber = 3,
    kCubicNumber = 4,
};

static GPoint quadBezier(GPoint pts[], float t) {
    GPoint A = pts[0];
    GPoint B = pts[1];
    GPoint C = pts[2];
    float x = (((A.x() - 2*B.x() + C.x())*t + (2*B.x() - 2*A.x()))*t) + A.x();
    float y = (((A.y() - 2*B.y() + C.y())*t + (2*B.y() - 2*A.y()))*t) + A.y();
    return GPoint::Make(x, y);
}

static GPoint cubicBezier(GPoint pts[], float t) {
    GPoint A = pts[0];
    GPoint B = pts[1];
    GPoint C = pts[2];
    GPoint D = pts[3];
    float x = ((((D.x() - 3*C.x() + 3*B.x() - A.x())*t + (3*C.x() - 6*B.x() + 3*A.x()))*t + (3*B.x() - 3*A.x()))*t) + A.x();
    float y = ((((D.y() - 3*C.y() + 3*B.y() - A.y())*t + (3*C.y() - 6*B.y() + 3*A.y()))*t + (3*B.y() - 3*A.y()))*t) + A.y();
    return GPoint::Make(x, y);
}

static int numberOfQuadSegments(GPoint pts[]) {
    GPoint A = pts[0];
    GPoint B = pts[1];
    GPoint C = pts[2];
    float x = (A.x() - 2 * B.x() + C.x())/2;
    float y = (A.y() - 2 * B.y() + C.y())/2;
    return (unsigned) std::sqrt(std::sqrt(x*x + y*y) / tolerance);
}

static int numberOfCubicSegments(GPoint pts[]) {
    GPoint A = pts[0];
    GPoint B = pts[1];
    GPoint C = pts[2];
    GPoint D = pts[3];
    float x1 = (A.x() - 2 * B.x() + C.x())/2;
    float y1 = (A.y() - 2 * B.y() + C.y())/2;
    float x2 = (B.x() - 2 * C.x() + D.x())/2;
    float y2 = (B.y() - 2 * B.y() + D.y())/2;
    float x = std::max(std::abs(x1), std::abs(x2));
    float y = std::max(std::abs(y1), std::abs(y2));
    return (unsigned) std::sqrt((3 * std::sqrt(x*x + y*y)) / (4 * tolerance));
}