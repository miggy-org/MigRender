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
