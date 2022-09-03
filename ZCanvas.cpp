/**
 *  Copyright 2022 Zack Schrage
 */

#include "GCanvas.h"
#include "GBitmap.h"
#include "GRect.h"
#include "GPixel.h"
#include "GColor.h"
#include "GMath.h"
#include "GPath.h"
#include "GShader.h"
#include "ZBlendMode.h"
#include "ZEdge.h"
#include "ZBezier.h"
#include "ZPath.h"
#include "ZTriColorShader.h"
#include "ZTriProxyShader.h"
#include "ZComposedShader.h"
#include "ZGradient.h"

#include <vector>
#include <stack>
#include <functional>
#include <stdio.h>
#include <iostream>

class ZCanvas : public GCanvas {

public:

    ZCanvas(const GBitmap& device) : fDevice(device) {
        tmStack.push(GMatrix());
    }

    void drawPaint(const GPaint& paint) override {
        GShader* shader = paint.getShader();
        GColor color = paint.getColor();
        GPixel src = GPixel_PackARGB(GRoundToInt(255 * color.a), GRoundToInt(255 * color.a * color.r), GRoundToInt(255 * color.a * color.g), GRoundToInt(255 * color.a * color.b));
        int alpha = GPixel_GetA(src);
        void (*blitFunction) (const GBitmap&, const GPaint&, BlendFunction, int, int, int);
        blitFunction = &blitDefault;
        if (shader != nullptr) {
            if (!shader->setContext(tmStack.top())) return;
            if (!shader->isOpaque()) alpha = 1; //Stub alpha
            blitFunction = &blitShader;
        }
        BlendFunction b = pickBlend(paint.getBlendMode(), alpha);
        for (int i = 0; i < fDevice.height(); i++) {
            blitFunction(fDevice, paint, b, 0, fDevice.width(), i);
        }
    }

    void drawRect(const GRect& rect, const GPaint& paint) override {
        GPoint points[4];
        points[0] = GPoint::Make(rect.fLeft, rect.fTop);
        points[1] = GPoint::Make(rect.fRight, rect.fTop);
        points[2] = GPoint::Make(rect.fRight, rect.fBottom);
        points[3] = GPoint::Make(rect.fLeft, rect.fBottom);
        drawConvexPolygon(points, 4, paint);
        return;

        GColor color = paint.getColor();
        GPixel src = GPixel_PackARGB(GRoundToInt(255 * color.a), GRoundToInt(255 * color.a * color.r), GRoundToInt(255 * color.a * color.g), GRoundToInt(255 * color.a * color.b));
        GRect intersect = intersection(rect, GRect::WH(fDevice.width(), fDevice.height()));
        roundRectangle(intersect);
        void (*blitFunction) (const GBitmap&, const GPaint&, BlendFunction, int, int, int);
        blitFunction = &blitDefault;
        for (int i = intersect.fTop; i < intersect.fBottom; i++) {
            blitFunction(fDevice, paint, pickBlend(paint.getBlendMode(), GPixel_GetA(src)), intersect.fLeft, intersect.fRight, i);
        }
    }

    void drawConvexPolygon(const GPoint points[], int count, const GPaint& paint) override {
        GShader* shader = paint.getShader();
        GColor color = paint.getColor();
        GPixel src = GPixel_PackARGB(GRoundToInt(255 * color.a), GRoundToInt(255 * color.a * color.r), GRoundToInt(255 * color.a * color.g), GRoundToInt(255 * color.a * color.b));
        GPoint tPoints[count];
        tmStack.top().mapPoints(tPoints, points, count);
        int alpha = GPixel_GetA(src);
        void (*blitFunction) (const GBitmap&, const GPaint&, BlendFunction, int, int, int);
        blitFunction = &blitDefault;
        if (shader != nullptr) {
            if (!shader->setContext(tmStack.top())) return;
            if (!shader->isOpaque()) alpha = 1; //Stub alpha
            blitFunction = &blitShader;
        }
        BlendFunction b = pickBlend(paint.getBlendMode(), alpha);

        std::vector<Edge> edges = generateEdges(tPoints, count, GRect::WH(fDevice.width(), fDevice.height()));
        if (edges.size() < 2) return;
        std::sort(edges.begin(), edges.end(), sortLambdaFunction);
        int upperBound = edges[0].top;
        int lowerBound = getLowerBound(edges);
        int leftIdx = 0;
        int rightIdx = 1;
        for (int i = upperBound; i < lowerBound; i++) {
            int left = GRoundToInt((edges.at(leftIdx).m * ((float)i+0.5)) + edges.at(leftIdx).b);
            int right = GRoundToInt((edges.at(rightIdx).m * ((float)i+0.5)) + edges.at(rightIdx).b);
            blitFunction(fDevice, paint, b, left, right, i);
            if (edges.at(leftIdx).bottom <= i + 1) leftIdx = std::max(leftIdx, rightIdx) + 1;
            if (edges.at(rightIdx).bottom <= i + 1) rightIdx = std::max(leftIdx, rightIdx) + 1;
        }
    }

