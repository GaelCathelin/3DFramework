out vec3 oColor;

void main() {
    vec2 vertices[3] = {{-0.5, 0.5}, {0.0, -0.5}, {0.5, 0.5}};
    gl_Position = vec4(vertices[gl_VertexIndex], 0.0, 1.0);
    oColor = mat3(1.0)[gl_VertexIndex];
}
