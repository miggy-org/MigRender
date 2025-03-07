#pragma once

#include "oglcommon.h"

_MIGRENDER_BEGIN

#define MAX_NUMBER_OF_LIGHTS  8
#define MAX_TEXTURES          8

#define TEX_TYPE_NONE          0
#define TEX_TYPE_DIFFUSE       1
#define TEX_TYPE_SPECULAR      2
#define TEX_TYPE_TRANSPARENCY  3
#define TEX_TYPE_GLOW          4
#define TEX_TYPE_BUMP          5
#define TEX_TYPE_REFLECTION    6

#define TEX_OP_MULTIPLY  1
#define TEX_OP_ADD       2
#define TEX_OP_SUBTRACT  3
#define TEX_OP_BLEND     4

#define TEX_FLAG_INVERT_COLOR 1
#define TEX_FLAG_INVERT_ALPHA 2

#define LIGHT_TYPE_OFF   0
#define LIGHT_TYPE_DIR   1
#define LIGHT_TYPE_POINT 2

enum class OglProgramType
{
	Invalid,
	Main,
	Bg
};

class OglShader
{
private:
	GLuint _type;
	GLuint _shader;

public:
	OglShader(void);
	OglShader(GLuint type, const std::string& src);

	void Init(GLuint type, const std::string& src);

	GLuint GetShader(void) const { return _shader; }
	GLuint GetType(void) const { return _type; }
};

struct OglTextureBlock
{
	GLint type;
	GLint coordIndex;
	GLint samplerIndex;
	GLint op;
	GLint flags;
};

class OglProgramBase
{
protected:
	GLuint _program;

	std::map<std::string, GLint> _mapAttributes;
	std::map<std::string, GLint> _mapUniforms;

protected:
	bool IsValidAttribute(const std::string& name);
	bool IsValidUniform(const std::string& name);

public:
	OglProgramBase(void);
	virtual ~OglProgramBase(void);

	virtual const OglProgramBase& BeginProgram(void);
	virtual const OglProgramBase& AddShader(const OglShader& shader) const;
	virtual void LinkProgram(void) const;
	virtual void DeleteProgram(void);
	virtual void UseProgram(void) const;

	void SetUniform4f(const std::string& name, double v0, double v1, double v2, double v3);
	void SetUniform3f(const std::string& name, double v0, double v1, double v2);
	void SetUniform2f(const std::string& name, double v0, double v1);
	void SetUniform1f(const std::string& name, double v0);
	void SetUniform4i(const std::string& name, int v0, int v1, int v2, int v3);
	void SetUniform3i(const std::string& name, int v0, int v1, int v2);
	void SetUniform2i(const std::string& name, int v0, int v1);
	void SetUniform1i(const std::string& name, int v0);
	void SetUniformMatrix(const std::string& name, bool transpose, const CMatrix& mat);

	GLint GetAttributePosition(const std::string& name, bool required = true);
	GLint GetUniformPosition(const std::string& name, bool required = true);

};

class OglMainProgram : public OglProgramBase
{
protected:

public:
	OglMainProgram(void);

	GLint GetVertexPosition(void);
	GLint GetVertexNormalPosition(void);
	GLint GetVertexTexPosition(int index);

	void LoadModelGlobals(const CModel& model);
	void LoadLight(int index, GLint type, const CPt& dirPoint, const COLOR& color);
	void LoadMatrices(const CMatrix& modelMatrix);
	void LoadObject(const CObj& obj);
	void LoadTextureSamplers(const std::vector<GLuint> textureIds);
	void LoadTextureBlocks(const std::vector<OglTextureBlock> textureBlocks);
	void LoadNormal(const CPt& norm);
};

_MIGRENDER_END