    void drawPath(const GPath& path, const GPaint& paint) override {
        GShader* shader = paint.getShader();
        GColor color = paint.getColor();
        GPixel src = GPixel_PackARGB(GRoundToInt(255 * color.a), GRoundToInt(255 * color.a * color.r), GRoundToInt(255 * color.a * color.g), GRoundToInt(255 * color.a * color.b));
        int alpha = GPixel_GetA(src);
        void (*blitFunction) (const GBitmap&, const GPaint&, BlendFunction, int, int, int);
        blitFunction = &blitDefault;
        if (shader != nullptr) {
            if (!shader->setContext(tmStack.top())) return;
            if (!shader->isOpaque()) alpha = 1; //Stub alpha
            blitFunction = &blitShader;
        }
        BlendFunction b = pickBlend(paint.getBlendMode(), alpha);

        std::vector<Edge> edges;
        GPoint pts[GPath::kMaxNextPoints];
        GPath pathCpy = path;
        pathCpy.transform(tmStack.top());
        GPath::Edger edger(pathCpy);
        GPath::Verb v;
        while ((v = edger.next(pts)) != GPath::kDone) {
            switch(v) {
                case GPath::kLine:
                    clipper(pts[0], pts[1], GRect::WH(fDevice.width(), fDevice.height()), edges);
                    break;
                case GPath::kQuad:
                    optimizeCurve(pts, NumberOfPoints::kQuadNumber, &quadBezier, &GPath::ChopQuadAt, GRect::WH(fDevice.width(), fDevice.height()), numberOfQuadSegments(pts), 0, 2, edges);
                    break;
                case GPath::kCubic:
                    optimizeCurve(pts, NumberOfPoints::kCubicNumber, &cubicBezier, &GPath::ChopCubicAt, GRect::WH(fDevice.width(), fDevice.height()), numberOfCubicSegments(pts), 0, 2, edges);
                    break;
                default:
                    break;
            }
        }
        if (edges.size() < 2) return;
        std::sort(edges.begin(), edges.end(), sortLambdaFunction);

        int upperBound = edges[0].top;
        int lowerBound = getLowerBound(edges);
        int numActiveEdges = 0;
        for (int y = upperBound; y < lowerBound; y++) {
            int w = 0;
            int left = 0;
            int right = 0;
            numActiveEdges = manageActiveEdges(edges, y);
            std::sort(edges.begin(), edges.begin() + numActiveEdges, [y](const Edge& a, const Edge& b) { 
                return sortXLambdaFunction(a, b, y); 
            });
            for (int e = 0; e < numActiveEdges; e++) {
                int x = GRoundToInt(edges[e].m*(y + 0.5) + edges[e].b);
                if (w == 0) left = x;
                w += edges[e].w;
                if (w == 0) {
                    right = x;
                    blitFunction(fDevice, paint, b, left, right, y);
                }
            }
        }
    }

