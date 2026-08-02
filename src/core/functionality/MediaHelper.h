#pragma once
#include "godot_cpp/classes/resource.hpp"
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/gradient.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <unordered_map>

namespace godot {

class MediaHelper : public Resource {
	GDCLASS(MediaHelper, Resource)

private:
	static Ref<Gradient> waveform_gradient;
	static std::unordered_map<int64_t, PackedByteArray> audio_data_ids;

	typedef void (*DrawWaveformAction)(Ref<Image> image, int width, int height, int line_width, int x, double sample);

protected:
	static void _bind_methods();

public:
	static Ref<Gradient> get_waveform_gradient();
	static void set_waveform_gradient(const Ref<Gradient> &new_val);

	static void push_audio_data(int64_t data_id, const PackedByteArray &data);
	static void free_audio_data(int64_t data_id);

	static Ref<Image> generate_waveform_image(int64_t data_id, double second_from, double second_to, Image::Format image_format, int width, int height, int space_width, int line_width, int draw_method_idx, Color bg_color);

	static void draw_waveform_line_thumbnail(Ref<Image> image, int width, int height, int line_width, int x, double sample);
	static void draw_waveform_line_timeline(Ref<Image> image, int width, int height, int line_width, int x, double sample);
};

}
