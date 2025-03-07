#pragma once

#define GLEW_STATIC

#include "../../lib/glew/include/GL/glew.h"
//#include "GL/gl.h"
//#include "GL/glu.h"

#include "defines.h"
#include "matrix.h"
#include "image.h"

void CheckGLError(const char* szError);

double ConvertToRadians(double degrees);

const GLfloat* ConvertMatrixToFloat(const MigRender::CMatrix& tm);

GLuint LoadTextureMap(const MigRender::CImageBuffer* pbuf);
