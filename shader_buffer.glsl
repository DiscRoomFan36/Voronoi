#version 430

in vec2 fragTexCoord;

out vec4 finalColor;


uniform int width;
uniform int height;

uniform int num_points;


layout(std430, binding = 1) buffer PositionBufferLayout {
    vec2 position_buffer[]; // positions of the points
};

layout(std430, binding = 2) buffer ColorBufferLayout {
    int color_buffer[]; // [r, g, b, a] array
};


void main() {
    if (num_points == 0) {
        finalColor = vec4(0, 0, 0, 1); // make this clear?
        return;
    }

    vec2 pos = vec2(fragTexCoord.x * width, fragTexCoord.y * height);

    // flip the y because of handed-ness
    pos.y = height - pos.y;

    // find the closest
    int close_index = 0;
    float d1 = length(pos - position_buffer[0]);
    for (int i = 0; i < num_points; i++) {
        float d2 = length(pos - position_buffer[i]);
        if (d2 < d1) {
            d1 = d2;
            close_index = i;
        }
    }

    int color = color_buffer[close_index];

    float r = ((color >> 0*8) & 0xFF) / 255.0;
    float g = ((color >> 1*8) & 0xFF) / 255.0;
    float b = ((color >> 2*8) & 0xFF) / 255.0;
    float a = ((color >> 3*8) & 0xFF) / 255.0;

    finalColor = vec4(r, g, b, a);
}