    void drawMesh(const GPoint verts[], const GColor colors[], const GPoint texs[], int count, const int indices[], const GPaint& paint) override {
        GPoint myVerts[3];
        GColor myColors[3];
        GPoint myTexs[3];
        std::unique_ptr<GShader> shader;
        std::unique_ptr<GShader> triColorShader;
        std::unique_ptr<GShader> triProxyShader;
        for (int i = 0; i < count; i++) {
            myVerts[0] = verts[indices[3*i]];
            myVerts[1] = verts[indices[3*i+1]];
            myVerts[2] = verts[indices[3*i+2]];
            if (colors != nullptr && texs != nullptr) {
                myColors[0] = colors[indices[3*i]];
                myColors[1] = colors[indices[3*i+1]];
                myColors[2] = colors[indices[3*i+2]];
                myTexs[0] = texs[indices[3*i]];
                myTexs[1] = texs[indices[3*i+1]];
                myTexs[2] = texs[indices[3*i+2]];
                triColorShader = GCreateTriColorShader(myVerts, myColors);
                triProxyShader = GCreateTriProxyShader(myVerts, myTexs, paint.getShader());
                shader = GCreateComposedShader(triColorShader.get(), triProxyShader.get());
            }
            else if (colors != nullptr) {
                myColors[0] = colors[indices[3*i]];
                myColors[1] = colors[indices[3*i+1]];
                myColors[2] = colors[indices[3*i+2]];
                shader = GCreateTriColorShader(myVerts, myColors);
            }
            else if (texs != nullptr) {
                myTexs[0] = texs[indices[3*i]];
                myTexs[1] = texs[indices[3*i+1]];
                myTexs[2] = texs[indices[3*i+2]];
                shader = GCreateTriProxyShader(myVerts, myTexs, paint.getShader());
            }
            else return;

            GPath path;
            path.moveTo(myVerts[0]);
            path.lineTo(myVerts[1]);
            path.lineTo(myVerts[2]);
            drawPath(path, GPaint(shader.get()));

        }
    }

