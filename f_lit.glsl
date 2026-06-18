#version 330

uniform vec4 color; // kolor materialu (bez tekstur - na tym etapie)

in vec3 vN;
in vec3 vL0;
in vec3 vL1;
in vec3 vView;

out vec4 pixelColor;

void main(void)
{
	vec3 n = normalize(vN);
	vec3 v = normalize(vView);
	vec3 l0 = normalize(vL0);
	vec3 l1 = normalize(vL1);

	vec3 r0 = reflect(-l0, n);
	vec3 r1 = reflect(-l1, n);

	float amb = 0.18;
	float d0 = max(dot(n, l0), 0.0);
	float d1 = max(dot(n, l1), 0.0);
	float s0 = pow(max(dot(r0, v), 0.0), 24.0);
	float s1 = pow(max(dot(r1, v), 0.0), 24.0);

	vec3 c = color.rgb;
	vec3 lit = c * amb
			 + c * (0.70 * d0 + 0.55 * d1)
			 + vec3(0.25) * (0.5 * s0 + 0.4 * s1);

	pixelColor = vec4(clamp(lit, 0.0, 1.0), color.a);
}
