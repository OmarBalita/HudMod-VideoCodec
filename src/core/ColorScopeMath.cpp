#include "core/ColorScopeMath.hpp"

namespace godot {

    void ColorScopeMath::_bind_methods() {
        ClassDB::bind_static_method("ColorScopeMath", D_METHOD("calculate", "image", "image_data", "samples_down_scale"), &ColorScopeMath::calculate);
    }

    ColorScopeMath::ColorScopeMath() {}
    ColorScopeMath::~ColorScopeMath() {}


    Dictionary ColorScopeMath::calculate(Ref<Image> image, PackedByteArray image_data, int samples_down_scale) {

        if (image.is_null() || image_data.is_empty()) {
            return Dictionary();
        }

        int width = image->get_width();
        int height = image->get_height();
        
        const uint8_t* raw_ptr = image_data.ptr();
        
        int64_t data_size = image_data.size();
        int channels = (int)(data_size / (width * height));

        int width_ds = width / samples_down_scale;
        int height_ds = height / samples_down_scale;
        float pixel_opacity = 0.03f * samples_down_scale;

        std::vector<Vector4> h_data(256, Vector4(0, 0, 0, 0));
        std::vector<Vector4> w_data(width_ds * 256, Vector4(0, 0, 0, 0));

        for (int x_step = 0; x_step < width_ds; x_step++) {
            int x = x_step * samples_down_scale;
            int w_data_offset = x_step * 256;

            for (int y_step = 0; y_step < height_ds; y_step++) {
                int y = y_step * samples_down_scale;
                
                if (x >= width || y >= height) continue;

                int64_t idx = ((int64_t)y * width + x) * channels;

                if (idx + 2 >= data_size) continue;

                uint8_t r = raw_ptr[idx];
                uint8_t g = raw_ptr[idx + 1];
                uint8_t b = raw_ptr[idx + 2];
                
                uint8_t lum = (uint8_t)(r * 0.299f + g * 0.587f + b * 0.114f);

                h_data[r].x += 1.0f;
                h_data[g].y += 1.0f;
                h_data[b].z += 1.0f;
                h_data[lum].w += 1.0f;

                w_data[w_data_offset + r].x += pixel_opacity;
                w_data[w_data_offset + g].y += pixel_opacity;
                w_data[w_data_offset + b].z += pixel_opacity;
                w_data[w_data_offset + lum].w += pixel_opacity;
            }
        }

        Ref<Image> r_img = Image::create_empty(width_ds, 256, false, Image::FORMAT_LA8);
        Ref<Image> g_img = Image::create_empty(width_ds, 256, false, Image::FORMAT_LA8);
        Ref<Image> b_img = Image::create_empty(width_ds, 256, false, Image::FORMAT_LA8);
        Ref<Image> l_img = Image::create_empty(width_ds, 256, false, Image::FORMAT_LA8);

        for (int x = 0; x < width_ds; ++x) {
            int w_data_offset = x * 256;
            for (int y = 0; y < 256; ++y) {
                Vector4 val = w_data[w_data_offset + y];
                int inv_y = 255 - y;

                r_img->set_pixel(x, inv_y, Color(1, 1, 1, Math::min(val.x, 1.0f)));
                g_img->set_pixel(x, inv_y, Color(1, 1, 1, Math::min(val.y, 1.0f)));
                b_img->set_pixel(x, inv_y, Color(1, 1, 1, Math::min(val.z, 1.0f)));
                l_img->set_pixel(x, inv_y, Color(1, 1, 1, Math::min(val.w, 1.0f)));
            }
        }

        Dictionary result;
        result["resolution"] = Vector2i(width, height);
        
        Array h_data_array;
        for(const auto& v : h_data) h_data_array.push_back(v);
        result["histogram"] = h_data_array;
        
        Dictionary w_dict;
        for (int x = 0; x < width_ds; ++x) {
            Array column;
            for (int y = 0; y < 256; ++y) {
                column.push_back(w_data[x * 256 + y]);
            }
            w_dict[x] = column;
        }
        result["waveform"] = w_dict;

        result["r_img"] = ImageTexture::create_from_image(r_img);
        result["g_img"] = ImageTexture::create_from_image(g_img);
        result["b_img"] = ImageTexture::create_from_image(b_img);
        result["lum_img"] = ImageTexture::create_from_image(l_img);

        return result;
    }

}