    void drawQuad(const GPoint verts[4], const GColor colors[4], const GPoint texs[4], int level, const GPaint& paint) override {
        float dWeight = 1.0/(level+1);
        GPoint allPoints[level+2][level+2];
        GColor allColors[level+2][level+2];
        GPoint allTextures[level+2][level+2];
        float weightVert = 0;
        for (int i = 0; i < level + 2; i++) {
            GPoint a = interpolatePoints(verts[0], verts[1], weightVert);
            GPoint b = interpolatePoints(verts[3], verts[2], weightVert);
            GColor colorA;
            GColor colorB;
            if (colors != nullptr) {
                colorA = GColor::RGBA(colors[0].r * (1.0 - weightVert) + (colors[1].r * weightVert), colors[0].g * (1.0 - weightVert) + (colors[1].g * weightVert), colors[0].b * (1.0 - weightVert) + (colors[1].b * weightVert), colors[0].a * (1.0 - weightVert) + (colors[1].a * weightVert));
                colorB = GColor::RGBA(colors[3].r * (1.0 - weightVert) + (colors[2].r * weightVert), colors[3].g * (1.0 - weightVert) + (colors[2].g * weightVert), colors[3].b * (1.0 - weightVert) + (colors[2].b * weightVert), colors[3].a * (1.0 - weightVert) + (colors[2].a * weightVert));
            }
            GPoint texsA;
            GPoint texsB;
            if (texs != nullptr) {
                texsA = interpolatePoints(texs[0], texs[1], weightVert);
                texsB = interpolatePoints(texs[3], texs[2], weightVert);
            }
            if (i == (level + 1)) {
                a = GPoint::Make(verts[1].x(), verts[1].y());
                b = GPoint::Make(verts[2].x(), verts[2].y());
                if (colors != nullptr) {
                    colorA = GColor::RGBA(colors[1].r, colors[1].g, colors[1].b, colors[1].a);
                    colorB = GColor::RGBA(colors[2].r, colors[2].g, colors[2].b, colors[2].a);
                }
                if (texs != nullptr) {
                    texsA = GPoint::Make(texs[1].x(), texs[1].y());
                    texsB = GPoint::Make(texs[2].x(), texs[2].y());
                }
            }
            if (colors != nullptr) {
                colorA = colorA.pinToUnit();
                colorB = colorB.pinToUnit();
            }

            float weight = 0;
            for (int j = 0; j < level + 1; j++) {
                allPoints[i][j] = interpolatePoints(a, b, weight);
                if (colors != nullptr) {
                    allColors[i][j] = GColor::RGBA(colorA.r * (1.0-weight) + (colorB.r * weight), colorA.g * (1.0-weight) + (colorB.g * weight), colorA.b * (1.0-weight) + (colorB.b * weight), colorA.a * (1.0-weight) + (colorB.a * weight));
                    allColors[i][j] = allColors[i][j].pinToUnit();
                }
                if (texs != nullptr) {
                    allTextures[i][j] = interpolatePoints(texsA, texsB, weight);
                }
                weight += dWeight;
            }
            allPoints[i][level + 1] = GPoint::Make(b.x(), b.y());
            if (colors != nullptr) {
                allColors[i][level + 1] = GColor::RGBA(colorB.r, colorB.g, colorB.b, colorB.a);
                allColors[i][level + 1] = allColors[i][level + 1].pinToUnit();
            }
            if (texs != nullptr) {
                allTextures[i][level + 1] = GPoint::Make(texsB.x(), texsB.y());
            }
            weightVert += dWeight;
        }

        //Draw Triangles
        for (int i = 0; i < level + 1; i++) {
            for (int j = 0; j < level + 1; j++) {
                int indices[3];
                indices[0] = 0;
                indices[1] = 1;
                indices[2] = 2;

                //Upper Half Triangle
                GPoint verts1[3];
                verts1[0] = allPoints[i][j];
                verts1[1] = allPoints[i][j+1];
                verts1[2] = allPoints[i+1][j];
                GColor colors1[3];
                if (colors != nullptr) {
                    colors1[0] = allColors[i][j];
                    colors1[1] = allColors[i][j+1];
                    colors1[2] = allColors[i+1][j];
                }
                GPoint texs1[3];
                if (texs != nullptr) {
                    texs1[0] = allTextures[i][j];
                    texs1[1] = allTextures[i][j+1];
                    texs1[2] = allTextures[i+1][j];
                }

                //Lower Half Triangle
                GPoint verts2[3];
                verts2[0] = allPoints[i+1][j+1];
                verts2[1] = allPoints[i][j+1];
                verts2[2] = allPoints[i+1][j];
                GColor colors2[3];
                if (colors != nullptr) {
                    colors2[0] = allColors[i+1][j+1];
                    colors2[1] = allColors[i][j+1];
                    colors2[2] = allColors[i+1][j];
                }
                GPoint texs2[3];
                if (texs != nullptr) {
                    texs2[0] = allTextures[i+1][j+1];
                    texs2[1] = allTextures[i][j+1];
                    texs2[2] = allTextures[i+1][j];
                }
     
                drawMesh(verts1, colors ? colors1 : nullptr, texs ? texs1 : nullptr, 1, indices, paint);
                drawMesh(verts2, colors ? colors2 : nullptr, texs ? texs2 : nullptr, 1, indices, paint);
            }
        }
    }

    void drawStroke(const GPoint points[], int count, float thickness, CapType capType, BendType bendType, const GPaint& paint) override {
        GPath stroke;
        GVector prev = buildNormalVector(points[0], points[1]);
        GVector prevOrth = orthogonalizeVector(prev) * (thickness/2);
        addCapToStroke(stroke, GPoint::Make(points[0].x(), points[0].y()), prev, prevOrth, capType, thickness);
        for (int i = 0; i < count - 2; i++) {
            addRectangleToStroke(stroke, points[i], points[i+1], prevOrth);
            GVector curr = buildNormalVector(points[i+1], points[i+2]);
            GVector currOrth = orthogonalizeVector(curr) * (thickness/2);
            addJointToStroke(stroke, points[i+1], prev, prevOrth, curr, currOrth, bendType, thickness);
            prev = curr;
            prevOrth = currOrth;
        }
        addRectangleToStroke(stroke, points[count-2], points[count-1], prevOrth);
        addCapToStroke(stroke, GPoint::Make(points[count-1].x(), points[count-1].y()), prev, prevOrth, capType, thickness);
        drawPath(stroke, paint);
    }

    void concat(const GMatrix& matrix) override {
        tmStack.top() = GMatrix::Concat(tmStack.top(), matrix);
    }

    void restore() override {
        tmStack.pop();
    }

