#version 330

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec4 lp0; // pozycja swiatla 0
uniform vec4 lp1; // pozycja swiatla 1

in vec4 vertex;
in vec4 normal;

out vec3 vN;    // normalna w przestrzeni oka
out vec3 vL0;   // kierunek do swiatla 0
out vec3 vL1;   // kierunek do swiatla 1
out vec3 vView; // kierunek do obserwatora

void main(void)
{
	vec4 eye = V * M * vertex;
	vN = mat3(V * M) * normal.xyz; 
	vL0 = (V * lp0 - eye).xyz;
	vL1 = (V * lp1 - eye).xyz;
	vView = -eye.xyz;
	gl_Position = P * eye;
}
