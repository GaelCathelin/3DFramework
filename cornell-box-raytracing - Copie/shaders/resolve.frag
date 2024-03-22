uniform sampler2DMS Texture;

out vec3 FragColor;

void main() {
    FragColor = vec3(0.0);
    for (int i = 0; i < textureSamples(Texture); i++) {
		vec4 S = texelFetch(Texture, ivec2(gl_FragCoord), i);
		if (S.w > 0.0)
			FragColor += min(vec3(1.0), S.rgb / S.w);
	}

    FragColor /= textureSamples(Texture);
}
