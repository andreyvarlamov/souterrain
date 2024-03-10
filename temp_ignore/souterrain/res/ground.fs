#version 330 core

in vec4 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D sprite;
uniform sampler2D vig;
uniform vec4 colorDiffuse;

void main()
{
    vec4 spriteTexel = texture(sprite, fragTexCoord.xy);
    vec4 vigTexel = texture(vig, fragTexCoord.zw);
    finalColor = fragColor * vec4(spriteTexel.rgb, spriteTexel.a * vigTexel.a);
}
