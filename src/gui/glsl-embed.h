#ifndef GUI_GLSL_EMBED_H_
#define GUI_GLSL_EMBED_H_

inline constexpr const char* kVertexShaderSrc = R"(#version 330
layout(location = 0) in vec3 aPos;
uniform mat4 uMVP;
uniform float uPointSize; ////////////////////
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    gl_PointSize = uPointSize; ////////////////////////
}
)";

inline constexpr const char* kFragmentShaderSrc = R"(#version 330 
out vec4 FragColor;
uniform vec4 uColor;

uniform float uPatternLength;   // длина цикла в пикселях, например 12.0//////////////////
uniform int uDrawEdgesDashed;   // 1 — пунктир, 0 — сплошные//////////////////////////

uniform bool uRound; /////////////////////////

void main() {
if (uDrawEdgesDashed == 1) {////////////////////////
        // Используем комбинацию x и y экранных координат ////////////////////////
        float coord = gl_FragCoord.x + gl_FragCoord.y; ///////////////////////
        float dash = step(0.5, fract(coord / uPatternLength)); //////////////
        if (dash < 0.5) discard; ////////////////////////
    } //////////////////////

if (uRound) { ///////////////////////////
    // gl_PointCoord идёт от (0,0) до (1,1) внутри точки
    vec2 c = gl_PointCoord - vec2(0.5);
    if (dot(c, c) > 0.25)   // радиус 0.5
    discard;
    } //////////////////////

    FragColor = uColor;
}
)";
#endif
