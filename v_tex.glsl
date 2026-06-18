#version 330

//Zmienne jednorodne
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

//Atrybuty
in vec4 vertex; //wspolrzedne wierzcholka w przestrzeni modelu
in vec2 texCoord0; //wspolrzedne teksturowania

//Zmienne interpolowane
out vec2 iTexCoord0;

void main(void) {
    iTexCoord0 = texCoord0;
    gl_Position = P * V * M * vertex;
}
