uniform sampler2D Texture;

in vec2 iTexCoord;
in vec3 iPos;

out vec3 FragColor;

void main() {
    vec3 N = normalize(cross(dFdy(iPos), dFdx(iPos)));
    vec3 L = normalize(vec3(1.0, -1.0, 1.0));
    FragColor = max(0.0, 0.6 * dot(N, L) + 0.4) * texture(Texture, iTexCoord).rgb;
}
