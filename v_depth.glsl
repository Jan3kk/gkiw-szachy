#version 330

uniform mat4 MVP;

in vec4 vertex;

void main(void)
{
	gl_Position = MVP * vertex;
}
