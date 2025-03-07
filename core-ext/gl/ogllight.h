#pragma once

#include "model.h"

#include "oglcommon.h"

_MIGRENDER_BEGIN

// deprecated, but kept around in case we need it later
class OglLight
{
public:
	OglLight(void);

	void Render(const CMatrix& itm, const CLight& lit, int lightIndex) const;
};

_MIGRENDER_END
