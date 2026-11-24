#ifndef GUI_GLSL_EMBED_H_
#define GUI_GLSL_EMBED_H_

inline constexpr const char* kVertexShaderSrc = R"(#version 300 es
precision highp float;
precision highp int;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uMVP;
uniform mat4 uModelMatrix;
uniform mat4 uNormalMatrix;

uniform bool isWireframe;
uniform bool uFlatShading;
uniform float uPointSize;

out vec3 FragPos;
out vec3 Normal;
out vec2 vTexCoords;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    FragPos = vec3(uModelMatrix * vec4(aPos, 1.0));
    Normal = normalize(mat3(uNormalMatrix) * aNormal);
    vTexCoords = aTexCoord;
    gl_PointSize = uPointSize;
}
)";

inline constexpr const char* kFragmentShaderSrc = R"(#version 300 es
precision highp float;
precision highp int;

in vec3 FragPos;
in vec3 Normal;
in vec2 vTexCoords;

uniform bool isWireframe;
uniform vec3 uColor;

uniform bool uDashedEdges;
uniform bool uCircleVertices;
uniform bool uFlatShading;
uniform vec3 uLightColor;
uniform vec3 uLightPos;
uniform vec3 uViewPos;

uniform sampler2D uTex;
uniform bool uHasTex;

out vec4 FragColor;

float dashMask(float x, float l, float r);
void makeCircleVertices();

void main() {
    if (isWireframe) {
        float alpha = 1.0;
        if (uDashedEdges) alpha = dashMask(gl_FragCoord.x, 12.0, 0.3);
        FragColor = vec4(uColor, alpha);
    } else {
        const float kAmbientStrength = 0.1;
        vec3 ambientColor = uLightColor * kAmbientStrength;
        vec3 lightDir = normalize(uLightPos - FragPos);
        vec3 normalDir = uFlatShading
            ? normalize(cross(dFdx(FragPos), dFdy(FragPos)))
            : normalize(Normal);
        if (!gl_FrontFacing) normalDir = -normalDir;

        float diff = dot(normalDir, lightDir);
        vec3 diffuseColor = max(0.0, diff) * uLightColor;

        vec3 baseColor = uHasTex ? texture(uTex, vTexCoords).rgb : uColor;

        if (uFlatShading) {
            FragColor = vec4((ambientColor + diffuseColor) * baseColor, 1.0);
        } else {
            const float kShininess = 32.0;
            const float kSpecularStrength = 0.5;
            vec3 viewDir = normalize(uViewPos - FragPos);

            vec3 reflectDir = reflect(-lightDir, normalDir);
            float spec = pow(max(0.0, dot(viewDir,reflectDir)),kShininess);
            vec3 specularColor = kSpecularStrength * spec * uLightColor;
            FragColor = vec4((ambientColor + diffuseColor + specularColor) * baseColor, 1.0);
        }
    }

    if (uCircleVertices) makeCircleVertices();
}

float dashMask(float x, float l, float r) {
    float w = 1.0;
    x = x - l * floor(x / l);

    float a = l * r * 0.5 - w * 0.5;
    float b = l * r * 0.5 + w * 0.5;
    float c = l - b;
    float d = l - a;

    float t;
    if      (x < a) t = 1.0;
    else if (x < b) t = 1.0 - (x - a) / w;
    else if (x < c) t = 0.0;
    else if (x < d) t = (x - c) / w;
    else            t = 1.0;

    return 1.0 - smoothstep(0.0, 1.0, t);
}

void makeCircleVertices() {
    vec2 c = gl_PointCoord - vec2(0.5);
    if (dot(c, c) > 0.25) discard;
}
)";

#endif
