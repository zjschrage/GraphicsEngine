#include "GColor.h"
#include "GPoint.h"
#include "GShader.h"
#include "ZGradientFunction.h"

std::unique_ptr<GShader> GCreateGradient(GPoint p0, GPoint p1, const GColor color[], int count, GShader::TileMode tileMode, GradientFunction gradientFunction);