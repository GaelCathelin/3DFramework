void main() {
    gl_Position = vec4(4.0 * vec2(gl_VertexIndex & 1, gl_VertexIndex >> 1) - 1.0, 0.0, 1.0);
}
