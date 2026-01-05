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
uniform sampler2D shadowMap;
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

in vec4 fPosLightSpace;

float calcShadow(vec4 fragPosLightSpace) {
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        return 0.0;
        
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // check whether current frag pos is in shadow
    float bias = 0.005;
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    return shadow;
}

void calcDirLight(vec3 normal, vec3 viewDir, float shadow) {
    vec3 lightDirN = normalize(lightDir);
    float diff = max(dot(normal, lightDirN), 0.0);
    vec3 reflectDir = reflect(-lightDirN, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    ambientTotal += lightColor * ambientStrength;
    // Shadow affects Diffuse and Specular, but NOT Ambient
    diffuseTotal += lightColor * diff * (1.0 - shadow);
    specularTotal += lightColor * spec * specularStrength * (1.0 - shadow);
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
    float shadow = calcShadow(fPosLightSpace);
    calcDirLight(normal, viewDir, shadow);
    
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
