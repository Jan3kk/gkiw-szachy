#version 330

//Tekstura bez cieniowania (uzywane dla nieba - iteracja 1)
uniform sampler2D textureMap0;

in vec2 iTexCoord0;

out vec4 pixelColor; //Zmienna wyjsciowa fragment shadera

void main(void) {
    pixelColor = texture(textureMap0, iTexCoord0);
}
