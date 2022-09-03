/**
 *  Copyright 2022 Zack Schrage
 */

#include "GPath.h"

static float cLambda = 0.551915;

GPath& GPath::addRect(const GRect& r, Direction dir) {
    moveTo(r.fLeft, r.fTop);
    if (dir == kCW_Direction) {
        lineTo(r.fRight, r.fTop);
        lineTo(r.fRight, r.fBottom);
        lineTo(r.fLeft, r.fBottom);
    }
    else {
        lineTo(r.fLeft, r.fBottom);
        lineTo(r.fRight, r.fBottom);
        lineTo(r.fRight, r.fTop);
    }
    return *this;
}

GPath& GPath::addPolygon(const GPoint pts[], int count) {
    moveTo(pts[0]);
    for (int i = 1; i < count; i++) {
        lineTo(pts[i]);
    }
    return *this;
}

GPath& GPath::addCircle(GPoint center, float radius, Direction dir) {
    float lambda = cLambda * radius;
    GPoint A = GPoint::Make(center.x(), center.y() - radius);
    GPoint B = GPoint::Make(center.x() + lambda, center.y() - radius);
    GPoint C = GPoint::Make(center.x() + radius, center.y() - lambda);
    GPoint D = GPoint::Make(center.x() + radius, center.y());
    GPoint E = GPoint::Make(center.x() + radius, center.y() + lambda);
    GPoint F = GPoint::Make(center.x() + lambda, center.y() + radius);
    GPoint G = GPoint::Make(center.x(), center.y() + radius);
    GPoint H = GPoint::Make(center.x() - lambda, center.y() + radius);
    GPoint I = GPoint::Make(center.x() - radius, center.y() + lambda);
    GPoint J = GPoint::Make(center.x() - radius, center.y());
    GPoint K = GPoint::Make(center.x() - radius, center.y() - lambda);
    GPoint L = GPoint::Make(center.x() - lambda, center.y() - radius);
    moveTo(A.x(), A.y());
    if (dir == kCW_Direction) {
        cubicTo(B.x(), B.y(), C.x(), C.y(), D.x(), D.y());
        cubicTo(E.x(), E.y(), F.x(), F.y(), G.x(), G.y());
        cubicTo(H.x(), H.y(), I.x(), I.y(), J.x(), J.y());
        cubicTo(K.x(), K.y(), L.x(), L.y(), A.x(), A.y());
    }
    else {
        cubicTo(L.x(), L.y(), K.x(), K.y(), J.x(), J.y());
        cubicTo(I.x(), I.y(), H.x(), H.y(), G.x(), G.y());
        cubicTo(F.x(), F.y(), E.x(), E.y(), D.x(), D.y());
        cubicTo(C.x(), C.y(), B.x(), B.y(), A.x(), A.y());
    }
    return *this;
}

GRect GPath::bounds() const {
    float xMin = FLT_MAX;
    float yMin = FLT_MAX;
    float xMax = FLT_MIN;
    float yMax = FLT_MIN;
    for (GPoint p: fPts) {
        if (p.fX < xMin) xMin = p.fX;
        if (p.fX > xMax) xMax = p.fX;
        if (p.fY < yMin) yMin = p.fY;
        if (p.fY > yMax) yMax = p.fY;
    }
    return GRect::LTRB(xMin, yMin, xMax, yMax);
}

void GPath::transform(const GMatrix& m) {
    int n = fPts.size();
    GPoint pts[n];
    GPoint transformed[n];
    for (int i = 0; i < n; i++) {
        pts[i] = fPts[i];
    }
    m.mapPoints(transformed, pts, n);
    for (int i = 0; i < n; i++) {
        fPts[i] = transformed[i];
    }
}

GPoint interpolate(GPoint p0, GPoint p1, float t) {
    float newX = p0.x() * (1-t) + p1.x() * (t);
    float newY = p0.y() * (1-t) + p1.y() * (t);
    return GPoint::Make(newX, newY);
}

void GPath::ChopQuadAt(const GPoint src[3], GPoint dst[5], float t) {
    dst[0] = src[0]; //A
    dst[4] = src[2]; //C
    dst[1] = interpolate(src[0], src[1], t); //interpolate A & B
    dst[3] = interpolate(src[1], src[2], t); //interpolate B & C
    dst[2] = interpolate(dst[1], dst[3], t); // interpolate AB & BC
}

void GPath::ChopCubicAt(const GPoint src[4], GPoint dst[7], float t) {
    dst[0] = src[0]; //A
    dst[6] = src[3]; //D
    dst[1] = interpolate(src[0], src[1], t); //interpolate A & B
    dst[5] = interpolate(src[2], src[3], t); //interpolate C & D
    GPoint bc = interpolate(src[1], src[2], t); //interpolate B & C
    dst[2] = interpolate(dst[1], bc, t); //interpolate AB & BC
    dst[4] = interpolate(bc, dst[5], t); //interpolate BC & CD
    dst[3] = interpolate(dst[2], dst[4], t); //interpolate ABBC & BCCD
}