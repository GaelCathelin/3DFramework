in float pointSize;

out vec4 FragColor;

void main() {
    float alpha = smoothstep(0.5, 0.5 - 1.5 / pointSize, distance(gl_PointCoord, vec2(0.5)));
    FragColor = vec4(vec3(1.0), alpha);
}
