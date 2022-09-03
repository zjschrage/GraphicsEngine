#include "GColor.h"
#include "GPoint.h"
#include "GShader.h"

std::unique_ptr<GShader> GCreateComposedShader(GShader* triColorShader, GShader* triBMShader);