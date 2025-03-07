#include "oglrender.h"

#include "migexcept.h"

using namespace MigRender;

///////////////////////////////////////////////////////////////////////////////
// OglShader
//

OglShader::OglShader(void) : _type(-1), _shader(-1)
{
}

OglShader::OglShader(GLuint type, const std::string& src) : _type(-1), _shader(-1)
{
	Init(type, src);
}

void OglShader::Init(GLuint type, const std::string& src)
{
	const char* p = src.c_str();

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &p, NULL);
	glCompileShader(shader);

	int success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		throw mig_exception(infoLog);
	}

	_type = type;
	_shader = shader;
}

///////////////////////////////////////////////////////////////////////////////
// OglProgramBase
//

static bool IsValidPosition(GLint position)
{
	return (position != -1);
}

OglProgramBase::OglProgramBase(void) : _program(0)
{
}

OglProgramBase::~OglProgramBase(void)
{
	DeleteProgram();
}

bool OglProgramBase::IsValidAttribute(const std::string& name)
{
	return (GetAttributePosition(name, false) != -1);
}

bool OglProgramBase::IsValidUniform(const std::string& name)
{
	return (GetUniformPosition(name, false) != -1);
}

const OglProgramBase& OglProgramBase::BeginProgram(void)
{
	_program = glCreateProgram();
	return *this;
}

const OglProgramBase& OglProgramBase::AddShader(const OglShader& shader) const
{
	glAttachShader(_program, shader.GetShader());
	return *this;
}

void OglProgramBase::LinkProgram(void) const
{
	glLinkProgram(_program);

	int success = 0;
	glGetProgramiv(_program, GL_LINK_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetProgramInfoLog(_program, 512, NULL, infoLog);
		throw mig_exception(infoLog);
	}
}

void OglProgramBase::DeleteProgram(void)
{
	if (_program != 0)
		glDeleteProgram(_program);
	_program = 0;
}

void OglProgramBase::UseProgram(void) const
{
	glUseProgram(_program);
	CheckGLError("Use program error");
}

void OglProgramBase::SetUniform4f(const std::string& name, double v0, double v1, double v2, double v3)
{
	GLint pos = GetUniformPosition(name);
	glUniform4f(pos, static_cast<GLfloat>(v0), static_cast<GLfloat>(v1), static_cast<GLfloat>(v2), static_cast<GLfloat>(v3));
}

void OglProgramBase::SetUniform3f(const std::string& name, double v0, double v1, double v2)
{
	GLint pos = GetUniformPosition(name);
	glUniform3f(pos, static_cast<GLfloat>(v0), static_cast<GLfloat>(v1), static_cast<GLfloat>(v2));
}

void OglProgramBase::SetUniform2f(const std::string& name, double v0, double v1)
{
	GLint pos = GetUniformPosition(name);
	glUniform2f(pos, static_cast<GLfloat>(v0), static_cast<GLfloat>(v1));
}

void OglProgramBase::SetUniform1f(const std::string& name, double v0)
{
	GLint pos = GetUniformPosition(name);
	glUniform1f(pos, static_cast<GLfloat>(v0));
}

void OglProgramBase::SetUniform4i(const std::string& name, int v0, int v1, int v2, int v3)
{
	GLint pos = GetUniformPosition(name);
	glUniform4i(pos, static_cast<GLint>(v0), static_cast<GLint>(v1), static_cast<GLint>(v2), static_cast<GLint>(v3));
}

void OglProgramBase::SetUniform3i(const std::string& name, int v0, int v1, int v2)
{
	GLint pos = GetUniformPosition(name);
	glUniform3i(pos, static_cast<GLint>(v0), static_cast<GLint>(v1), static_cast<GLint>(v2));
}

void OglProgramBase::SetUniform2i(const std::string& name, int v0, int v1)
{
	GLint pos = GetUniformPosition(name);
	glUniform2i(pos, static_cast<GLint>(v0), static_cast<GLint>(v1));
}

void OglProgramBase::SetUniform1i(const std::string& name, int v0)
{
	GLint pos = GetUniformPosition(name);
	glUniform1i(pos, static_cast<GLint>(v0));
}

