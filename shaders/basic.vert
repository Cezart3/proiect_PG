#version 410 core

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

out vec4 fPosEye;
out vec3 fNormalEye;
out vec2 fTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main() 
{
	fPosEye = view * model * vec4(vPosition, 1.0f);
	fNormalEye = normalize(normalMatrix * vNormal);
    fTexCoords = vTexCoords;
    
	gl_Position = projection * view * model * vec4(vPosition, 1.0f);
}
