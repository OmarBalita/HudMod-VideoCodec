#include "VideoDecoder.hpp"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

#include <iostream>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/imgutils.h>
}

void VideoDecoder::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_video_path", "video_path"), &VideoDecoder::set_video_path);
    ClassDB::bind_method(D_METHOD("get_video_path"), &VideoDecoder::get_video_path);
    ClassDB::bind_method(D_METHOD("set_low_res", "low_res"), &VideoDecoder::set_low_res);
    ClassDB::bind_method(D_METHOD("get_low_res"), &VideoDecoder::get_low_res);
    ClassDB::bind_method(D_METHOD("set_skip_frame", "skip_frame"), &VideoDecoder::set_skip_frame);
    ClassDB::bind_method(D_METHOD("get_skip_frame"), &VideoDecoder::get_skip_frame);
    ClassDB::bind_method(D_METHOD("set_internal_enhance", "internal_enhance"), &VideoDecoder::set_internal_enhance);
    ClassDB::bind_method(D_METHOD("get_internal_enhance"), &VideoDecoder::get_internal_enhance);
    
    ClassDB::bind_method(D_METHOD("open"), &VideoDecoder::open);
    ClassDB::bind_method(D_METHOD("init_from_video_decoder", "video_decoder"), &VideoDecoder::init_from_video_decoder);
    ClassDB::bind_method(D_METHOD("close"), &VideoDecoder::close);

    ClassDB::bind_method(D_METHOD("get_width"), &VideoDecoder::get_width);
    ClassDB::bind_method(D_METHOD("get_height"), &VideoDecoder::get_height);
    ClassDB::bind_method(D_METHOD("get_resolution"), &VideoDecoder::get_resolution);
    ClassDB::bind_method(D_METHOD("get_duration"), &VideoDecoder::get_duration);
    ClassDB::bind_method(D_METHOD("get_fps"), &VideoDecoder::get_fps);
    ClassDB::bind_method(D_METHOD("get_total_frames_native"), &VideoDecoder::get_total_frames_native);
    ClassDB::bind_method(D_METHOD("get_total_frames_by_dur"), &VideoDecoder::get_total_frames_by_dur);
    ClassDB::bind_method(D_METHOD("get_total_frames_by_timebase"), &VideoDecoder::get_total_frames_by_timebase);
    ClassDB::bind_method(D_METHOD("get_bit_depth"), &VideoDecoder::get_bit_depth);
    ClassDB::bind_method(D_METHOD("get_color_space"), &VideoDecoder::get_color_space);
    ClassDB::bind_method(D_METHOD("get_color_matrix_idx"), &VideoDecoder::get_color_matrix_idx);
    ClassDB::bind_method(D_METHOD("get_color_range"), &VideoDecoder::get_color_range);
    ClassDB::bind_method(D_METHOD("get_channels_dim"), &VideoDecoder::get_channels_dim);
    
    ClassDB::bind_method(D_METHOD("get_curr_frame"), &VideoDecoder::get_curr_frame);
    
    ClassDB::bind_method(D_METHOD("read_next_frame"), &VideoDecoder::read_next_frame);
    ClassDB::bind_method(D_METHOD("read_prev_frame"), &VideoDecoder::read_prev_frame);
    ClassDB::bind_method(D_METHOD("skip_frames", "steps"), &VideoDecoder::skip_frames);
    ClassDB::bind_method(D_METHOD("skip_next_frames", "steps"), &VideoDecoder::skip_next_frames);
    ClassDB::bind_method(D_METHOD("seek_frame", "frame"), &VideoDecoder::seek_frame);
    ClassDB::bind_method(D_METHOD("seek_frame_smart", "frame"), &VideoDecoder::seek_frame_smart);
    
    ClassDB::bind_method(D_METHOD("update_video_data", "scale_factor"), &VideoDecoder::update_video_data);
    ClassDB::bind_method(D_METHOD("get_video_data"), &VideoDecoder::get_video_data);
    ClassDB::bind_method(D_METHOD("set_video_data"), &VideoDecoder::set_video_data);

    ClassDB::bind_method(D_METHOD("update_video_channels", "scale_factor"), &VideoDecoder::update_video_channels);
    ClassDB::bind_method(D_METHOD("get_channel_y"), &VideoDecoder::get_channel_y);
    ClassDB::bind_method(D_METHOD("get_channel_u"), &VideoDecoder::get_channel_u);
    ClassDB::bind_method(D_METHOD("get_channel_v"), &VideoDecoder::get_channel_v);
    ClassDB::bind_method(D_METHOD("set_channel_y"), &VideoDecoder::set_channel_y);
    ClassDB::bind_method(D_METHOD("set_channel_u"), &VideoDecoder::set_channel_u);
    ClassDB::bind_method(D_METHOD("set_channel_v"), &VideoDecoder::set_channel_v);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "video_path", PROPERTY_HINT_GLOBAL_FILE), "set_video_path", "get_video_path");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "low_res"), "set_low_res", "get_low_res");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "skip_frame"), "set_skip_frame", "get_skip_frame");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "internal_enhance"), "set_internal_enhance", "get_internal_enhance");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "video_data"), "set_video_data", "get_video_data");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "channel_y"), "set_channel_y", "get_channel_y");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "channel_u"), "set_channel_u", "get_channel_u");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "channel_v"), "set_channel_v", "get_channel_v");
}


