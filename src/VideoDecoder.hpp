
#ifndef VIDEO_DECODER
#define VIDEO_DECODER

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavutil/avutil.h>
}

namespace godot {

    class VideoDecoder: public Resource {
        GDCLASS(VideoDecoder, Resource);
        
        private:
            AVFormatContext* format_context = nullptr;
            int video_stream_idx;
            AVCodecParameters* codec_params = nullptr;
            const AVCodec* codec = nullptr;
            AVCodecContext* codec_context = nullptr;
            
            AVPacket* packet = nullptr;
            AVFrame* frame = nullptr;
            AVFrame* scaled_frame = nullptr;
            SwsContext* scale_sws_ctx = nullptr;
            int curr_frame;
            
            SwsContext* sws_ctx = nullptr;

            int find_video_stream_idx();
            bool seek_exact_frame(int target_frame);
            void scale_frame(float scale_factor);
            AVFrame* get_source_frame(float scale_factor);

        public:
            godot::String video_path;
            int low_res = 0;
            bool skip_frame = false;
            bool internal_enhance = true;

            PackedByteArray video_data;
            PackedByteArray channel_y;
            PackedByteArray channel_u;
            PackedByteArray channel_v;
            
            godot::String get_video_path() const;
            void set_video_path(const godot::String &p_video_path);
            int get_low_res() const;
            void set_low_res(const int &p_low_res);
            bool get_skip_frame() const;
            void set_skip_frame(const bool &p_skip_frame);
            bool get_internal_enhance() const;
            void set_internal_enhance(const bool &internal_enhance);
            
            bool open();
            bool init_from_video_decoder(const Ref<VideoDecoder> &existing_decoder);
            void close();
            
            int get_width() const;
            int get_height() const;
            godot::Vector2i get_resolution() const;
            double get_duration() const;
            double get_fps() const;
            int64_t get_total_frames_native();
            int64_t get_total_frames_by_dur();
            int64_t get_total_frames_by_timebase();
            int get_bit_depth();
            int get_color_space();
            int get_color_matrix_idx();
            int get_color_range();
            godot::Dictionary get_channels_dim();

            int get_curr_frame() const;
            
            bool read_next_frame();
            bool read_prev_frame();
            bool skip_frames(int steps);
            bool skip_next_frames(int steps);
            bool seek_frame(int p_frame);
            bool seek_frame_smart(int p_frame);

            void update_video_data(float scale_factor = 1.0f);
            godot::PackedByteArray get_video_data() const;
            void set_video_data(const PackedByteArray &p_video_data);
            
            void update_video_channels(float scale_factor = 1.0f);
            godot::PackedByteArray get_channel_y() const;
            void set_channel_y(const godot::PackedByteArray &p_channel_y);
            godot::PackedByteArray get_channel_u() const;
            void set_channel_u(const godot::PackedByteArray &p_channel_u);
            godot::PackedByteArray get_channel_v() const;
            void set_channel_v(const godot::PackedByteArray &p_channel_v);

            VideoDecoder();
            ~VideoDecoder();
        
        protected:
            static void _bind_methods();
    };
}

#endif
