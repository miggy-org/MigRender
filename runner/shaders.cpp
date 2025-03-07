#include <string>

// embedded background vertex shader
std::string bgVertexShader = R"(
#version 330 core

in vec4 vPosition;
in vec2 vTex;

uniform mat4 matMVP;

out vec3 varFragPos;
out vec2 varTexCoord;

void main() {
	gl_Position = matMVP * vPosition;
	varFragPos = vec3(vPosition);
	varTexCoord = vTex;
}
)";

// embedded background fragment shader
std::string bgFragmentShader = R"(
#version 330 core
precision mediump float;

uniform vec4 colNorth;
uniform vec4 colEquator;
uniform vec4 colSouth;

uniform vec4 colFade;
uniform vec4 colFog;
uniform int useFog;

uniform vec3 viewPos;

uniform sampler2D bgTexture;
uniform int useTexture;

in vec3 varFragPos;
in vec2 varTexCoord;

out vec4 FragColor;

void main() {
	if (useFog > 0)
		FragColor = colFog;
	else if (useTexture > 0 && varTexCoord.x >= 0 && varTexCoord.x <= 1 && varTexCoord.y >= 0 && varTexCoord.y <= 1)
		FragColor = texture2D(bgTexture, varTexCoord);
	else
	{
		vec3 viewDir = normalize(varFragPos - viewPos);
		if (viewDir.y > 0)
			FragColor = colNorth * viewDir.y + colEquator * (1 - viewDir.y);
		else
			FragColor = colSouth * (-viewDir.y) + colEquator * (1 + viewDir.y);
	}

	if (colFade.a > 0)
	{
		FragColor *= (1 - colFade.a);
		FragColor += colFade * colFade.a;
	}
}
)";

// embedded main vertex shader
std::string mainVertexShader = R"(
#version 330 core

#define MAX_TEXTURES 8

in vec4 vPosition;
in vec3 vNormal;
in vec2 vTex[MAX_TEXTURES];

uniform mat4 matMVP;
uniform mat4 matModel;

out vec3 varFragPos;
out vec3 varNorm;
out vec2 varTexCoord[MAX_TEXTURES];

void main() {
	gl_Position = matMVP * vPosition;
	varFragPos = vec3(matModel * vPosition);
	vec4 norm = normalize(matModel * vec4(vNormal.x, vNormal.y, vNormal.z, 0));
	varNorm = vec3(norm.x, norm.y, norm.z);

	for (int i = 0; i < MAX_TEXTURES; i++)
		varTexCoord[i] = vTex[i];
}
)";

// embedded main fragment shader
std::string mainFragmentShader = R"(
#version 330 core
precision mediump float;

uniform vec4 colDiffuse;
uniform vec4 colSpecular;
uniform vec4 colGlow;
uniform vec4 colRefl;
uniform vec4 colRefr;
uniform vec3 viewPos;

uniform vec4 colFade;
uniform vec4 colFog;
uniform float nearFog;
uniform float farFog;

#define PI  3.14159265

#define MAX_NUMBER_OF_LIGHTS  8
#define MAX_TEXTURES 8

#define LIGHT_TYPE_OFF   0
#define LIGHT_TYPE_DIR   1
#define LIGHT_TYPE_POINT 2

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

struct LightBlock
{
	int type;
	vec3 dirPoint; // direction of dir light, location of point light
	vec4 colLight;
};
uniform LightBlock lights[MAX_NUMBER_OF_LIGHTS];
uniform vec4 colAmbient;

uniform sampler2D textures[MAX_TEXTURES];
struct TextureBlock
{
	int type;         // see TEX_TYPE_*
	int coordIndex;   // -1 means not used
	int samplerIndex; // -1 means not used
	int op;           // see TEX_OP_*
	int flags;        // see TEX_FLAG_*
};
uniform TextureBlock texBlocks[MAX_TEXTURES];

in vec3 varFragPos;
in vec3 varNorm;
in vec2 varTexCoord[MAX_TEXTURES];

out vec4 FragColor;

vec4 applyTextureOp(int operation, vec4 lhs, vec4 colTex, float alpha)
{
	if (operation == TEX_OP_MULTIPLY)
		lhs *= colTex;
	else if (operation == TEX_OP_ADD)
		lhs += colTex;
	else if (operation == TEX_OP_SUBTRACT)
		lhs -= colTex;
	else if (operation == TEX_OP_BLEND)
	{
		if (alpha < 1)
		{
			lhs *= (1 - alpha);
			lhs += (colTex * alpha);
		}
	}
	return lhs;
}

vec4 processTexture(TextureBlock block, vec4 rhs)
{
	vec4 lhs = rhs;

	if (block.coordIndex > -1 && block.samplerIndex > -1)
	{
		if ((block.flags & TEX_FLAG_INVERT_COLOR) != 0)
		{
			rhs.r = 1 - rhs.r;
			rhs.g = 1 - rhs.g;
			rhs.b = 1 - rhs.b;
		}
		if ((block.flags & TEX_FLAG_INVERT_ALPHA) != 0)
			rhs.a = 1 - rhs.a;

		vec4 colTex = texture2D(textures[block.samplerIndex], varTexCoord[block.coordIndex]);
		lhs = applyTextureOp(block.op, lhs, colTex, rhs.a);
	}
	return lhs;
}