VideoDecoder::VideoDecoder() {
    video_stream_idx = -1;
    video_path = "";
}

VideoDecoder::~VideoDecoder() {
    close();
}

godot::String VideoDecoder::get_video_path() const {return video_path;}
void VideoDecoder::set_video_path(const godot::String &p_video_path) {video_path = p_video_path;}
int VideoDecoder::get_low_res() const {return low_res;}
void VideoDecoder::set_low_res(const int &p_low_res) {low_res = p_low_res;}
bool VideoDecoder::get_skip_frame() const {return skip_frame;}
void VideoDecoder::set_skip_frame(const bool &p_skip_frame) {skip_frame = p_skip_frame;}
bool VideoDecoder::get_internal_enhance() const {return internal_enhance;}
void VideoDecoder::set_internal_enhance(const bool &p_internal_enhance) {internal_enhance = p_internal_enhance;}

bool VideoDecoder::open() {
    close();
    
    CharString path_utf8 = video_path.utf8();
    const char* c_video_path = path_utf8.get_data();
    
    if (avformat_open_input(&format_context, c_video_path, nullptr, nullptr) < 0) {
        godot::UtilityFunctions::printerr("Failed to open video file.");
        return false;
    }

    if (avformat_find_stream_info(format_context, nullptr) < 0) {
        close();
        return false;
    }

    video_stream_idx = find_video_stream_idx();
    if (video_stream_idx == -1) {
        close();
        return false;
    }

    AVStream* stream = format_context->streams[video_stream_idx];
    codec_params = stream->codecpar;
    codec = avcodec_find_decoder(codec_params->codec_id);

    if (!codec) {
        close();
        return false;
    }

    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        close();
        return false;
    }

    if (avcodec_parameters_to_context(codec_context, codec_params) < 0) {
        close();
        return false;
    }

    codec_context->thread_count = 0;
    codec_context->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
    codec_context->lowres = UtilityFunctions::clampi(low_res, 0, 3);

    codec_context->skip_frame = skip_frame ? AVDISCARD_NONREF : AVDISCARD_NONE;
    
    if (!internal_enhance) {
        codec_context->skip_loop_filter = AVDISCARD_ALL;
        codec_context->skip_idct = AVDISCARD_ALL;
        codec_context->flags2 |= AV_CODEC_FLAG2_FAST;
    }

    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "threads", "auto", 0);

    if (avcodec_open2(codec_context, codec, &opts) < 0) {
        av_dict_free(&opts);
        close();
        return false;
    }
    av_dict_free(&opts);

    packet = av_packet_alloc();
    frame = av_frame_alloc();

    if (!packet || !frame) {
        close();
        return false;
    }

    return true;
}

int VideoDecoder::find_video_stream_idx() {
    int result = -1;

    for (unsigned int i = 0; i < format_context->nb_streams; ++i) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            result = i;
            break;
        }
    }

    if (result == -1) {
        godot::print_error("Failed to find video stream");
    }
    return result;
}

