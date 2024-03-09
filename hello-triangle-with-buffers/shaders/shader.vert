in vec4 iPosition;
in vec3 iColor;

out vec3 oColor;

void main() {
    gl_Position = iPosition;
    oColor = iColor;
}
