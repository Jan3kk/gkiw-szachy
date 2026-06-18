#version 330

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec3 lightDir0;		   // kierunek DO swiatla 0 (gwiazda) - swiatlo kierunkowe
uniform vec3 lightDir1;		   // kierunek DO swiatla 1 (wypelnienie)
uniform mat4 lightSpaceMatrix; // P*V swiatla (do mapy cieni)
uniform vec2 uvScale;		   // krotnosc powtarzania tekstury
uniform vec2 uvOffset;		   // przesuniecie tekstury (np. inny fragment slojow na polu)

in vec4 vertex;
in vec4 normal;
in vec2 texCoord0;
in vec4 tangent;

out vec3 vN;		   // normalna w przestrzeni oka
out vec3 vT;		   // wektor styczny w przestrzeni oka
out vec3 vL0;		   // kierunek do swiatla 0
out vec3 vL1;		   // kierunek do swiatla 1
out vec3 vView;		   // kierunek do obserwatora
out vec4 vShadowCoord; // pozycja wierzcholka w przestrzeni swiatla
out vec2 iTex;		   // wspolrzedne tekstury

void main(void)
{
	vec4 eye = V * M * vertex;
	vN = mat3(V * M) * normal.xyz;
	vT = mat3(V * M) * tangent.xyz;
	vL0 = mat3(V) * lightDir0; // swiatlo kierunkowe: staly kierunek w calej scenie
	vL1 = mat3(V) * lightDir1;
	vView = -eye.xyz;
	vShadowCoord = lightSpaceMatrix * M * vertex;
	iTex = texCoord0 * uvScale + uvOffset;
	gl_Position = P * eye;
}