bool VideoDecoder::init_from_video_decoder(const Ref<VideoDecoder> &existing_decoder) {
    if (existing_decoder.is_null()) {
        return false;
    }

    close();

    this->video_path = existing_decoder->video_path;

    CharString path_utf8 = video_path.utf8();

    AVDictionary* options = nullptr;
    av_dict_set(&options, "probesize", "32", 0);
    av_dict_set(&options, "analyzeduration", "0", 0);
    av_dict_set(&options, "fflags", "nobuffer", 0);

    if (avformat_open_input(&format_context, path_utf8.get_data(), nullptr, &options) < 0) {
        return false;
    }

    this->video_stream_idx = existing_decoder->video_stream_idx;
    this->codec = existing_decoder->codec; 

    if (!codec) {
        close();
        return false;
    }

    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        close();
        return false;
    }

    if (avcodec_parameters_to_context(codec_context, existing_decoder->format_context->streams[video_stream_idx]->codecpar) < 0) {
        close();
        return false;
    }

    codec_context->thread_count = 0;
    codec_context->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
    
    codec_context->lowres = godot::UtilityFunctions::clampi(this->low_res, 0, 3);

    codec_context->skip_frame = this->skip_frame ? AVDISCARD_NONREF : AVDISCARD_NONE;

    if (!this->internal_enhance) {
        codec_context->skip_loop_filter = AVDISCARD_ALL;
        codec_context->skip_idct = AVDISCARD_ALL;
        codec_context->flags2 |= AV_CODEC_FLAG2_FAST;
    }

    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "threads", "auto", 0);

    if (avcodec_open2(codec_context, codec, &opts) < 0) {
        av_dict_free(&opts);
        close();
        return false;
    }
    av_dict_free(&opts);

    packet = av_packet_alloc();
    frame = av_frame_alloc();

    if (!packet || !frame) {
        close();
        return false;
    }

    return true;
}

void VideoDecoder::close() {

    if (sws_ctx) {
        sws_freeContext(sws_ctx);
        sws_ctx = nullptr;
    }
    if (scale_sws_ctx) {
        sws_freeContext(scale_sws_ctx);
        scale_sws_ctx = nullptr;
    }
    if (codec_context) {
        avcodec_free_context(&codec_context);
        codec_context = nullptr;
    }
    if (format_context) {
        avformat_close_input(&format_context);
        format_context = nullptr;
    }
    if (packet) {
        av_packet_free(&packet);
        packet = nullptr;
    }
    if (frame) {
        av_frame_free(&frame);
        frame = nullptr;
    }
    if (scaled_frame) {
        av_frame_free(&scaled_frame);
        scaled_frame = nullptr;
    }
    video_stream_idx = -1;
    curr_frame = 0;
    codec = nullptr;
}

int VideoDecoder::get_width() const {return codec_params->width;}
int VideoDecoder::get_height() const {return codec_params->height;}
godot::Vector2i VideoDecoder::get_resolution() const {return Vector2i(get_width(), get_height());}
double VideoDecoder::get_duration() const {return (double)format_context->duration / AV_TIME_BASE;}
double VideoDecoder::get_fps() const {
    AVStream* stream = format_context->streams[video_stream_idx];
    return av_q2d(stream->avg_frame_rate);
}
int64_t VideoDecoder::get_total_frames_native() {return format_context->streams[video_stream_idx]->nb_frames;}
int64_t VideoDecoder::get_total_frames_by_dur() {return (int64_t)(get_duration() * get_fps());}
int64_t VideoDecoder::get_total_frames_by_timebase() {
    AVStream* stream = format_context->streams[video_stream_idx];
    double stream_dur = stream->duration * av_q2d(stream->time_base);
    return (int64_t)(stream_dur * get_fps());
}
int VideoDecoder::get_bit_depth() {
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get((AVPixelFormat)codec_context->pix_fmt);
    if (!desc) {return 8;}
    return desc->comp[0].depth;
}
int VideoDecoder::get_color_space() {return codec_context->colorspace;}
int VideoDecoder::get_color_matrix_idx() {
    switch (get_color_space()) {
        case AVCOL_SPC_BT709:
        default:
            return 0;
        case AVCOL_SPC_BT2020_NCL:
        case AVCOL_SPC_BT2020_CL:
            return 1;
        case AVCOL_SPC_BT470BG:
        case AVCOL_SPC_SMPTE170M:
        case AVCOL_SPC_SMPTE240M:
            return 2;
        }
}
int VideoDecoder::get_color_range() {return codec_context->color_range;}

