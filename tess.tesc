#version 460
layout(vertices = 3) out;
layout(binding = 0) uniform Data { vec4 values[4]; } data;
void main() {
    uint index = min(uint(data.values[0].x), 2u);
    gl_out[gl_InvocationID].gl_Position = gl_in[index].gl_Position + data.values[1];
    gl_TessLevelOuter[0] = 1.0;
    gl_TessLevelOuter[1] = 1.0;
    gl_TessLevelOuter[2] = 1.0;
    gl_TessLevelInner[0] = 1.0;
}
