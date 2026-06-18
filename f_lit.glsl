#version 330

uniform vec4 color;         // kolor/odcien materialu (mnozony przez teksture)
uniform vec3 lc0;           // kolor swiatla 0 (glowne, chlodne) - rzuca cien
uniform vec3 lc1;           // kolor swiatla 1 (slabe wypelnienie)
uniform vec3 ambientColor;  // swiatlo otoczenia (nocny, niebieskawy)
uniform float specStrength; // sila odblaskow materialu (0 = matowy)
uniform int useTex;         // 1 = uzyj tekstur (color/normal/roughness), 0 = jednolity kolor
uniform sampler2D shadowMap;
uniform sampler2D diffuseMap; // kolor (albedo)
uniform sampler2D normalMap;  // mapa normalnych (faktura)
uniform sampler2D roughMap;   // szorstkosc (steruje polyskiem)

in vec3 vN;
in vec3 vT;
in vec3 vL0;
in vec3 vL1;
in vec3 vView;
in vec4 vShadowCoord;
in vec2 iTex;

out vec4 pixelColor;

// 1.0 = w pelni oswietlone, 0.0 = w pelnym cieniu (dla swiatla glownego). PCF 3x3 = miekko.
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
		// Macierz TBN (styczna -> oko) i normalna z mapy normalnych
		vec3 Ng = normalize(vN);
		vec3 T = normalize(vT - dot(vT, Ng) * Ng); // ortogonalizacja
		vec3 B = cross(Ng, T);
		vec3 nm = texture(normalMap, iTex).xyz * 2.0 - 1.0;
		n = normalize(mat3(T, B, Ng) * nm);
		float rough = texture(roughMap, iTex).r;
		specS = specStrength * (1.0 - rough); // gladsze miejsca = mocniejszy odblask
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

	float sh = shadowFactor(n, l0); // cien tylko od swiatla glownego

	vec3 lit = albedo * ambientColor
			 + albedo * (lc0 * d0 * sh + lc1 * d1)
			 + (lc0 * s0 * sh + lc1 * s1) * specS;

	pixelColor = vec4(clamp(lit, 0.0, 1.0), color.a);
}