godot::Dictionary VideoDecoder::get_channels_dim() {
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(codec_context->pix_fmt);
    godot::Dictionary dims;

    if (!desc) {
        dims["y"] = Vector2i(0, 0);
        dims["uv"] = Vector2i(0, 0);
        return dims;
    }

    int y_w = codec_context->width;
    int y_h = codec_context->height;

    int uv_w = AV_CEIL_RSHIFT(y_w, desc->log2_chroma_w);
    int uv_h = AV_CEIL_RSHIFT(y_h, desc->log2_chroma_h);

    dims["y"] = Vector2i(y_w, y_h);
    dims["uv"] = Vector2i(uv_w, uv_h);

    return dims;
}

int VideoDecoder::get_curr_frame() const {return curr_frame;}

bool VideoDecoder::read_next_frame() {
    int ret;
    while (av_read_frame(format_context, packet) >= 0) {
        if (packet->stream_index == video_stream_idx) {
            ret = avcodec_send_packet(codec_context, packet);
            if (ret < 0) {
                av_packet_unref(packet);
                return false;
            }

            ret = avcodec_receive_frame(codec_context, frame);
            if (ret == 0) {
                curr_frame++;
                av_packet_unref(packet);
                return true;
            } else if (ret == AVERROR(EAGAIN)) {
                av_packet_unref(packet);
                continue;
            }
        }
        av_packet_unref(packet);
    }
    return false;
}

bool VideoDecoder::read_prev_frame() {
    return skip_frames(-1);
}

bool VideoDecoder::skip_frames(int steps) {
    int target_frame = curr_frame + steps;
    if (target_frame < 0) return false;
    return seek_frame(target_frame);
}

bool VideoDecoder::skip_next_frames(int steps) {
    steps = godot::UtilityFunctions::clampi(steps, 1, INT32_MAX);
    for (int i = 0; i < steps; i++) {
        if (!read_next_frame()) return true;
    }
    return true;
}

bool VideoDecoder::seek_frame(int p_frame) {
    if (!format_context || video_stream_idx == -1) return false;

    AVStream* stream = format_context->streams[video_stream_idx];
    
    double fps = av_q2d(stream->avg_frame_rate);
    if (fps <= 0) fps = 24.0;

    double time_in_seconds = (double)p_frame / fps;

    int64_t target_pts = av_rescale_q(
        (int64_t)(time_in_seconds * AV_TIME_BASE), 
        AV_TIME_BASE_Q, 
        stream->time_base
    );

    if (av_seek_frame(format_context, video_stream_idx, target_pts, AVSEEK_FLAG_BACKWARD) < 0) {
        godot::UtilityFunctions::printerr("FFmpeg seek failed.");
        return false;
    }

    avcodec_flush_buffers(codec_context);

    bool success = seek_exact_frame(p_frame); 

    if (success) {
        curr_frame = p_frame;
    }

    return success;
}

bool VideoDecoder::seek_exact_frame(int target_frame) {
    AVStream* stream = format_context->streams[video_stream_idx];
    int retry_count = 0;
    
    const int max_retries = 1000;

    while (retry_count < max_retries) {
        if (av_read_frame(format_context, packet) < 0) return false;

        if (packet->stream_index == video_stream_idx) {
            if (avcodec_send_packet(codec_context, packet) >= 0) {
                while (avcodec_receive_frame(codec_context, frame) >= 0) {
                    
                    int64_t pts = (frame->pts != AV_NOPTS_VALUE) ? frame->pts : frame->pkt_dts;
                    
                    double current_time = pts * av_q2d(stream->time_base);
                    int current_frame_num = (int)(current_time * av_q2d(stream->avg_frame_rate) + 0.5);

                    if (current_frame_num >= target_frame) {
                        av_packet_unref(packet);
                        return true;
                    }
                    av_frame_unref(frame);
                }
            }
            retry_count++;
        }
        av_packet_unref(packet);
    }
    return false;
}

bool VideoDecoder::seek_frame_smart(int p_frame) {
    int steps = p_frame - curr_frame;
    if (steps == 1) return read_next_frame();
    else if (steps > 1) {
        if (steps > 35) return seek_frame(p_frame);
        else return skip_next_frames(steps);
    }
    else return seek_frame(p_frame);
}

