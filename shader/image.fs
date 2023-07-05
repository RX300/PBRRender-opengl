#version 330 core
out vec3 FragColor;
in vec2 TexCoords;
uniform  sampler2D texture1;
void main() 
{
    vec3 color = texture(texture1, TexCoords).rgb;
        // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = color;
}