#include "VideoRenderer.hpp"
#include <godot_cpp/variant/utility_functions.hpp>


namespace godot {

    VideoRenderer::EncoderType VideoRenderer::_get_encoder_type(const String &name) {
        if (name == "libx264" || name == "libx265") return ENC_H26x;
        if (name == "libvpx-vp9" || name == "libvpx") return ENC_VPX;
        if (name == "libsvtav1") return ENC_AV1;
        if (name == "mpeg4" || name == "libxvid") return ENC_LEGACY;
        if (name == "prores") return ENC_PRORES;
        if (name == "ffv1") return ENC_LOSSLESS;
        return ENC_UNKNOWN;
    }


    void VideoRenderer::_setup_encoder_presets(AVCodecContext *ctx, const String &encoder_name, int quality_level) {

        quality_level = CLAMP(quality_level, 0, 5);
        
        EncoderType type = _get_encoder_type(encoder_name);

        switch (type) {
            case ENC_H26x: {
                const char* presets[] = {"ultrafast", "faster", "medium", "slow", "slower", "veryslow"};
                av_opt_set(ctx->priv_data, "preset", presets[quality_level], 0);
            } break;

            case ENC_VPX: {
                int cpu_used[] = {8, 6, 4, 2, 1, 0};
                av_opt_set(ctx->priv_data, "cpu-used", String::num(cpu_used[quality_level]).utf8().get_data(), 0);
                av_opt_set(ctx->priv_data, "row-mt", "1", 0);
            } break;

            case ENC_AV1: {
                int svt_presets[] = {12, 10, 8, 6, 4, 2};
                av_opt_set(ctx->priv_data, "preset", String::num(svt_presets[quality_level]).utf8().get_data(), 0);
            } break;

            case ENC_LEGACY: {
                ctx->mb_decision = (quality_level > 3) ? 2 : 1;
                if (quality_level >= 4) {
                    ctx->flags |= AV_CODEC_FLAG_4MV;
                }
            } break;

            case ENC_PRORES: {
            } break;

            case ENC_LOSSLESS: {
                ctx->level = 3; 
                av_opt_set(ctx->priv_data, "coder", (quality_level > 3) ? "1" : "0", 0);
                av_opt_set(ctx->priv_data, "context", "1", 0);
            } break;

            case ENC_UNKNOWN:
            default:
                UtilityFunctions::print("Warning: No specific preset logic for encoder: ", encoder_name);
                break;
        }
    }


    void VideoRenderer::_setup_codec_profile(AVCodecContext *ctx, const String &encoder_name) {
        if (!ctx || !ctx->priv_data) return;

        int bit_depth = 8;
        int chroma_format = 420;
        
        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(ctx->pix_fmt);
        if (desc) {
            bit_depth = desc->comp[0].depth;
            
            if (desc->nb_components >= 3) {
                if (desc->log2_chroma_w == 0 && desc->log2_chroma_h == 0) {
                    chroma_format = 444;
                } else if (desc->log2_chroma_w == 1 && desc->log2_chroma_h == 0) {
                    chroma_format = 422;
                } else {
                    chroma_format = 420;
                }
            }
        }

        EncoderType type = _get_encoder_type(encoder_name);

        switch (type) {
            case ENC_H26x: {
                const char* profile = nullptr;

                if (encoder_name == "libx264") {
                    if (chroma_format == 444) {
                        profile = "high444";
                    } else if (chroma_format == 422) {
                        profile = "high422";
                    } else if (bit_depth == 10) {
                        profile = "high10";
                    } else {
                        profile = "high";
                    }
                } 
                else if (encoder_name == "libx265") {
                    if (chroma_format == 444) {
                        if (bit_depth == 8)  profile = "main444-8";
                        else if (bit_depth == 10) profile = "main444-10";
                        else profile = "main444-12";
                    } else if (chroma_format == 422) {
                        if (bit_depth == 8)  profile = "main422-8";
                        else if (bit_depth == 10) profile = "main422-10";
                        else profile = "main422-12";
                    } else {
                        if (bit_depth == 10) profile = "main10";
                        else if (bit_depth == 12) profile = "main12";
                        else profile = "main";
                    }
                }
                if (profile) {
                    av_opt_set(ctx->priv_data, "profile", profile, 0);
                }
            } break;

            case ENC_VPX: {
                const char* vpx_profile = "0";
                if (bit_depth <= 8) {
                    vpx_profile = (chroma_format == 420) ? "0" : "1";
                } else {
                    vpx_profile = (chroma_format == 420) ? "2" : "3";
                }
                av_opt_set(ctx->priv_data, "profile", vpx_profile, 0);
            } break;

            case ENC_AV1: {
                const char* av1_profile = "main";
                if (chroma_format == 444 && bit_depth <= 10) {
                    av1_profile = "high";
                } else if (chroma_format == 422 || bit_depth == 12) {
                    av1_profile = "professional";
                }
                av_opt_set(ctx->priv_data, "profile", av1_profile, 0);
            } break;

            case ENC_PRORES: {
                const char* prores_profile = "3"; // Standard 'HQ'
                if (chroma_format == 444) prores_profile = "4"; // 4444
                else if (bit_depth >= 12) prores_profile = "5"; // 4444 XQ
                av_opt_set(ctx->priv_data, "profile", prores_profile, 0);
            } break;

            case ENC_LEGACY: {
                ctx->profile = AV_PROFILE_UNKNOWN; 
            } break;

            default:
                break;
        }
    }


