layout(binding = 0) uniform _ {
    mat4 ProjectionMatrix, ViewMatrix;
};

in vec4 iPosition;
in vec3 iNormal;

out vec3 oPos, oNormal;

void main() {
    oPos = iPosition.xyz;
    gl_Position = ProjectionMatrix * ViewMatrix * iPosition;
	oNormal = iNormal;
}
