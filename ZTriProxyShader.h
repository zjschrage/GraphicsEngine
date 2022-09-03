#include "GColor.h"
#include "GPoint.h"
#include "GShader.h"

std::unique_ptr<GShader> GCreateTriProxyShader(const GPoint pts[], const GPoint texs[], GShader* shader);