#version 330 core

in vec4 fragTexCoord;
in vec4 fragColor;
in vec3 fragPos;

out vec4 finalColor;

uniform sampler2D diffuse;
uniform sampler2D normal;

uniform vec3 lightPos;

void main()
{
    vec4 diffuseTexel = texture(diffuse, fragTexCoord.xy);
    vec4 normalTexel = texture(normal, fragTexCoord.xy);
    normalTexel = normalize(normalTexel * 2.0 - 1.0);
    vec3 norm = vec3(normalTexel.r, -normalTexel.g, normalTexel.b);

    float ambientStrength = 0.1;
    vec3 lightDir = normalize(lightPos - fragPos);
    float diffuseStrength = max(dot(norm, lightDir), 0.0);

    float lightStrength = ambientStrength + diffuseStrength;
    
    finalColor = vec4(fragColor.rgb * diffuseTexel.rgb * lightStrength, fragColor.a * diffuseTexel.a);
}