    void VideoRenderer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_width"), &VideoRenderer::get_width);
        ClassDB::bind_method(D_METHOD("set_width", "new_width"), &VideoRenderer::set_width);
        ClassDB::bind_method(D_METHOD("get_height"), &VideoRenderer::get_height);
        ClassDB::bind_method(D_METHOD("set_height", "new_height"), &VideoRenderer::set_height);
        ClassDB::bind_method(D_METHOD("get_fps"), &VideoRenderer::get_fps);
        ClassDB::bind_method(D_METHOD("set_fps", "new_fps"), &VideoRenderer::set_fps);

        ClassDB::bind_method(D_METHOD("get_bit_rate_control_mode"), &VideoRenderer::get_bit_rate_control_mode);
        ClassDB::bind_method(D_METHOD("set_bit_rate_control_mode", "mode"), &VideoRenderer::set_bit_rate_control_mode);
        ClassDB::bind_method(D_METHOD("get_bit_rate"), &VideoRenderer::get_bit_rate);
        ClassDB::bind_method(D_METHOD("set_bit_rate", "new_bit_rate"), &VideoRenderer::set_bit_rate);
        ClassDB::bind_method(D_METHOD("get_crf_value"), &VideoRenderer::get_crf_value);
        ClassDB::bind_method(D_METHOD("set_crf_value", "new_crf_value"), &VideoRenderer::set_crf_value);

        ClassDB::bind_method(D_METHOD("get_color_space_idx"), &VideoRenderer::get_color_space_idx);
        ClassDB::bind_method(D_METHOD("set_color_space_idx", "new_color_space_idx"), &VideoRenderer::set_color_space_idx);
        ClassDB::bind_method(D_METHOD("get_is_color_range_limited"), &VideoRenderer::get_is_color_range_limited);
        ClassDB::bind_method(D_METHOD("set_is_color_range_limited", "new_is_color_range_limited"), &VideoRenderer::set_is_color_range_limited);

        ClassDB::bind_method(D_METHOD("get_quality_level"), &VideoRenderer::get_quality_level);
        ClassDB::bind_method(D_METHOD("set_quality_level", "new_quality_level"), &VideoRenderer::set_quality_level);

        ClassDB::bind_method(D_METHOD("get_container_name"), &VideoRenderer::get_container_name);
        ClassDB::bind_method(D_METHOD("set_container_name", "new_container_name"), &VideoRenderer::set_container_name);
        ClassDB::bind_method(D_METHOD("get_encoder_name"), &VideoRenderer::get_encoder_name);
        ClassDB::bind_method(D_METHOD("set_encoder_name", "new_encoder_name"), &VideoRenderer::set_encoder_name);
        ClassDB::bind_method(D_METHOD("get_pixel_format"), &VideoRenderer::get_pixel_format);
        ClassDB::bind_method(D_METHOD("set_pixel_format", "new_pixel_format"), &VideoRenderer::set_pixel_format);

        ClassDB::bind_method(D_METHOD("find_available_encoders_for_container"), &VideoRenderer::find_available_encoders_for_container);
        ClassDB::bind_method(D_METHOD("find_available_pixel_formats_for_encoder"), &VideoRenderer::find_available_pixel_formats_for_encoder);

