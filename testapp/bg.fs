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
