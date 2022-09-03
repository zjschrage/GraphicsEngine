/**
 *  Copyright 2022 Zack Schrage
 */

#include "GPoint.h"

typedef struct Edge {
    int w;
    float m;
    float b;
    int left;
    int top;
    int bottom;
} Edge;

static Edge createEdge(GPoint p1, GPoint p2, float w, float m, float b);
static Edge createEdge(GPoint p1, GPoint p2, int w);

static Edge createEdge(GPoint p1, GPoint p2, int w) {
    float m = (p2.x() - p1.x())/(p2.y() - p1.y());
    float b = p1.x() - (m * p1.y());
    return createEdge(p1, p2, w, m, b);
}

static Edge createEdge(GPoint p1, GPoint p2, float w, float m, float b) {
    Edge e;
    e.w = w;
    e.m = m;
    e.b = b;
    e.left = GRoundToInt(std::min(p1.x(), p2.x()));
    e.top = GRoundToInt(std::min(p1.y(), p2.y()));
    e.bottom = GRoundToInt(std::max(p1.y(), p2.y()));
    return e;
}