    void save() override {
        tmStack.push(tmStack.top());
    }

    //Helper Methods

    static void blitDefault(const GBitmap& fDevice, const GPaint& paint, BlendFunction b, int left, int right, int y) {
        if (left >= right) return;
        GPixel* p = fDevice.getAddr(left, y);
        GColor color = paint.getColor();
        GPixel src = GPixel_PackARGB(GRoundToInt(255 * color.a), GRoundToInt(255 * color.a * color.r), GRoundToInt(255 * color.a * color.g), GRoundToInt(255 * color.a * color.b));
        b(&src, p, right-left, false);
    }

    static void blitShader(const GBitmap& fDevice, const GPaint& paint, BlendFunction b, int left, int right, int y) {
        if (left >= right) return;
        GPixel newPixels[right-left];
        paint.getShader()->shadeRow(left, y, right-left, newPixels);
        GPixel* p = fDevice.getAddr(left, y);
        b(newPixels, p, right-left, true);
    }

    static GRect intersection(GRect r1, GRect r2) {
        return GRect::LTRB(std::max(r1.fLeft, r2.fLeft), std::max(r1.fTop, r2.fTop), std::min(r1.fRight, r2.fRight),  std::min(r1.fBottom, r2.fBottom));
    }

    static void roundRectangle(GRect& rect) {
        rect.fTop = GRoundToInt(rect.fTop);
        rect.fLeft = GRoundToInt(rect.fLeft);
        rect.fRight = GRoundToInt(rect.fRight);
        rect.fBottom = GRoundToInt(rect.fBottom);
    }

    static std::vector<Edge> generateEdges(const GPoint points[], int count, GRect bounds) {
        std::vector<Edge> edges;
        for (int i = 0; i < count - 1; i++) {
            clipper(points[i], points[i+1], bounds, edges);
        }
        clipper(points[0], points[count-1], bounds, edges);
        return edges;
    }

    static void clipper(GPoint p1, GPoint p2, GRect bounds, std::vector<Edge>& edges) {
        if (!isNotHorizontal(p1, p2)) return;
        int w = p1.fY < p2.fY ? 1 : -1;
        float m = (p2.x() - p1.x())/(p2.y() - p1.y());
        float b = p1.x() - (m * p1.y());
        //Let p1 be above p2
        if (p1.y() > p2.y()) {
            GPoint temp = p1;
            p1 = p2;
            p2 = temp;
        }
        //Top Cases
        if (p2.y() < bounds.fTop) return;
        if (p1.y() < bounds.fTop) p1.set(m*bounds.fTop + b, bounds.fTop);
        //Bottom Cases
        if (p1.y() >= bounds.fBottom) return;
        if (p2.y() >= bounds.fBottom) p2.set(m*bounds.fBottom + b, bounds.fBottom);
        //Let p1 be to the left of p2
        if (p1.x() > p2.x()) {
            GPoint temp = p1;
            p1 = p2;
            p2 = temp;
        }
        //Left Cases
        if (p2.x() < bounds.fLeft) {
            GPoint np1;
            GPoint np2;
            np1.set(bounds.fLeft, p1.y());
            np2.set(bounds.fLeft, p2.y());
            if (isNotHorizontal(np1, np2)) {
                edges.push_back(createEdge(np1, np2, w));
                return;
            }
        }
        else if (p1.x() < bounds.fLeft) {
            GPoint p3;
            p3.set(bounds.fLeft, p1.y());
            p1.set(bounds.fLeft, (bounds.fLeft - b)/m);
            if (isNotHorizontal(p1, p3)) edges.push_back(createEdge(p1, p3, w));
        }
        //Right Cases
        if (p1.x() >= bounds.fRight) {
            GPoint np1;
            GPoint np2;
            np1.set(bounds.fRight, p1.y());
            np2.set(bounds.fRight, p2.y());
            if (isNotHorizontal(np1, np2)) {
                edges.push_back(createEdge(np1, np2, w));
                return;
            }
        }
        else if (p2.x() >= bounds.fRight) {
            GPoint p3;
            p3.set(bounds.fRight, p2.y());
            p2.set(bounds.fRight, (bounds.fRight - b)/m);
            if (isNotHorizontal(p2, p3)) edges.push_back(createEdge(p2, p3, w));
        }
        if (isNotHorizontal(p1, p2)) {
            edges.push_back(createEdge(p1, p2, w, m, b));
        }
    }