void VideoDecoder::scale_frame(float scale_factor) {
    if (!frame || !frame->data[0]) return;

    int new_w = (int)(codec_context->width * scale_factor);
    int new_h = (int)(codec_context->height * scale_factor);
    
    scale_sws_ctx = sws_getCachedContext(scale_sws_ctx,
        frame->width, frame->height, (AVPixelFormat)frame->format,
        new_w, new_h, (AVPixelFormat)frame->format, 
        SWS_FAST_BILINEAR, NULL, NULL, NULL);

    if (!scaled_frame) scaled_frame = av_frame_alloc();
    if (scaled_frame->width != new_w || scaled_frame->height != new_h) {
        av_frame_unref(scaled_frame);
        scaled_frame->format = frame->format;
        scaled_frame->width = new_w;
        scaled_frame->height = new_h;
        av_frame_get_buffer(scaled_frame, 0);
    }

    sws_scale(scale_sws_ctx, frame->data, frame->linesize, 0, frame->height, scaled_frame->data, scaled_frame->linesize);
}

void VideoDecoder::update_video_data(float scale_factor) {
    AVFrame* source_frame = get_source_frame(scale_factor);
    if (!source_frame || !source_frame->data[0]) return;
    
    int width = codec_context->width;
    int height = codec_context->height;

    int data_size = width * height * 3;
    if (video_data.size() != data_size) {
        video_data.resize(data_size);
    }
    
    if (!sws_ctx) {
        sws_ctx = sws_getContext(
            width, height, (AVPixelFormat)source_frame->format,
            width, height, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, NULL, NULL, NULL
        );
    }
    
    uint8_t* dest_data[4] = { video_data.ptrw(), NULL, NULL, NULL };
    int dest_linesize[4] = { 3 * width, 0, 0, 0 };

    sws_scale(sws_ctx, source_frame->data, source_frame->linesize, 0, height, dest_data, dest_linesize);
}
godot::PackedByteArray VideoDecoder::get_video_data() const {
    return video_data;
}
void VideoDecoder::set_video_data(const PackedByteArray &p_video_data) {
    video_data = p_video_data;
}

void VideoDecoder::update_video_channels(float scale_factor) {
    AVFrame* source_frame = get_source_frame(scale_factor);

    if (!source_frame || !source_frame->data[0]) return;

    int width = source_frame->width; 
    int height = source_frame->height;

    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get((AVPixelFormat)source_frame->format);
    int bytes_per_pixel = (desc->comp[0].depth > 8) ? 2 : 1;

    for (int i = 0; i < 3; ++i) {
        int curr_w = (i == 0) ? width : AV_CEIL_RSHIFT(width, desc->log2_chroma_w);
        int curr_h = (i == 0) ? height : AV_CEIL_RSHIFT(height, desc->log2_chroma_h);
        
        int plane_size_in_bytes = curr_w * curr_h * bytes_per_pixel;
        
        PackedByteArray *target_array;
        if (i == 0) target_array = &channel_y;
        else if (i == 1) target_array = &channel_u;
        else target_array = &channel_v;

        if (target_array->size() != plane_size_in_bytes) {
            target_array->resize(plane_size_in_bytes);
        }

        uint8_t* src_data = source_frame->data[i];
        int src_linesize = source_frame->linesize[i];
        uint8_t* dest_ptr = target_array->ptrw();

        int line_width_bytes = curr_w * bytes_per_pixel;
        for (int y = 0; y < curr_h; ++y) {
            memcpy(dest_ptr + (y * line_width_bytes), src_data + (y * src_linesize), line_width_bytes);
        }
    }
}

AVFrame* VideoDecoder::get_source_frame(float scale_factor) {
    AVFrame* source_frame = nullptr;
    if (scale_factor >= 1.0f) {
        source_frame = frame;
    }
    else {
        scale_frame(scale_factor);
        source_frame = scaled_frame;
    }
    return source_frame;
}

godot::PackedByteArray VideoDecoder::get_channel_y() const {return channel_y;}
godot::PackedByteArray VideoDecoder::get_channel_u() const {return channel_u;}
godot::PackedByteArray VideoDecoder::get_channel_v() const {return channel_v;}

void VideoDecoder::set_channel_y(const godot::PackedByteArray &p_channel_y) {channel_y = p_channel_y;}
void VideoDecoder::set_channel_u(const godot::PackedByteArray &p_channel_u) {channel_u = p_channel_u;}
void VideoDecoder::set_channel_v(const godot::PackedByteArray &p_channel_v) {channel_v = p_channel_v;}
