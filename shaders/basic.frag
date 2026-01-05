#version 410 core

in vec4 fPosEye;
in vec3 fNormalEye;
in vec2 fTexCoords;

out vec4 fColor;

// Lighting Structures
struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 color;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
    vec3 color;
    int active;
};

// Uniforms
uniform mat4 view;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform PointLight pointLights[6];
uniform int nrPointLights;
uniform SpotLight spotLight;

// Textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform int hasTexture;

// Material
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;

// Components
vec3 ambientTotal;
vec3 diffuseTotal;
vec3 specularTotal;

float ambientStrength = 0.2f;
float specularStrength = 0.5f;
float shininess = 32.0f;

void calcDirLight(vec3 normal, vec3 viewDir) {
    vec3 lightDirN = normalize(lightDir);
    float diff = max(dot(normal, lightDirN), 0.0);
    vec3 reflectDir = reflect(-lightDirN, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    ambientTotal += lightColor * ambientStrength;
    diffuseTotal += lightColor * diff;
    specularTotal += lightColor * spec * specularStrength;
}

void calcPointLight(PointLight light, vec3 normal, vec3 viewDir, vec3 fragPos) {
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    ambientTotal += light.color * ambientStrength * attenuation;
    diffuseTotal += light.color * diff * attenuation;
    specularTotal += light.color * spec * specularStrength * attenuation;
}

void calcSpotLight(SpotLight light, vec3 normal, vec3 viewDir, vec3 fragPos) {
    if (light.active == 0) return;
    vec3 lightDir = normalize(light.position - fragPos);
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    if (theta > light.outerCutOff) {
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        float distance = length(light.position - fragPos);
        float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
        ambientTotal += light.color * ambientStrength * attenuation * intensity;
        diffuseTotal += light.color * diff * attenuation * intensity;
        specularTotal += light.color * spec * specularStrength * attenuation * intensity;
    }
}

void main() 
{
    ambientTotal = vec3(0.0);
    diffuseTotal = vec3(0.0);
    specularTotal = vec3(0.0);
    vec3 normal = normalize(fNormalEye);
    vec3 viewDir = normalize(-fPosEye.xyz);
    
    // 1. Directional
    calcDirLight(normal, viewDir);
    
    // 2. Point Lights (SAFE LOOP)
    for(int i = 0; i < 6; i++) {
        if (i >= nrPointLights) break;
        calcPointLight(pointLights[i], normal, viewDir, fPosEye.xyz);
    }
    
    // 3. Spot Light
    calcSpotLight(spotLight, normal, viewDir, fPosEye.xyz);

    vec3 ambientFinal = ambientTotal * Ka;
    vec3 diffuseFinal = diffuseTotal * Kd;
    vec3 specularFinal = specularTotal * Ks;

    if (hasTexture == 1) {
        vec3 texColor = texture(diffuseTexture, fTexCoords).rgb;
        ambientFinal *= texColor;
        diffuseFinal *= texColor;
        specularFinal *= texture(specularTexture, fTexCoords).rgb;
    }

    vec3 color = min(ambientFinal + diffuseFinal + specularFinal, 1.0f);
    fColor = vec4(color, 1.0f);
}