void OglProgramBase::SetUniformMatrix(const std::string& name, bool transpose, const CMatrix& mat)
{
	GLint pos = GetUniformPosition(name);
	glUniformMatrix4fv(pos, 1, (transpose ? GL_TRUE : GL_FALSE), ConvertMatrixToFloat(mat));
}

GLint OglProgramBase::GetAttributePosition(const std::string& name, bool required)
{
	const auto& iter = _mapAttributes.find(name);
	if (iter == _mapAttributes.end())
	{
		GLint position = glGetAttribLocation(_program, name.c_str());
		if (position == -1 && required)
			throw mig_exception("Invalid attribute position");
		_mapAttributes[name] = position;
		return position;
	}
	return iter->second;
}

GLint OglProgramBase::GetUniformPosition(const std::string& name, bool required)
{
	const auto& iter = _mapUniforms.find(name);
	if (iter == _mapUniforms.end())
	{
		GLint position = glGetUniformLocation(_program, name.c_str());
		if (position == -1 && required)
			throw mig_exception("Invalid uniform position");
		_mapUniforms[name] = position;
		return position;
	}
	return iter->second;
}

///////////////////////////////////////////////////////////////////////////////
// OglMainProgram
//

OglMainProgram::OglMainProgram(void)
{
}

GLint OglMainProgram::GetVertexPosition(void)
{
	return GetAttributePosition("vPosition", true);
}

GLint OglMainProgram::GetVertexNormalPosition(void)
{
	return GetAttributePosition("vNormal", true);
}

GLint OglMainProgram::GetVertexTexPosition(int index)
{
	return GetAttributePosition(std::string("vTex[" + std::to_string(index) + "]").c_str());
}

void OglMainProgram::LoadModelGlobals(const CModel& model)
{
	if (IsValidUniform("viewPos"))
	{
		CPt viewPos(0, 0, 0);
		model.GetCamera().GetTM().TransformPoint(viewPos, 1, MatrixType::CTM);
		SetUniform3f("viewPos", static_cast<GLfloat>(viewPos.x), static_cast<GLfloat>(viewPos.y), static_cast<GLfloat>(viewPos.z));
		CheckGLError("View position load error");
	}

	if (IsValidUniform("colAmbient"))
	{
		const COLOR& ambient = model.GetAmbientLight();
		SetUniform4f("colAmbient", static_cast<GLfloat>(ambient.r), static_cast<GLfloat>(ambient.g), static_cast<GLfloat>(ambient.b), static_cast<GLfloat>(ambient.a));
		CheckGLError("Ambient load error");
	}

	if (IsValidUniform("colFade"))
	{
		const COLOR& fade = model.GetFade();
		SetUniform4f("colFade", static_cast<GLfloat>(fade.r), static_cast<GLfloat>(fade.g), static_cast<GLfloat>(fade.b), static_cast<GLfloat>(fade.a));
		CheckGLError("Fade load error");
	}

	if (IsValidUniform("colFog") && IsValidUniform("nearFog") && IsValidUniform("farFog"))
	{
		const COLOR& fog = model.GetFog();
		SetUniform4f("colFog", fog.r, fog.g, fog.b, fog.a);
		CheckGLError("Fog load error");
		SetUniform1f("nearFog", model.GetFogNear());
		CheckGLError("Near fog load error");
		SetUniform1f("farFog", model.GetFogFar());
		CheckGLError("Far fog load error");
	}
}

void OglMainProgram::LoadLight(int index, GLint type, const CPt& dirPoint, const COLOR& color)
{
	if (index < 0 || index >= MAX_NUMBER_OF_LIGHTS)
		throw mig_exception("Invalid light index");

	std::string prefix = "lights[" + std::to_string(index) + "]";
	std::string typeStr = prefix + ".type";
	std::string dirStr = prefix + ".dirPoint";
	std::string colStr = prefix + ".colLight";

	if (IsValidUniform(typeStr))
	{
		SetUniform1i(typeStr, type);
		CheckGLError("Light type load error");
	}

	if (IsValidUniform(dirStr))
	{
		SetUniform3f(dirStr, dirPoint.x, dirPoint.y, dirPoint.z);
		CheckGLError("Light dir/origin load error");
	}

	if (IsValidUniform(colStr))
	{
		SetUniform4f(colStr, color.r, color.g, color.b, color.a);
		CheckGLError("Light color load error");
	}
}

