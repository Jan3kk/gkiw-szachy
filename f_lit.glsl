#version 330

uniform vec4 color;
uniform vec3 lc0;
uniform vec3 lc1;
uniform vec3 ambientColor;
uniform float specStrength;
uniform int useTex;
uniform sampler2D shadowMap;
uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D roughMap;

in vec3 vN;
in vec3 vT;
in vec3 vL0;
in vec3 vL1;
in vec3 vView;
in vec4 vShadowCoord;
in vec2 iTex;

out vec4 pixelColor;

float shadowFactor(vec3 n, vec3 l0)
{
	vec3 p = vShadowCoord.xyz / vShadowCoord.w;
	p = p * 0.5 + 0.5;
	if (p.z > 1.0)
		return 1.0;
	float bias = max(0.0030 * (1.0 - dot(n, l0)), 0.0010);
	vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));
	float sh = 0.0;
	for (int x = -1; x <= 1; x++)
		for (int y = -1; y <= 1; y++)
		{
			float d = texture(shadowMap, p.xy + vec2(x, y) * texel).r;
			sh += (p.z - bias > d) ? 1.0 : 0.0;
		}
	return 1.0 - sh / 9.0;
}

void main(void)
{
	vec3 v = normalize(vView);
	vec3 l0 = normalize(vL0);
	vec3 l1 = normalize(vL1);

	vec3 n;
	vec3 albedo;
	float specS = specStrength;

	if (useTex == 1)
	{
		albedo = texture(diffuseMap, iTex).rgb * color.rgb;
		vec3 Ng = normalize(vN);
		vec3 T = normalize(vT - dot(vT, Ng) * Ng);
		vec3 B = cross(Ng, T);
		vec3 nm = texture(normalMap, iTex).xyz * 2.0 - 1.0;
		n = normalize(mat3(T, B, Ng) * nm);
		float rough = texture(roughMap, iTex).r;
		specS = specStrength * (1.0 - rough);
	}
	else
	{
		albedo = color.rgb;
		n = normalize(vN);
	}

	vec3 r0 = reflect(-l0, n);
	vec3 r1 = reflect(-l1, n);
	float d0 = max(dot(n, l0), 0.0);
	float d1 = max(dot(n, l1), 0.0);
	float s0 = pow(max(dot(r0, v), 0.0), 24.0);
	float s1 = pow(max(dot(r1, v), 0.0), 24.0);

	float sh = shadowFactor(n, l0);

	vec3 lit = albedo * ambientColor
			 + albedo * (lc0 * d0 * sh + lc1 * d1)
			 + (lc0 * s0 * sh + lc1 * s1) * specS;

	pixelColor = vec4(clamp(lit, 0.0, 1.0), color.a);
}
