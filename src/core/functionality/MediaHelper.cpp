#include "core/functionality/MediaHelper.h"
#include <godot_cpp/core/class_db.hpp>
#include <algorithm>
#include <vector>

using namespace godot;

Ref<Gradient> MediaHelper::waveform_gradient;
std::unordered_map<int64_t, PackedByteArray> MediaHelper::audio_data_ids;

void MediaHelper::_bind_methods() {
	ClassDB::bind_static_method("MediaHelper", D_METHOD("get_waveform_gradient"), &MediaHelper::get_waveform_gradient);
	ClassDB::bind_static_method("MediaHelper", D_METHOD("set_waveform_gradient", "new_val"), &MediaHelper::set_waveform_gradient);

	ClassDB::bind_static_method("MediaHelper", D_METHOD("push_audio_data", "data_id", "data"), &MediaHelper::push_audio_data);
	ClassDB::bind_static_method("MediaHelper", D_METHOD("free_audio_data", "data_id"), &MediaHelper::free_audio_data);

	ClassDB::bind_static_method("MediaHelper", D_METHOD("generate_waveform_image", "data_id", "second_from", "second_to", "image_format", "width", "height", "space_width", "line_width", "draw_method_idx", "bg_color"), &MediaHelper::generate_waveform_image, DEFVAL(0), DEFVAL(Color()));
}

Ref<Gradient> MediaHelper::get_waveform_gradient() {
	return waveform_gradient;
}

void MediaHelper::set_waveform_gradient(const Ref<Gradient> &new_val) {
	waveform_gradient = new_val;
}

void MediaHelper::push_audio_data(int64_t data_id, const PackedByteArray &data) {
	audio_data_ids.try_emplace(data_id, data);
}

void MediaHelper::free_audio_data(int64_t data_id) {
	audio_data_ids.erase(data_id);
}

Ref<Image> MediaHelper::generate_waveform_image(int64_t data_id, double second_from, double second_to, Image::Format image_format, int width, int height, int space_width, int line_width, int draw_method_idx, Color bg_color) {
	auto it = audio_data_ids.find(data_id);
	if (it == audio_data_ids.end()) return nullptr;

	DrawWaveformAction draw_method = draw_method_idx == 0 ? draw_waveform_line_thumbnail : draw_waveform_line_timeline;

	Ref<Image> image = Image::create_empty(width, height, false, image_format);
	image->fill(bg_color);

	const PackedByteArray &raw_data = it->second;

	int channels = 2;
	int sample_rate = 48000;
	int bytes_per_sample = 4;
	int bytes_per_frame = bytes_per_sample * channels;

	const float *float_data = reinterpret_cast<const float *>(raw_data.ptr());
	int64_t float_count = raw_data.size() / bytes_per_sample;

	double length = raw_data.size() / (double)(bytes_per_frame * sample_rate);
	second_to = MIN(second_to, length);
	double duration = second_to - second_from;

	int start_sample = (int)(second_from * sample_rate);
	int total_samples_to_process = (int)(duration * sample_rate);

	int samples_per_pixel = total_samples_to_process / width;
	if (samples_per_pixel < 1) samples_per_pixel = 1;

	int step_width = space_width + line_width;
	int lines_count = width / step_width;

	int samples_step = 1;
	if (samples_per_pixel > 100000) samples_step = 10000;
	else if (samples_per_pixel > 10000) samples_step = 1000;
	else if (samples_per_pixel > 1000) samples_step = 100;
	else if (samples_per_pixel > 100) samples_step = 10;

	std::vector<float> amplitudes;
	amplitudes.reserve(lines_count);

	for (int x = 0; x < lines_count; x++) {
		int pixel_x = x * step_width;

		int64_t curr_sample_idx = (start_sample + (pixel_x * samples_per_pixel)) * channels;
		int64_t end_sample_idx = curr_sample_idx + (samples_per_pixel * channels);

		float max_amplitude = 0.0f;

		if (curr_sample_idx >= float_count) {
			amplitudes.push_back(0.0f);
			continue;
		}
		end_sample_idx = MIN(end_sample_idx, float_count);

		for (int64_t s = curr_sample_idx; s < end_sample_idx - 1; s += 2 * samples_step) {
			float abs1 = Math::abs(float_data[s]);
			float abs2 = Math::abs(float_data[s + 1]);

			float sample_value = (abs1 + abs2) * 0.5f;
			if (sample_value > max_amplitude) max_amplitude = sample_value;
		}

		amplitudes.push_back(max_amplitude);
	}

	float max_amplitude_glob = amplitudes.empty() ? 0.0f : *std::max_element(amplitudes.begin(), amplitudes.end());
	float amplitude_multiplier = max_amplitude_glob > 0.0f ? 1.0f / max_amplitude_glob : 1.0f;

	for (size_t idx = 0; idx < amplitudes.size(); idx++) {
		draw_method(image, width, height, line_width, (int)idx * step_width, amplitudes[idx] * amplitude_multiplier);
	}

	image->generate_mipmaps();
	return image;
}

void MediaHelper::draw_waveform_line_thumbnail(Ref<Image> image, int width, int height, int line_width, int x, double sample) {
	float offset = x / (float)image->get_width();
	int height_half = height / 2;
	int sample_height = (int)(sample * height);

	Vector2i pos = Vector2i(x, (int)(height_half - sample_height / 2.0f));
	Vector2i size = Vector2i(line_width, MAX(1, sample_height));

	image->fill_rect(Rect2i(pos, size), waveform_gradient->sample(offset));
}

void MediaHelper::draw_waveform_line_timeline(Ref<Image> image, int width, int height, int line_width, int x, double sample) {
	int height_half = height / 2;
	int sample_height = (int)(sample * height);

	Vector2i pos = Vector2i(x, (int)(height_half - sample_height / 2.0f));
	Vector2i size = Vector2i(line_width, MAX(1, sample_height));

	Color color = Color(Color(0, 0, 0), 0.6f);
	image->fill_rect(Rect2i(pos, size), color);
}