vec4 processReflection(TextureBlock block, vec4 rhs)
{
	vec4 lhs = rhs;

	if (block.samplerIndex > -1)
	{
		vec2 uvc;

		vec3 viewDir = normalize(viewPos - varFragPos);
		float f2ndoti = 2*dot(varNorm, viewDir);

		vec3 tmp = normalize(viewDir - f2ndoti*varNorm);
		tmp = normalize(vec3(-tmp.x, 0, tmp.z));
		if (length(tmp) > 0)
		{
			uvc.x = atan(tmp.z, tmp.x);
			if (uvc.x >= 0)
				uvc.x = uvc.x / (2*PI);
			else
				uvc.x = (PI + uvc.x) / (2*PI) + 0.5;
		}
		else
			uvc.x = 0.5;
		uvc.y = (1 + varNorm.y) / 2;

		vec4 colTex = texture2D(textures[block.samplerIndex], uvc);
		lhs = applyTextureOp(block.op, lhs, colTex, rhs.a);
	}
	return lhs * colSpecular;
}

vec4 processTextures(int type, vec4 rhs, bool useEmptyIfNone)
{
	vec4 lhs = rhs;

	bool applied = false;
	for (int i = 0; i < MAX_TEXTURES; i++)
	{
		if (texBlocks[i].type == type)
		{
			if (type != TEX_TYPE_REFLECTION)
				lhs = processTexture(texBlocks[i], lhs);
			else
				lhs = processReflection(texBlocks[i], lhs);
			applied = true;
		}
	}

	return (applied || !useEmptyIfNone ? lhs : vec4(0, 0, 0, 0));
}

vec2 processBumpOffset(TextureBlock block)
{
	vec2 offset = vec2(0, 0);
	if (block.coordIndex > -1 && block.samplerIndex > -1)
	{
		vec2 tc = varTexCoord[block.coordIndex];
		vec2 tcLeft = vec2(tc.x - 0.01, tc.y);
		vec2 tcRight = vec2(tc.x + 0.01, tc.y);
		vec2 tcUp = vec2(tc.x, tc.y - 0.01);
		vec2 tcDown = vec2(tc.x, tc.y + 0.01);

		vec4 colLeft = texture2D(textures[block.samplerIndex], tcLeft);
		vec4 colRight = texture2D(textures[block.samplerIndex], tcRight);
		vec4 colUp = texture2D(textures[block.samplerIndex], tcUp);
		vec4 colDown = texture2D(textures[block.samplerIndex], tcDown);

		offset.x = (colRight.x - colLeft.x);
		offset.y = (colUp.y - colDown.y);
	}
	return offset;
}

vec3 processBumpMap()
{
	vec3 realNorm = varNorm;

	for (int i = 0; i < MAX_TEXTURES; i++)
	{
		if (texBlocks[i].type == TEX_TYPE_BUMP)
		{
			vec2 offset = processBumpOffset(texBlocks[i]);
			realNorm = normalize(vec3(realNorm.x + offset.x, realNorm.y + offset.y, realNorm.z));
			break;
		}
	}

	return realNorm;
}

void main() {
	vec4 texRefr = processTextures(TEX_TYPE_TRANSPARENCY, colRefr, false);
	if (texRefr.a == 0)
	{
		discard;
	}

	vec4 texDiff = processTextures(TEX_TYPE_DIFFUSE, colDiffuse, false);
	vec4 texSpec = processTextures(TEX_TYPE_SPECULAR, colSpecular, false);
	vec4 texGlow = processTextures(TEX_TYPE_GLOW, colGlow, false);
	vec4 texRefl = processTextures(TEX_TYPE_REFLECTION, colRefl, true);

	vec4 ambient = colAmbient * texDiff;
	vec4 diffuse;
	vec4 specular;

	vec3 normal = processBumpMap();

	vec3 viewDir = normalize(viewPos - varFragPos);
	for (int i = 0; i < MAX_NUMBER_OF_LIGHTS; i++)
	{
		if (lights[i].type == LIGHT_TYPE_OFF)
			break;

		vec3 dir;
		if (lights[i].type == LIGHT_TYPE_POINT)
			dir = normalize(varFragPos - lights[i].dirPoint);
		else
			dir = lights[i].dirPoint;

		diffuse += texDiff * lights[i].colLight * max(dot(-dir, normal), 0);

		vec3 reflectDir = reflect(dir, normal);
		specular += texSpec * pow(max(dot(viewDir, reflectDir), 0.0), 100);
	}

	FragColor = ambient + diffuse + specular + texGlow + texRefl;
	FragColor.a = texRefr.a;
	if (nearFog > 0 && farFog > 0)
	{
		vec3 viewVector = varFragPos - viewPos;
		float length = length(viewVector);
		if (length > nearFog)
		{
			float scale = min((length - nearFog) / (farFog - nearFog), 1);
			FragColor *= (1 - scale);
			FragColor += vec4(colFog.r * scale, colFog.g * scale, colFog.b * scale, 0);
			FragColor.a = 1;
		}
	}
	if (colFade.a > 0)
	{
		float fragAlpha = FragColor.a;
		FragColor *= (1 - colFade.a);
		FragColor += vec4(colFade.r * colFade.a, colFade.g * colFade.a, colFade.b * colFade.a, 0);
		FragColor.a = fragAlpha;
	}
}
)";
