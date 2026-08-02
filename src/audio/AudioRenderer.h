#pragma once
#include <godot_cpp/classes/resource.hpp>
#include "video/VideoRenderer.h"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswresample/swresample.h>
    #include <libavutil/opt.h>
    #include <libavutil/audio_fifo.h>
}

namespace godot {

class AudioRenderer: public Resource {

    GDCLASS(AudioRenderer, Resource);

private:
    String container_name = "mp3";
    String encoder_name = "libmp3lame";

    int sample_rate = 48000;
    int channels = 2;

    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    AVStream* stream = nullptr;
    SwrContext* swr_ctx = nullptr;
    AVAudioFifo* audio_fifo = nullptr;
    AVFrame* input_frame = nullptr;
    AVFrame* output_frame = nullptr;

    int stream_idx = -1;

    int64_t curr_pts = 0;

    void _process_fifo(AVFrame* frame);
    void _encode_and_write(AVFrame* frame);


protected:
    static void _bind_methods();

public:
    AudioRenderer();
    ~AudioRenderer();

    String get_container_name() const {return container_name;}
    void set_container_name(String new_container_name) {container_name = new_container_name;}
    String get_encoder_name() const {return encoder_name;}
    void set_encoder_name(String new_encoder_name) {encoder_name = new_encoder_name;}

    int get_sample_rate() const {return sample_rate;}
    void set_sample_rate(int new_sample_rate) {sample_rate = new_sample_rate;}
    int get_channels() const {return channels;}
    void set_channels(int new_channels) {channels = new_channels;}
    
    void prepare_format_ctx(const String &output_path);
    void share_video_renderer_format_ctx(Ref<VideoRenderer> video_renderer);

    bool start();

    bool open_output_file();

    void push_samples(PackedByteArray samples);
    
    bool finish();

    bool close_output_file();

};
}
