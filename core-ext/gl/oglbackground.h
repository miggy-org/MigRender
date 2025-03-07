#pragma once

#include "model.h"

#include "oglcommon.h"
#include "oglprogram.h"

_MIGRENDER_BEGIN

class OglBackground
{
private:
	GLuint _vertexArray;
	GLuint _bgTextureId;

	OglProgramBase* _bgProgram;

public:
	OglBackground(void);

	void Init(void);
	void Clear(void);

	void Load(const CModel& model);

	void Render(const CModel& model);
};

_MIGRENDER_END
