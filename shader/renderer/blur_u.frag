#version 330

in vec2 coord;
out vec3 result;

uniform sampler2D image_tex;
uniform vec2 frame_size;
uniform int radius = 2;


vec3 fetch(in float u, in float v)
{
	return texture2D(image_tex, coord + vec2(u, v) / frame_size).rgb;
}

void main()
{
	vec3 sum = vec3(0); int n = 0;
	for(int u = -radius; u <= radius; ++u)
	{
		sum += fetch(u, 0);
		n++;
	}
	result = sum / n;
}
