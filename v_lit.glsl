#version 330

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 lightDir0;
uniform vec3 lightDir1;
uniform mat4 lightSpaceMatrix;
uniform vec2 uvScale;
uniform vec2 uvOffset;

in vec4 vertex;
in vec4 normal;
in vec2 texCoord0;
in vec4 tangent;

out vec3 vN;
out vec3 vT;
out vec3 vL0;
out vec3 vL1;
out vec3 vView;
out vec4 vShadowCoord;
out vec2 iTex;

void main(void)
{
	vec4 eye = V * M * vertex;
	vN = mat3(V * M) * normal.xyz;
	vT = mat3(V * M) * tangent.xyz;
	vL0 = mat3(V) * lightDir0;
	vL1 = mat3(V) * lightDir1;
	vView = -eye.xyz;
	vShadowCoord = lightSpaceMatrix * M * vertex;
	iTex = texCoord0 * uvScale + uvOffset;
	gl_Position = P * eye;
}