        ClassDB::bind_method(D_METHOD("start", "output_path"), &VideoRenderer::start);
        ClassDB::bind_method(D_METHOD("open_output_file"), &VideoRenderer::open_output_file);
        ClassDB::bind_method(D_METHOD("send_frame", "frame"), &VideoRenderer::send_frame);
        ClassDB::bind_method(D_METHOD("finish"), &VideoRenderer::finish);
        ClassDB::bind_method(D_METHOD("close_output_file"), &VideoRenderer::close_output_file);
    }

    VideoRenderer::VideoRenderer() {

    }

    VideoRenderer::~VideoRenderer() {

    }
    
    PackedStringArray VideoRenderer::find_available_encoders_for_container() {

        PackedStringArray result;

        const AVOutputFormat *out_fmt = av_guess_format(container_name.utf8().get_data(), nullptr, nullptr);
        
        if (!out_fmt) {
            UtilityFunctions::printerr("Container not found: ", container_name);
            return result;
        }

        void *opaque = nullptr;
        const AVCodec *codec = nullptr;
        
        while ((codec = av_codec_iterate(&opaque)) != nullptr) {
            if (av_codec_is_encoder(codec) && codec->type == AVMEDIA_TYPE_VIDEO) {
                int support = avformat_query_codec(out_fmt, codec->id, 1);
                if (support == 1) {
                    result.append(String(codec->name));
                }
            }
        }

        return result;
    }

    PackedStringArray VideoRenderer::find_available_pixel_formats_for_encoder() {
        PackedStringArray result;
        const AVCodec* codec = avcodec_find_encoder_by_name(encoder_name.utf8().get_data());
        const AVPixelFormat* p = codec->pix_fmts;
        while (*p != AV_PIX_FMT_NONE) {
            result.append(String(av_get_pix_fmt_name(*p)));
            p++;
        }
        return result;
    }

    bool VideoRenderer::start(const String &output_path) {
        
        const char* output_path_c = output_path.utf8().get_data();

        const AVOutputFormat *out_fmt = av_guess_format(container_name.utf8().get_data(), nullptr, nullptr);
        if (!out_fmt) {
            UtilityFunctions::printerr("Unsupported container format: ", container_name);
            return false;
        }

        avformat_alloc_output_context2(&format_ctx, out_fmt, nullptr, output_path_c);
        if (!format_ctx) {
            UtilityFunctions::printerr("Failed to allocate format context.");
            return false;
        }

        const AVCodec* codec = avcodec_find_encoder_by_name(encoder_name.utf8().get_data());
        if (!codec) {
            UtilityFunctions::printerr("Codec not found or not supported in your FFmpeg: ", encoder_name);
            return false;
        }

        video_stream = avformat_new_stream(format_ctx, nullptr);
        if (!video_stream) {
            UtilityFunctions::printerr("Failed to create new stream.");
            return false;
        }
        
        codec_ctx = avcodec_alloc_context3(codec);
        if (!codec_ctx) {
            UtilityFunctions::printerr("Failed to allocate codec context.");
            return false;
        }

        codec_ctx->codec_id = codec->id;
        codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
        codec_ctx->width = width;
        codec_ctx->height = height;
        codec_ctx->time_base = {1, fps};
        codec_ctx->framerate = {fps, 1};

        switch (color_space_idx) {
            case 0:
                codec_ctx->color_primaries = AVCOL_PRI_BT709;
                codec_ctx->color_trc       = AVCOL_TRC_BT709;
                codec_ctx->colorspace      = AVCOL_SPC_BT709;
                break;
            case 1:
                codec_ctx->color_primaries = AVCOL_PRI_BT2020;
                codec_ctx->color_trc       = AVCOL_TRC_BT2020_10;
                codec_ctx->colorspace      = AVCOL_SPC_BT2020_NCL;
                break;
        }

        codec_ctx->pix_fmt = AV_PIX_FMT_NONE;

        if (codec->pix_fmts) {

            const enum AVPixelFormat* p = codec->pix_fmts;

            if (pixel_format != "") {
                enum AVPixelFormat pixel_format_id = av_get_pix_fmt(pixel_format.utf8().get_data());
                while (*p != AV_PIX_FMT_NONE) {
                    if (*p == pixel_format_id) {
                        codec_ctx->pix_fmt = pixel_format_id;
                        break;
                    }
                    p++;
                }
                if (codec_ctx->pix_fmt == AV_PIX_FMT_NONE) {
                    UtilityFunctions::print("Requested pix_fmt '", pixel_format, "' is not supported by ", encoder_name, ". Falling back to default.");
                }
            }

            if (codec_ctx->pix_fmt == AV_PIX_FMT_NONE) {
                codec_ctx->pix_fmt = codec->pix_fmts[0];
            }
        
        } else {
            codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
        }

        switch (bit_rate_control_mode) {
            case 0: // CRF
                av_opt_set(codec_ctx->priv_data, "crf", String::num(crf_value).utf8().get_data(), 0);
                break;
            case 1: // VBR
                codec_ctx->bit_rate = bit_rate;
                codec_ctx->rc_max_rate = bit_rate * 1.5; 
                codec_ctx->rc_buffer_size = bit_rate * 2;
                break;
            case 2: // CBR
                codec_ctx->bit_rate = bit_rate;
                codec_ctx->rc_max_rate = bit_rate;
                codec_ctx->rc_min_rate = bit_rate;
                codec_ctx->rc_buffer_size = bit_rate;
                codec_ctx->rc_initial_buffer_occupancy = bit_rate * 0.9;
                break;
            default:
                UtilityFunctions::print("Invalid bit rate control mode. Using default settings.");
        }
        av_opt_set(codec_ctx->priv_data, "profile", "high", 0);

        _setup_encoder_presets(codec_ctx, encoder_name, quality_level);
        _setup_codec_profile(codec_ctx, encoder_name);

        if (format_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
            UtilityFunctions::printerr("Cannot open video codec.");
            return false;
        }

        avcodec_parameters_from_context(video_stream->codecpar, codec_ctx);

        frame_input = av_frame_alloc();
        frame_output = av_frame_alloc();

        frame_output->format = codec_ctx->pix_fmt;
        frame_output->width = width;
        frame_output->height = height;
        av_frame_get_buffer(frame_output, 0);

        sws_ctx = sws_getContext(
            width, height, AV_PIX_FMT_RGBAF16LE,
            width, height, codec_ctx->pix_fmt,
            SWS_LANCZOS | SWS_ACCURATE_RND, nullptr, nullptr, nullptr
        );

        UtilityFunctions::print("Video Renderer started successfully!");

        return true;
    }

    bool VideoRenderer::open_output_file() {
        if (!(format_ctx->oformat->flags & AVFMT_NOFILE)) {
            if (avio_open(&format_ctx->pb, format_ctx->url, AVIO_FLAG_WRITE) < 0) {
                UtilityFunctions::printerr("Could not open file for writing.");
                return false;
            }
        }
        if (avformat_write_header(format_ctx, nullptr) < 0) {
            UtilityFunctions::printerr("Error occurred when writing header.");
            return false;
        }
        return true;
    }

    bool VideoRenderer::send_frame(const Ref<Image> &image) {

        if (image.is_null()) {
            UtilityFunctions::printerr("Cannot send frame: Image is null!");
            return false;
        }

        if (image->get_format() != Image::Format::FORMAT_RGBAH) {
            UtilityFunctions::printerr("Invalid format! Expected FORMAT_RGBAH.");
            return false;
        }

        PackedByteArray data = image->get_data();

        int expected_size = width * height * 4 * 2;
        if (data.size() != expected_size) {
            UtilityFunctions::printerr("Frame size mismatch! Expected: ", expected_size, " got: ", data.size());
            return false;
        }

        const uint8_t *src_data = data.ptr();
        av_image_fill_arrays(frame_input->data, frame_input->linesize, src_data, AV_PIX_FMT_RGBAF16LE, width, height, 1);

        sws_scale(sws_ctx, frame_input->data, frame_input->linesize, 0, height, frame_output->data, frame_output->linesize);

        frame_output->pts = frame_count++;

        int ret = avcodec_send_frame(codec_ctx, frame_output);
        if (ret < 0) {
            UtilityFunctions::printerr("Error sending frame to encoder.");
            return false;
        }

        AVPacket pkt;
        av_init_packet(&pkt);

        while (ret >= 0) {
            ret = avcodec_receive_packet(codec_ctx, &pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                UtilityFunctions::printerr("Error during encoding.");
                return false;
            }

            av_packet_rescale_ts(&pkt, codec_ctx->time_base, video_stream->time_base);
            pkt.stream_index = video_stream->index;

            av_interleaved_write_frame(format_ctx, &pkt);
            av_packet_unref(&pkt);
        }

        return true;
    }


    bool VideoRenderer::finish() {
        if (!format_ctx) return false;

        avcodec_send_frame(codec_ctx, nullptr);
        
        AVPacket *pkt = av_packet_alloc();
        while (avcodec_receive_packet(codec_ctx, pkt) >= 0) {
            av_packet_rescale_ts(pkt, codec_ctx->time_base, video_stream->time_base);
            pkt->stream_index = video_stream->index;
            av_interleaved_write_frame(format_ctx, pkt);
            av_packet_unref(pkt);
        }
        av_packet_free(&pkt);

        if (sws_ctx) sws_freeContext(sws_ctx);
        if (frame_input) av_frame_free(&frame_input);
        if (frame_output) av_frame_free(&frame_output);
        if (codec_ctx) avcodec_free_context(&codec_ctx);

        codec_ctx = nullptr;
        video_stream = nullptr;
        frame_input = nullptr;
        frame_output = nullptr;
        sws_ctx = nullptr;
        frame_count = 0;

        UtilityFunctions::print("Video rendering finished and saved successfully!");
        return true;
    }

    bool VideoRenderer::close_output_file() {

        if (!format_ctx) return false;
        
        av_write_trailer(format_ctx);

        if (format_ctx && !(format_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&format_ctx->pb);
        }
        avformat_free_context(format_ctx);
        
        format_ctx = nullptr;
        
        return true;
    }
}



