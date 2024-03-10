#version 330 core

layout (location = 0) in vec3 vertPosition;
layout (location = 1) in vec4 vertTexCoord;
layout (location = 2) in vec4 vertColor;

out vec4 fragTexCoord;
out vec4 fragColor;

uniform mat4 mvp;

void main()
{
    fragTexCoord = vertTexCoord;
    fragColor = vertColor;
    gl_Position = mvp * vec4(vertPosition, 1.0);
}
