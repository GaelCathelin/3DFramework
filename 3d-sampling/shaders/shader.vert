uniform _ {
	mat4 ProjectionMatrix, ViewMatrix;
	vec3 pos;
};

void main() {
	gl_PointSize = 2.0;
    gl_Position = ProjectionMatrix * ViewMatrix * vec4(pos, 1.0);
}