    static void optimizeCurve(GPoint pts[], int numPts, BezierFunction bezierFunction, ChopperFunction chopperFunction, GRect bounds, int segments, int n, int nMax, std::vector<Edge> &edges) {
        if (n >= nMax || verticalBoundedness(pts, numPts, bounds, true)) {
            segmenter(pts, numPts, bezierFunction, bounds, segments >> n, edges);
            return;
        }
        else if (verticalBoundedness(pts, numPts, bounds, false)) {
            return;
        }

        GPoint halfCurves[2*numPts - 1];
        chopperFunction(pts, halfCurves, 0.5);

        GPoint left[numPts];
        for (int i = 0; i < numPts; i++) {
            left[i] = halfCurves[i];
        }
        GPoint right[numPts];
        for (int i = 0; i < numPts; i++) {
            right[i] = halfCurves[numPts - 1 + i];
        }

        optimizeCurve(left, numPts, bezierFunction, chopperFunction, bounds, segments, n++, nMax, edges);
        optimizeCurve(right, numPts, bezierFunction, chopperFunction, bounds, segments, n++, nMax, edges);
    }

    static void segmenter(GPoint pts[], int numPts, BezierFunction bezierFunction, GRect bounds, int segments, std::vector<Edge> &edges) {
        GPoint prev = pts[0];
        GPoint last = pts[numPts - 1];
        if (segments == 0) {
            clipper(prev, last, bounds, edges);
            return;
        }
        float t = 0.0;
        float dt = 1.0/segments;
        for (int i = 0; i < segments - 1; i++) {
            GPoint newPoint = bezierFunction(pts, t);
            clipper(prev, newPoint, bounds, edges);
            prev = newPoint;
            t += dt;
        }
        clipper(prev, last, bounds, edges);
    }

    static GVector buildNormalVector(GPoint p0, GPoint p1) {
        GVector vector;
        vector.set(p1.x() - p0.x(), p1.y() - p0.y());
        float invLength = 1 / vector.length();
        vector.set(vector.x() * invLength, vector.y() * invLength);
        return vector;
    }

    static GVector orthogonalizeVector(GVector vector) {
        GVector orthogonal;
        orthogonal.set(-vector.y(), vector.x());
        return orthogonal;
    }

    static void addRectangleToStroke(GPath& stroke, GPoint p0, GPoint p1, GVector constructionVector) {
        stroke.moveTo(p0.x() + constructionVector.x(), p0.y() + constructionVector.y());
        stroke.lineTo(p1.x() + constructionVector.x(), p1.y() + constructionVector.y());
        stroke.lineTo(p1.x() - constructionVector.x(), p1.y() - constructionVector.y());
        stroke.lineTo(p0.x() - constructionVector.x(), p0.y() - constructionVector.y());
    }

    static void addCapToStroke(GPath& stroke, GPoint p, GVector prev, GVector prevOrth, GCanvas::CapType capType, int thickness) {
        switch (capType) {
            case Circle:
                stroke.moveTo(p.x(), p.y());
                stroke.addCircle(GPoint::Make(p.x(), p.y()), thickness, GPath::kCCW_Direction);
                break;
            case Square:
                GVector reverseNormal;
                reverseNormal.set(-prev.x() * thickness, -prev.y() * thickness);
                stroke.moveTo(p.x() - prevOrth.x() + reverseNormal.x(), p.y() - prevOrth.y() + reverseNormal.y());
                stroke.lineTo(p.x() + prevOrth.x() + reverseNormal.x(), p.y() + prevOrth.y() + reverseNormal.y());
                stroke.lineTo(p.x() + prevOrth.x() - reverseNormal.x(), p.y() + prevOrth.y() - reverseNormal.y());
                stroke.lineTo(p.x() - prevOrth.x() - reverseNormal.x(), p.y() - prevOrth.y() - reverseNormal.y());
                break;
        }
    }

