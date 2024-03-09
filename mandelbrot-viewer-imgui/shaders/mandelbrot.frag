#extension GL_EXT_shader_explicit_arithmetic_types_int16: require
#extension GL_EXT_shader_explicit_arithmetic_types_int64: require
#extension GL_EXT_shader_explicit_arithmetic_types_float16: require
#define half  float16_t
#define hvec2 f16vec2

#define FP_SHIFT    63.0
#define INT32_SHIFT 28u
#define INT64_SHIFT 60u

#define PI 3.1415927

#define FP16  0
#define FP32  1
#define FP64  2
#define INT32 3
#define INT64 4

uniform _ {
    dvec2 offset;
    double zoom;
    vec2 resolution;
    int maxIters;
    float subPixel;
    int version;
};

out vec4 FragColor;

int     initFixed(float  x) {return int(x * exp2(INT32_SHIFT));}
ivec2   initFixed(vec2   v) {return ivec2(initFixed(v.x), initFixed(v.y));}
int64_t initFixed(double x) {return int64_t(x * (1ul << INT64_SHIFT));}
i64vec2 initFixed(dvec2  v) {return i64vec2(initFixed(v.x), initFixed(v.y));}

int     squareFixed(int     x) {int64_t i = x; return int(i * i >> INT32_SHIFT);}
ivec2   squareFixed(ivec2   v) {return ivec2(squareFixed(v.x), squareFixed(v.y));}
int64_t squareFixed(int64_t x) {
    uint64_t u = uint64_t(abs(x));
    uint64_t uh = u >> 32ul, ul = u & 0xFFFFFFFFul;
    return int64_t((uh * uh << 64ul - INT64_SHIFT) + (ul * uh >> INT64_SHIFT - 33ul) + (ul * ul >> INT64_SHIFT));
}
i64vec2 squareFixed(i64vec2 v) {return i64vec2(squareFixed(v.x), squareFixed(v.y));}

vec3 Colorize(vec2 Z, int nbIters) {
    vec3 Color = vec3(0.0);

    if (nbIters < maxIters) {
        Z *= exp2(-FP_SHIFT);
        float t = nbIters + 1.0 - log2(2.0 * FP_SHIFT + log2(dot(Z, Z)));
        Color = 0.5 - 0.5 * cos(2.0 * PI * (t / 64.0 + vec3(-0.1, 0.0, 0.1)));
        Color *= Color;
    }

    return Color;
}

vec4 fp16(hvec2 C) {
    hvec2 Z = C;
    int nbIters = 1;

    while (dot(Z, Z) < 12.0hf && nbIters < maxIters) {
        Z = hvec2(fma(-Z.y, Z.y, fma(Z.x, Z.x, C.x)), fma(2.0hf * Z.x, Z.y, C.y));
        Z = hvec2(fma(-Z.y, Z.y, fma(Z.x, Z.x, C.x)), fma(2.0hf * Z.x, Z.y, C.y));
        Z = hvec2(fma(-Z.y, Z.y, fma(Z.x, Z.x, C.x)), fma(2.0hf * Z.x, Z.y, C.y));
        nbIters += 3;
    }

    return vec4(Colorize(Z, nbIters), 1.0);
}

vec4 fp32(vec2 C) {
    vec2 Z = C;
    int nbIters = 1;

    while (dot(Z, Z) < 12.0 && nbIters < maxIters) {
        Z = vec2(fma(-Z.y, Z.y, fma(Z.x, Z.x, C.x)), fma(2.0 * Z.x, Z.y, C.y));
        Z = vec2(fma(-Z.y, Z.y, fma(Z.x, Z.x, C.x)), fma(2.0 * Z.x, Z.y, C.y));
        Z = vec2(fma(-Z.y, Z.y, fma(Z.x, Z.x, C.x)), fma(2.0 * Z.x, Z.y, C.y));
        Z = vec2(fma(-Z.y, Z.y, fma(Z.x, Z.x, C.x)), fma(2.0 * Z.x, Z.y, C.y));
        Z = vec2(fma(-Z.y, Z.y, fma(Z.x, Z.x, C.x)), fma(2.0 * Z.x, Z.y, C.y));
        Z = vec2(fma(-Z.y, Z.y, fma(Z.x, Z.x, C.x)), fma(2.0 * Z.x, Z.y, C.y));
        nbIters += 6;
    }

    return vec4(Colorize(Z, nbIters), 1.0);
}

