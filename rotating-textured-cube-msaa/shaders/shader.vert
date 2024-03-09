uniform _ {
    mat4 ProjectionMatrix, ViewMatrix; // automatically provided
    mat4 ModelMatrix;
};

in vec4 iPosition;
in vec2 iTexCoord;

out vec2 oTexCoord;
out vec3 oPos;

void main() {
    vec4 P = ModelMatrix * iPosition;
    oPos = P.xyz;
    gl_Position = ProjectionMatrix * ViewMatrix * P;
    oTexCoord = iTexCoord;
    oTexCoord.y = 1.0 - oTexCoord.y;
}
