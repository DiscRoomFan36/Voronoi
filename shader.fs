#version 330

in vec2 fragTexCoord;

out vec4 finalColor;

// The bottleneck, we cant have to many points, the shader cant handle it.
#define MAX_POINTS 256

uniform int num_points;
uniform vec2 points[MAX_POINTS];
uniform int colors[MAX_POINTS]; // packed RGBA color (an [r, g, b, a] array)

uniform int width;
uniform int height;

void main() {
    if (num_points > MAX_POINTS) {
        // this is an error!
        finalColor = vec4(0, 0, 0, 1);
        return;
    }

    vec2 pos = vec2(fragTexCoord.x * width, fragTexCoord.y * height);

    // flip the y because of handed-ness
    pos.y = height - pos.y;

    // find the closest
    int close_index = 0;
    float d1 = length(pos - points[0]);
    for (int i = 0; i < num_points; i++) {
        float d2 = length(pos - points[i]);
        if (d2 < d1) {
            d1 = d2;
            close_index = i;
        }
    }

    int color = colors[close_index];

    float r = ((color >> 0*8) & 0xFF) / 255.0;
    float g = ((color >> 1*8) & 0xFF) / 255.0;
    float b = ((color >> 2*8) & 0xFF) / 255.0;
    float a = ((color >> 3*8) & 0xFF) / 255.0;

    finalColor = vec4(r, g, b, a);
}