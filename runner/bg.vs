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
