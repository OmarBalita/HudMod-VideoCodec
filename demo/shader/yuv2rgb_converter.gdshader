shader_type canvas_item;

uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;

uniform float bit_depth;
uniform float bit_max_val;

uniform bool is_full_range;
uniform int color_space;

void fragment() {
	float y = texture(tex_y, UV).r;
	float u = texture(tex_u, UV).r;
	float v = texture(tex_v, UV).r;
	
	float bit_scale = (exp2(bit_depth) - 1.0) / bit_max_val;
	y /= bit_scale;
	u /= bit_scale;
	v /= bit_scale;
	
	float max_val   = exp2(bit_depth) - 1.0;
	float uv_offset = exp2(bit_depth - 1.0);
	float y_min     = exp2(bit_depth - 4.0);
	float y_range   = exp2(bit_depth - 4.0) * 219.0 / 16.0;
	float uv_range  = exp2(bit_depth - 4.0) * 224.0 / 16.0;

	if (is_full_range) {
		u -= uv_offset / max_val;
		v -= uv_offset / max_val;
	} else {
		y = (y - y_min     / max_val) / (y_range  / max_val);
		u = (u - uv_offset / max_val) / (uv_range / max_val);
		v = (v - uv_offset / max_val) / (uv_range / max_val);
	}
	
	if (color_space == 0) {        // BT.709
		COLOR.r = y + 1.5748 * v;
		COLOR.g = y - 0.1873 * u - 0.4681 * v;
		COLOR.b = y + 1.8556 * u;
	} else if (color_space == 1) { // BT.2020
		COLOR.r = y + 1.4746 * v;
		COLOR.g = y - 0.1645 * u - 0.5713 * v;
		COLOR.b = y + 1.8814 * u;
	} else {                       // BT.601
		COLOR.r = y + 1.402 * v;
		COLOR.g = y - 0.344 * u - 0.714 * v;
		COLOR.b = y + 1.772 * u;
	}
}