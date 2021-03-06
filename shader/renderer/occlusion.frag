#version 330

in vec2 coord;
out vec3 image;

uniform sampler2D depth_tex;
uniform sampler2D normal_tex;
uniform sampler2D noise_tex;
uniform vec3      samples[16] = vec3[](vec3(0.53812504, 0.18565957, -0.43192),vec3(0.13790712, 0.24864247, 0.44301823),vec3(0.33715037, 0.56794053, -0.005789503),vec3(-0.6999805, -0.04511441, -0.0019965635),vec3(0.06896307, -0.15983082, -0.85477847),vec3(0.056099437, 0.006954967, -0.1843352),vec3(-0.014653638, 0.14027752, 0.0762037),vec3(0.010019933, -0.1924225, -0.034443386),vec3(-0.35775623, -0.5301969, -0.43581226),vec3(-0.3169221, 0.106360726, 0.015860917),vec3(0.010350345, -0.58698344, 0.0046293875),vec3(-0.08972908, -0.49408212, 0.3287904),vec3(0.7119986, -0.0154690035, -0.09183723),vec3(-0.053382345, 0.059675813, -0.5411899),vec3(0.035267662, -0.063188605, 0.54602677),vec3(-0.47761092, 0.2847911, -0.0271716));
uniform vec2      frame_size;
uniform vec2      noise_size = vec2(64, 64);
uniform int       noise_tiling = 5;


vec3 orientate(in vec3 value, in vec3 compare)
{
	return (dot(value, compare) < 0) ? -value : value;
}

void main()
{
	float z   = texture2D(depth_tex, coord).r;                                            // read eye linear z
	vec3  nor = texture2D(normal_tex, coord).xyz;                                         // read normal
	vec3  ref = texture2D(noise_tex, coord * noise_tiling * frame_size / noise_size).xyz; // read dithering vector

	float bl = 0.0;
	for(int i = 0; i < 16; ++i)
	{
		vec3  of = orientate(reflect(samples[i], ref), nor);
		vec2 cd = coord + 0.02 * of.xy;
		float sz = texture2D(depth_tex, cd).r;
		float zd = (z - sz) * 1500 * clamp(35 - 2800 * z, 1, 35);
		bl += (1-z) * clamp(zd, 0.0, 1.0);
	}
	image = vec3(1.0 - bl / 16.0);
}