    static void addJointToStroke(GPath& stroke, GPoint p, GVector prev, GVector prevOrth, GVector curr, GVector currOrth, GCanvas::BendType bendType, int thickness) {
        float cross = (prev.x() * curr.y()) - (prev.y() * curr.x());
        switch (bendType) {
            case Rounded:
                stroke.moveTo(p.x(), p.y());
                stroke.addCircle(GPoint::Make(p.x(), p.y()), thickness/2, GPath::kCCW_Direction);
                break;
            case Bend:
                if (cross >= 0) {
                    stroke.moveTo(p.x(), p.y());
                    stroke.lineTo(p.x() - prevOrth.x(), p.y() - prevOrth.y());
                    stroke.lineTo(p.x() - currOrth.x(), p.y() - currOrth.y());
                }
                else {
                    stroke.moveTo(p.x(), p.y());
                    stroke.lineTo(p.x() + currOrth.x(), p.y() + currOrth.y());
                    stroke.lineTo(p.x() + prevOrth.x(), p.y() + prevOrth.y());
                }
                break;
            case Miter:
                GVector Q;
                Q.set(prev.x() - curr.x(), prev.y() - curr.y());
                Q = Q * (1 / Q.length());
                float dot = (prev.x() * curr.x()) + (prev.y() * curr.y());
                float q = (thickness/2) * std::sqrt(2.0/(1-dot));
                Q = Q * q;
                // stroke.moveTo(p.x() + Q.x(), p.y() + Q.y());
                // stroke.addCircle(GPoint::Make(p.x() + Q.x(), p.y() + Q.y()), thickness/3, GPath::kCCW_Direction);
                if (cross >= 0) {
                    stroke.moveTo(p.x(), p.y());
                    stroke.lineTo(p.x() - currOrth.x(), p.y() - currOrth.y());
                    stroke.lineTo(p.x() + Q.x(), p.y() + Q.y());
                    stroke.lineTo(p.x() - prevOrth.x(), p.y() - prevOrth.y());
                }
                else {
                    stroke.moveTo(p.x(), p.y());
                    stroke.lineTo(p.x() + prevOrth.x(), p.y() + prevOrth.y());
                    stroke.lineTo(p.x() + Q.x(), p.y() + Q.y());
                    stroke.lineTo(p.x() + currOrth.x(), p.y() + currOrth.y());
                }
                break;
        }
    }

    static int manageActiveEdges(std::vector<Edge>& edges, int y) {
        int count = 0;
        for (int i = 0; i < edges.size(); i++) {
            if (edges[i].top > y + 0.5) break;
            if (y + 0.5 >= edges[i].top && y + 0.5 <= edges[i].bottom) {
                edges[i].top = y;
                count++;
            }
            else {
                edges.erase(edges.begin() + i);
                i--;
            }
        }
        return count;
    }

    static bool sortLambdaFunction(Edge i, Edge j) { 
        if (i.top < j.top) return true;
        else if (i.top > j.top) return false; 
        else {
            if (i.left < j.left) return true;
            else if (i.left > j.left) return false;
            else return i.m < j.m;
        }
    }

    static bool sortXLambdaFunction(Edge i, Edge j, int y) {
        if (((i.m * (y+0.5)) + i.b) < ((j.m * (y+0.5)) + j.b)) return true;
        else return false;
    }

    static int getLowerBound(const std::vector<Edge>& edges) {
        int lower = edges[0].bottom;
        for (int i = 1; i < edges.size(); i++) {
            lower = std::max(lower, edges[i].bottom);
        }
        return lower;
    }

    static bool isNotHorizontal(GPoint p1, GPoint p2) {
        return GRoundToInt(p1.y()) != GRoundToInt(p2.y());
    }

    static bool verticalBoundedness(GPoint pts[], int numPts, GRect bounds, bool within) {
        for (int i = 0; i < numPts; i++) {
            bool inBounds = pts[i].y() < bounds.fBottom && pts[i].y() >= bounds.fTop;
            if (within && !inBounds) return false;
            else if (!within && inBounds) return false;
        }
        return true;
    }

    static GPoint interpolatePoints(GPoint a, GPoint b, float t) {
        return (a * (1-t)) + (b * t);
    }

private:
    
