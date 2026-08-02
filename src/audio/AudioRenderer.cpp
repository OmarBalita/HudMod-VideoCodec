#include "audio/AudioRenderer.h"

namespace godot {

    void AudioRenderer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_container_name"), &AudioRenderer::get_container_name);
        ClassDB::bind_method(D_METHOD("set_container_name", "new_container_name"), &AudioRenderer::set_container_name);
        ClassDB::bind_method(D_METHOD("get_encoder_name"), &AudioRenderer::get_encoder_name);
        ClassDB::bind_method(D_METHOD("set_encoder_name", "new_encoder_name"), &AudioRenderer::set_encoder_name);

        ClassDB::bind_method(D_METHOD("get_sample_rate"), &AudioRenderer::get_sample_rate);
        ClassDB::bind_method(D_METHOD("set_sample_rate", "new_sample_rate"), &AudioRenderer::set_sample_rate);
        ClassDB::bind_method(D_METHOD("get_channels"), &AudioRenderer::get_channels);
        ClassDB::bind_method(D_METHOD("set_channels", "new_channels"), &AudioRenderer::set_channels);

        ClassDB::bind_method(D_METHOD("prepare_format_ctx", "output_path"), &AudioRenderer::prepare_format_ctx);
        ClassDB::bind_method(D_METHOD("share_video_renderer_format_ctx", "video_renderer"), &AudioRenderer::share_video_renderer_format_ctx);
        ClassDB::bind_method(D_METHOD("start"), &AudioRenderer::start);
        ClassDB::bind_method(D_METHOD("push_samples", "samples"), &AudioRenderer::push_samples);
        ClassDB::bind_method(D_METHOD("open_output_file"), &AudioRenderer::open_output_file);
        ClassDB::bind_method(D_METHOD("finish"), &AudioRenderer::finish);
        ClassDB::bind_method(D_METHOD("close_output_file"), &AudioRenderer::close_output_file);
    }

    AudioRenderer::AudioRenderer() {}
    AudioRenderer::~AudioRenderer() {}

    void AudioRenderer::prepare_format_ctx(const String &output_path) {
        avformat_alloc_output_context2(&format_ctx, NULL, container_name.utf8().get_data(), output_path.utf8().get_data());
        if (!format_ctx) {
            UtilityFunctions::printerr("Could not allocate output format context");
            return;
        }
    }

     void AudioRenderer::share_video_renderer_format_ctx(Ref<VideoRenderer> video_renderer) {
        format_ctx = video_renderer->get_format_ctx();
    }


    bool AudioRenderer::start() {

        const AVCodec* codec = avcodec_find_encoder_by_name(encoder_name.utf8().get_data());
        if (codec == nullptr) {
            UtilityFunctions::print("Cannot find codec.");
            return false;
        }
        codec_ctx = avcodec_alloc_context3(codec);
        
        
        codec_ctx->sample_rate = sample_rate;
        av_channel_layout_default(&codec_ctx->ch_layout, channels);

        switch (codec->id)
        {
            case AV_CODEC_ID_OPUS:
                codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLT;
                break;
            case AV_CODEC_ID_MP3:
                codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
                break;
            default:
                codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
                break;
        }
        codec_ctx->time_base.num = 1;
        codec_ctx->time_base.den = sample_rate;

        if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
            UtilityFunctions::printerr("Could not open audio codec:", codec->long_name);
            return false;
        }

        int ret = swr_alloc_set_opts2(
            &swr_ctx,
            &codec_ctx->ch_layout,
            codec_ctx->sample_fmt,
            codec_ctx->sample_rate,
            &codec_ctx->ch_layout,
            AV_SAMPLE_FMT_FLT,
            codec_ctx->sample_rate,
            0, NULL
        );

        if (ret < 0 || swr_init(swr_ctx) < 0) {
            UtilityFunctions::printerr("Failed to initialize SwrContext");
            return false;
        }
        
        input_frame = av_frame_alloc(); // Allocate the input frame
        input_frame->nb_samples = codec_ctx->frame_size;
        input_frame->format = codec_ctx->sample_fmt;
        av_channel_layout_copy(&input_frame->ch_layout, &codec_ctx->ch_layout);
        av_frame_get_buffer(input_frame, 0); // Allocate the input frame buffer

        audio_fifo = av_audio_fifo_alloc(codec_ctx->sample_fmt, codec_ctx->ch_layout.nb_channels, 1);

        stream = avformat_new_stream(format_ctx, NULL);
        if (!stream) {
            UtilityFunctions::printerr("Could not create audio stream");
            return false;
        }

        if (avcodec_parameters_from_context(stream->codecpar, codec_ctx) < 0) {
            UtilityFunctions::printerr("Could not copy audio parameters to stream");
            return false;
        }

        stream_idx = stream->index;

        UtilityFunctions::printt("Audio Renderer started successfully with sample format as:", codec_ctx->sample_fmt == AV_SAMPLE_FMT_FLT ? "SAMPLE_FORMAT_FLOAT_INTERLEAVED" : "SAMPLE_FORMAT_FLOAT_PLANAR");

        return true;
    }

    bool AudioRenderer::open_output_file() {

        if (!(format_ctx->oformat->flags & AVFMT_NOFILE)) {
            if (avio_open(&format_ctx->pb, format_ctx->url, AVIO_FLAG_WRITE) < 0) {
                UtilityFunctions::printerr("Could not open output file: ", String(format_ctx->url));
                return false;
            }
        }
        if (avformat_write_header(format_ctx, NULL) < 0) {
            UtilityFunctions::printerr("Error occurred when writing header to output file");
            return false;
        }
        return true;
    }

    void AudioRenderer::push_samples(PackedByteArray samples) {
        if (samples.is_empty()) return;

        int channels = codec_ctx->ch_layout.nb_channels;
        int samples_num = samples.size() / sizeof(float) / channels;
        const uint8_t* src_ptr = samples.ptr();

        if (codec_ctx->sample_fmt == AV_SAMPLE_FMT_FLTP) {
            uint8_t** dst_data = nullptr;
            av_samples_alloc_array_and_samples(&dst_data, nullptr, channels, samples_num, AV_SAMPLE_FMT_FLTP, 0);

            const uint8_t* src_data[1] = { src_ptr };
            swr_convert(swr_ctx, dst_data, samples_num, src_data, samples_num);

            av_audio_fifo_write(audio_fifo, (void**)dst_data, samples_num);

            if (dst_data) {
                av_freep(&dst_data[0]);
                av_freep(&dst_data);
            }
        } else {
            const uint8_t* src_data[1] = { src_ptr };
            av_audio_fifo_write(audio_fifo, (void**)src_data, samples_num);
        }

        _process_fifo(input_frame); 
    }


    void AudioRenderer::_process_fifo(AVFrame* frame) {
        while (av_audio_fifo_size(audio_fifo) >= codec_ctx->frame_size) {
            
            av_frame_make_writable(frame);
            frame->nb_samples = codec_ctx->frame_size;

            av_audio_fifo_read(audio_fifo, (void**)frame->data, codec_ctx->frame_size);

            frame->pts = curr_pts;
            curr_pts += frame->nb_samples;

            _encode_and_write(frame);
        }
    }


    void AudioRenderer::_encode_and_write(AVFrame* frame) {
        int ret;

        ret = avcodec_send_frame(codec_ctx, frame);
        if (ret < 0) {
            return;
        }

        while (ret >= 0) {
            AVPacket* pkt = av_packet_alloc();
            ret = avcodec_receive_packet(codec_ctx, pkt);

            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                av_packet_free(&pkt);
                break;
            } else if (ret < 0) {
                av_packet_free(&pkt);
                return;
            }

            pkt->stream_index = stream_idx;
            pkt->pts = av_rescale_q(pkt->pts, codec_ctx->time_base, stream->time_base);
            pkt->dts = av_rescale_q(pkt->dts, codec_ctx->time_base, stream->time_base);
            pkt->duration = av_rescale_q(pkt->duration, codec_ctx->time_base, stream->time_base);

            av_interleaved_write_frame(format_ctx, pkt);

            av_packet_unref(pkt);
            av_packet_free(&pkt);
        }
    }


    bool AudioRenderer::finish() {
        int remaining_samples = av_audio_fifo_size(audio_fifo);

        if (remaining_samples > 0) {
            av_frame_make_writable(input_frame);
            av_samples_set_silence(input_frame->data, 0, codec_ctx->frame_size, codec_ctx->ch_layout.nb_channels, codec_ctx->sample_fmt);
            av_audio_fifo_read(audio_fifo, (void**)input_frame->data, remaining_samples);
            input_frame->pts = curr_pts;
            curr_pts += codec_ctx->frame_size;
            _encode_and_write(input_frame);
        }

        _encode_and_write(NULL);

        if (input_frame) av_frame_free(&input_frame);
        if (audio_fifo) av_audio_fifo_free(audio_fifo);
        if (swr_ctx) swr_free(&swr_ctx);
        if (codec_ctx) avcodec_free_context(&codec_ctx);

        UtilityFunctions::print("Audio rendering finished properly!");
        return true;
    }

    bool AudioRenderer::close_output_file() {

        if (!format_ctx) {return false;}
        
        av_write_trailer(format_ctx);

        if (format_ctx && !(format_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&format_ctx->pb);
        }
        avformat_free_context(format_ctx);
        
        format_ctx = nullptr;
        
        return true;
    }

}