vec4 fp64(dvec2 C) {
    dvec2 Z = C;
    int nbIters = 1;

    while (dot(Z, Z) < 12.0lf && nbIters < maxIters) {
        Z = dvec2(fma(-Z.y, Z.y, fma(Z.x, Z.x, C.x)), fma(2.0lf * Z.x, Z.y, C.y));
        Z = dvec2(fma(-Z.y, Z.y, fma(Z.x, Z.x, C.x)), fma(2.0lf * Z.x, Z.y, C.y));
        Z = dvec2(fma(-Z.y, Z.y, fma(Z.x, Z.x, C.x)), fma(2.0lf * Z.x, Z.y, C.y));
        Z = dvec2(fma(-Z.y, Z.y, fma(Z.x, Z.x, C.x)), fma(2.0lf * Z.x, Z.y, C.y));
        Z = dvec2(fma(-Z.y, Z.y, fma(Z.x, Z.x, C.x)), fma(2.0lf * Z.x, Z.y, C.y));
        Z = dvec2(fma(-Z.y, Z.y, fma(Z.x, Z.x, C.x)), fma(2.0lf * Z.x, Z.y, C.y));
        nbIters += 6;
    }

    return vec4(Colorize(vec2(Z), nbIters), 1.0);
}

vec4 int32(ivec2 C) {
    const ivec2 two = initFixed(vec2(2));
    ivec2 Z = C;
    int nbIters = 1;

    while (all(lessThan(abs(Z), two)) && nbIters < maxIters) {
        ivec2 Z2 = squareFixed(Z);
        Z = ivec2(Z2.x, squareFixed(Z.x + Z.y) - Z2.x) - Z2.y + C;
        nbIters++;
    }

    return vec4(Colorize(vec2(1.0 / exp2(INT32_SHIFT) * Z), nbIters), 1.0);
}

vec4 int64(i64vec2 C) {
    const i64vec2 two = initFixed(dvec2(2));
    i64vec2 Z = C;
    int nbIters = 1;

    while (all(lessThan(abs(Z), two)) && nbIters < maxIters) {
        i64vec2 Z2 = squareFixed(Z);
        Z = i64vec2(Z2.x, squareFixed(Z.x + Z.y) - Z2.x) - Z2.y + C;
        nbIters++;
    }

    return vec4(Colorize(vec2(1.0lf / (1ul << INT64_SHIFT) * Z), nbIters), 1.0);
}

void main() {
    dvec2 Coord = ((gl_FragCoord.xy - 0.5 * resolution) / resolution.y) * zoom + offset;

    FragColor = vec4(0.0);
    for (float y = -0.5; y < 0.5; y += subPixel) {
    for (float x = -0.5; x < 0.5; x += subPixel) {
        dvec2 C = (vec2(x, y) / resolution.y) * zoom + Coord;
        switch (version) {
            case FP16 : FragColor += fp16(hvec2(C)); break;
            case FP32 : FragColor += fp32(vec2(C)); break;
            case FP64 : FragColor += fp64(C); break;
            case INT32: FragColor += int32(ivec2(initFixed(C) >> INT64_SHIFT - INT32_SHIFT)); break;
            case INT64: FragColor += int64(initFixed(((gl_FragCoord.xy - 0.5 * resolution + vec2(x, y)) / resolution.y) * zoom) + initFixed(offset)); break;
        }
    }}

    FragColor.rgb /= FragColor.w;
}
