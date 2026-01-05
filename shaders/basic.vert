#version 410 core

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;



uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;
uniform mat4 lightSpaceMatrix;

out vec4 fPosEye;
out vec3 fNormalEye;
out vec2 fTexCoords;
out vec4 fPosLightSpace;

void main() 
{
	fPosEye = view * model * vec4(vPosition, 1.0f);
	fNormalEye = normalize(normalMatrix * vNormal);
    fTexCoords = vTexCoords;
    fPosLightSpace = lightSpaceMatrix * model * vec4(vPosition, 1.0f);
    
	gl_Position = projection * view * model * vec4(vPosition, 1.0f);
}
