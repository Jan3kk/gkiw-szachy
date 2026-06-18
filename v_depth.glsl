#version 330

// Przebieg glebii (shadow map): rzutujemy wierzcholki z punktu widzenia swiatla.
uniform mat4 MVP; // lightSpaceMatrix * M

in vec4 vertex;

void main(void)
{
	gl_Position = MVP * vertex;
}
