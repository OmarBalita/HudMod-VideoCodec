#ifndef VIDEO_RENDERER
#define VIDEO_RENDERER


#include <cstring>

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/image.hpp>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/opt.h>
}

namespace godot {

    class VideoRenderer: public Resource {
        
        GDCLASS(VideoRenderer, Resource);

        private:

            enum EncoderType {
                ENC_UNKNOWN,
                ENC_H26x,
                ENC_VPX,
                ENC_AV1,
                ENC_LEGACY,
                ENC_PRORES,
                ENC_LOSSLESS
            };
            
            AVFormatContext *format_ctx = nullptr;
            AVCodecContext *codec_ctx = nullptr;
            AVStream *video_stream = nullptr;
            AVFrame *frame_input = nullptr;
            AVFrame *frame_output = nullptr;
            SwsContext *sws_ctx = nullptr;

            int64_t frame_count = 0;

            int width = 1920;
            int height = 1080;
            int fps = 30;

            int bit_rate_control_mode = 0;
            int bit_rate = 6000000;
            int crf_value; // between 0 and 51

            int color_space_idx = 0;
            bool is_color_range_limited = false;
            
            int quality_level = 3; // between 0 and 5

            String container_name = "mp4";
            String encoder_name = "libx264";
            String pixel_format = "";
            
            EncoderType _get_encoder_type(const String &encoder_name);
            void _setup_encoder_presets(AVCodecContext *ctx, const String &encoder_name, int quality_level);
            void _setup_codec_profile(AVCodecContext *ctx, const String &encoder_name);
        
        protected:
            static void _bind_methods();
        
        public:

            VideoRenderer();
            ~VideoRenderer();

            AVFormatContext* get_format_ctx() {return format_ctx;};

            int get_width() const {return width;}
            void set_width(int new_width) {width = new_width;}
            int get_height() const {return height;}
            void set_height(int new_height) {height = new_height;}
            int get_fps() const {return fps;}
            void set_fps(int new_fps) {fps = new_fps;}

            int get_bit_rate_control_mode() const {return bit_rate_control_mode;}
            void set_bit_rate_control_mode(int mode) {bit_rate_control_mode = mode;}
            int get_bit_rate() const {return bit_rate;}
            void set_bit_rate(int new_bit_rate) {bit_rate = new_bit_rate;}
            int get_crf_value() const {return crf_value;}
            void set_crf_value(int new_crf_value) {crf_value = new_crf_value;}

            int get_color_space_idx() const {return color_space_idx;}
            void set_color_space_idx(int new_color_space_idx) {color_space_idx = new_color_space_idx;}
            bool get_is_color_range_limited() const {return is_color_range_limited;}
            void set_is_color_range_limited(bool new_is_color_range_limited) {is_color_range_limited = new_is_color_range_limited;}

            int get_quality_level() const {return quality_level;}
            void set_quality_level(int new_quality_level) {quality_level = new_quality_level;}

            String get_container_name() const {return container_name;}
            void set_container_name(const String &new_container_name) {container_name = new_container_name;}
            String get_encoder_name() const {return encoder_name;}
            void set_encoder_name(const String &new_encoder_name) {encoder_name = new_encoder_name;}
            String get_pixel_format() const {return pixel_format;}
            void set_pixel_format(const String &new_pixel_format) {pixel_format = new_pixel_format;}

            PackedStringArray find_available_encoders_for_container();
            PackedStringArray find_available_pixel_formats_for_encoder();

            bool start(const String &output_path);
            bool open_output_file();
            bool send_frame(const Ref<Image> &image);
            bool finish();
            bool close_output_file();

    };
}

#endif