    const GBitmap fDevice; // Store a copy of the bitmap
    std::stack<GMatrix> tmStack; // Store a stack of transformation matrices

};

std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& device) {
    return std::unique_ptr<GCanvas>(new ZCanvas(device));
}

std::string GDrawSomething(GCanvas* canvas, GISize dim);

std::string GDrawSomething(GCanvas* canvas, GISize dim) {
    float centerX = dim.fWidth/2;
    float centerY = dim.fHeight/2;

    int gradDist = centerX*(5.0/4);
    float gradAlpha = 1.0;
    GColor shaderColors[7];
    shaderColors[0] = GColor::RGBA(1, 0, 0, gradAlpha);
    shaderColors[1] = GColor::RGBA(0, 1, 0, gradAlpha);
    shaderColors[2] = GColor::RGBA(0, 0, 1, gradAlpha);
    shaderColors[3] = GColor::RGBA(1, 1, 0, gradAlpha);
    shaderColors[4] = GColor::RGBA(0, 1, 1, gradAlpha);
    shaderColors[5] = GColor::RGBA(1, 0, 1, gradAlpha);
    shaderColors[6] = GColor::RGBA(1, 0, 0, gradAlpha);
    std::unique_ptr<GShader> gradient = GCreateGradient(GPoint::Make(centerX, centerY), GPoint::Make(centerX, centerY + gradDist), shaderColors, 7, GShader::kClamp, &angular);
    canvas->drawPaint(GPaint(gradient.get()));

    // Stroke Test Code 

    // GPoint pts[7];
    // pts[0] = GPoint::Make(10, 10);
    // pts[1] = GPoint::Make(30, 225);
    // pts[2] = GPoint::Make(70, 60);
    // pts[3] = GPoint::Make(111, 110);
    // pts[4] = GPoint::Make(200, 110);
    // pts[5] = GPoint::Make(230, 230);
    // pts[6] = GPoint::Make(240, 20);
    // canvas->drawStroke(pts, 6, 8, GCanvas::Square, GCanvas::Miter, GPaint(gradient.get()));

    // Gear Background Gradient

    // int gradDist = centerX*(5.0/4);
    // float gradAlpha = 1.0;
    // GColor shaderColors[2];
    // shaderColors[0] = GColor::RGBA(0, 0, 0, gradAlpha);
    // shaderColors[1] = GColor::RGBA(1, 1, 1, gradAlpha);
    // std::unique_ptr<GShader> gradient = GCreateGradient(GPoint::Make(centerX, centerY), GPoint::Make(centerX, centerY + gradDist), shaderColors, 2, GShader::kClamp, &radial);
    // canvas->drawRect(GRect::LTRB(0, 0, dim.fWidth, dim.fHeight), GPaint(gradient.get()));

    // //Gear

    // int iterations = 20;
    // GPoint pts[4];
    // pts[0] = GPoint::Make(-35, -20);
    // pts[1] = GPoint::Make(35, -20);
    // pts[2] = GPoint::Make(50, 40);
    // pts[3] = GPoint::Make(-50, 40);
    // GColor clr[4];
    // float alpha = 1.0;
    // clr[0] = GColor::RGBA(1, 0.2, 0.7, alpha);
    // clr[1] = GColor::RGBA(0, 1, 0.5, alpha);
    // clr[2] = GColor::RGBA(0, 0, 1, alpha);
    // clr[3] = GColor::RGBA(0.8, 1, 0.6, alpha);

    // canvas->translate(centerX, centerY);
    // float theta = 0;
    // float dTheta = 1.0/iterations;
    // float dAlpha = alpha/iterations;
    // float dRate = 0.5;
    // for (int i = 0; i < iterations; i++) {
    //     canvas->save();
    //     canvas->rotate(theta*2*M_PI);
    //     canvas->translate(centerX/4, centerY/4);
    //     canvas->drawQuad(pts, clr, nullptr, 8, GPaint());
    //     canvas->restore();
    //     for (int j = 0; j < 4; j++) {
    //         clr[j].a -= (dAlpha * dRate);
    //     }
    //     theta += dTheta;
    // }

    return "gear";
}