void OglMainProgram::LoadMatrices(const CMatrix& modelMatrix)
{
	CMatrix mvp = modelMatrix;
	mvp.MatrixMultiply(OglRender::GetInstance()->GetViewMatrix());
	mvp.MatrixMultiply(OglRender::GetInstance()->GetPerspectiveMatrix());

	if (IsValidUniform("matMVP"))
	{
		SetUniformMatrix("matMVP", false, mvp);
		CheckGLError("Matrix load error");
	}

	if (IsValidUniform("matModel"))
	{
		SetUniformMatrix("matModel", false, modelMatrix);
		CheckGLError("Model matrix load error");
	}
}

void OglMainProgram::LoadObject(const CObj& obj)
{
	if (IsValidUniform("colDiffuse"))
	{
		const COLOR& color = obj.GetDiffuse();
		SetUniform4f("colDiffuse", color.r, color.g, color.b, color.a);
		CheckGLError("Diffuse load error");
	}

	if (IsValidUniform("colSpecular"))
	{
		const COLOR& specular = obj.GetSpecular();
		SetUniform4f("colSpecular", specular.r, specular.g, specular.b, specular.a);
		CheckGLError("Specular load error");
	}

	if (IsValidUniform("colGlow"))
	{
		const COLOR& glow = obj.GetGlow();
		SetUniform4f("colGlow", glow.r, glow.g, glow.b, glow.a);
		CheckGLError("Glow load error");
	}

	if (IsValidUniform("colRefl"))
	{
		const COLOR& refl = obj.GetReflection();
		SetUniform4f("colRefl", refl.r, refl.g, refl.b, refl.a);
		CheckGLError("Reflection load error");
	}

	if (IsValidUniform("colRefr"))
	{
		const COLOR& refr = obj.GetRefraction();
		SetUniform4f("colRefr", refr.r, refr.g, refr.b, refr.a);
		CheckGLError("Refraction load error");
	}
}

void OglMainProgram::LoadTextureSamplers(const std::vector<GLuint> textureIds)
{
	for (size_t i = 0; i < textureIds.size(); i++)
	{
		std::string prefix = std::string("textures[" + std::to_string(i) + "]");
		if (IsValidUniform(prefix))
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, textureIds[i]);

			SetUniform1i(prefix, i);
			CheckGLError("Sampler load error");
		}
	}
}

void OglMainProgram::LoadTextureBlocks(const std::vector<OglTextureBlock> textureBlocks)
{
	std::string posName;

	for (size_t i = 0; i < textureBlocks.size(); i++)
	{
		posName = "texBlocks[" + std::to_string(i) + "].type";
		if (IsValidUniform(posName))
			SetUniform1i(posName, textureBlocks[i].type);
		posName = "texBlocks[" + std::to_string(i) + "].coordIndex";
		if (IsValidUniform(posName))
			SetUniform1i(posName, textureBlocks[i].coordIndex);
		posName = "texBlocks[" + std::to_string(i) + "].samplerIndex";
		if (IsValidUniform(posName))
			SetUniform1i(posName, textureBlocks[i].samplerIndex);
		posName = "texBlocks[" + std::to_string(i) + "].op";
		if (IsValidUniform(posName))
			SetUniform1i(posName, textureBlocks[i].op);
		posName = "texBlocks[" + std::to_string(i) + "].flags";
		if (IsValidUniform(posName))
			SetUniform1i(posName, textureBlocks[i].flags);
	}
	for (size_t i = textureBlocks.size(); i < MAX_TEXTURES; i++)
	{
		posName = "texBlocks[" + std::to_string(i) + "].type";
		if (IsValidUniform(posName))
			SetUniform1i(posName, TEX_TYPE_NONE);
	}
}

void OglMainProgram::LoadNormal(const CPt& norm)
{
	if (IsValidUniform("normal"))
	{
		SetUniform3f("normal", norm.x, norm.y, norm.z);
		CheckGLError("Normal load error");
	}
}
