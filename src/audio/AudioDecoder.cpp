// Written by Claude AI.
#include "audio/AudioDecoder.hpp"

using namespace godot;

void AudioDecoder::_bind_methods() {
    ClassDB::bind_static_method(
        godot::StringName("AudioDecoder"),
        D_METHOD("create_data_from_path", "file_path"),
        &AudioDecoder::create_data_from_path
    );
}

AudioDecoder::AudioDecoder() {}
AudioDecoder::~AudioDecoder() {}

TypedArray<PackedByteArray> AudioDecoder::create_data_from_path(const String &file_path) {
    TypedArray<PackedByteArray> result;
    
    const int             TARGET_RATE      = 48000;
    const AVSampleFormat  TARGET_FMT       = AV_SAMPLE_FMT_FLT;
    const AVChannelLayout TARGET_LAYOUT    = AV_CHANNEL_LAYOUT_STEREO;
    const int             TARGET_CHANNELS  = 2;
    const int             BYTES_PER_SAMPLE = av_get_bytes_per_sample(TARGET_FMT);
    const int             FRAME_BYTES      = TARGET_CHANNELS * BYTES_PER_SAMPLE;
    const int             BYTE_RATE        = TARGET_RATE * FRAME_BYTES;
    const int             BITS_PER_SAMPLE  = BYTES_PER_SAMPLE * 8;

    AVFormatContext *fmt_ctx = avformat_alloc_context();
    if (avformat_open_input(&fmt_ctx, file_path.utf8().get_data(), nullptr, nullptr) < 0) {
        UtilityFunctions::printerr("AudioDecoder: cannot open file: ", file_path);
        avformat_free_context(fmt_ctx);
        return result;
    }
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        UtilityFunctions::printerr("AudioDecoder: cannot read stream info");
        avformat_close_input(&fmt_ctx);
        return result;
    }

    struct TrackContext {
        AVCodecContext       *dec        = nullptr;
        SwrContext           *swr        = nullptr;
        int                   stream_idx = -1;
        std::vector<uint8_t>  pcm;       // تجميع هنا أثناء الـ decoding
    };

    std::vector<TrackContext> tracks;
    tracks.reserve(4);

    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream          *stream = fmt_ctx->streams[i];
        AVCodecParameters *params = stream->codecpar;
        if (params->codec_type != AVMEDIA_TYPE_AUDIO) continue;

        const AVCodec *codec = avcodec_find_decoder(params->codec_id);
        if (!codec) {
            UtilityFunctions::printerr("AudioDecoder: no decoder for stream ", (int)i);
            continue;
        }

        AVCodecContext *dec = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(dec, params);
        dec->thread_count      = 0;
        dec->request_sample_fmt = TARGET_FMT;

        if (avcodec_open2(dec, codec, nullptr) < 0) {
            UtilityFunctions::printerr("AudioDecoder: avcodec_open2 failed, stream ", (int)i);
            avcodec_free_context(&dec);
            continue;
        }

        if (dec->ch_layout.nb_channels <= 0) {
            int nb_ch = params->ch_layout.nb_channels > 0 ? params->ch_layout.nb_channels : 2;
            av_channel_layout_default(&dec->ch_layout, nb_ch);
        }

        SwrContext *swr = nullptr;
        int ret = swr_alloc_set_opts2(
            &swr,
            &TARGET_LAYOUT, TARGET_FMT,  TARGET_RATE,
            &dec->ch_layout, dec->sample_fmt, dec->sample_rate,
            0, nullptr
        );
        if (ret < 0 || swr_init(swr) < 0) {
            UtilityFunctions::printerr("AudioDecoder: swr init failed, stream ", (int)i);
            swr_free(&swr);
            avcodec_free_context(&dec);
            continue;
        }

        // تقدير الحجم مسبقًا — نسخة واحدة فقط لكل الـ PCM
        std::vector<uint8_t> pcm;
        if (stream->duration > 0 && stream->time_base.den > 0) {
            const double duration_sec = (double)stream->duration * av_q2d(stream->time_base);
            pcm.reserve((size_t)(duration_sec * TARGET_RATE * FRAME_BYTES) + 4096);
        }

        tracks.push_back({ dec, swr, (int)i, std::move(pcm) });
    }

    if (tracks.empty()) {
        UtilityFunctions::printerr("AudioDecoder: no valid audio streams found");
        avformat_close_input(&fmt_ctx);
        return result;
    }

    AVPacket *packet = av_packet_alloc();
    AVFrame  *frame  = av_frame_alloc();
    AVFrame  *conv   = av_frame_alloc();
    const int nb_tracks = (int)tracks.size();

    while (av_read_frame(fmt_ctx, packet) >= 0) {
        const int pkt_stream = packet->stream_index;

        for (int t = 0; t < nb_tracks; t++) {
            TrackContext &track = tracks[t];
            if (pkt_stream != track.stream_idx) continue;

            if (avcodec_send_packet(track.dec, packet) < 0) break;

            while (true) {
                int ret = avcodec_receive_frame(track.dec, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                if (ret < 0) break;
                if (!frame->data[0] || frame->nb_samples <= 0) {
                    av_frame_unref(frame);
                    continue;
                }

                conv->format      = TARGET_FMT;
                conv->ch_layout   = TARGET_LAYOUT;
                conv->sample_rate = TARGET_RATE;
                conv->nb_samples  = swr_get_out_samples(track.swr, frame->nb_samples);

                if (av_frame_get_buffer(conv, 0) < 0) { av_frame_unref(frame); break; }

                if (swr_convert_frame(track.swr, conv, frame) >= 0) {
                    const size_t byte_size = (size_t)conv->nb_samples * FRAME_BYTES;
                    const uint8_t *src = conv->extended_data[0];
                    track.pcm.insert(track.pcm.end(), src, src + byte_size);
                }

                av_frame_unref(frame);
                av_frame_unref(conv);
            }
            break;
        }
        av_packet_unref(packet);
    }

    // Flush SWR لكل مسار
    for (int t = 0; t < nb_tracks; t++) {
        TrackContext &track = tracks[t];
        while (true) {
            conv->format      = TARGET_FMT;
            conv->ch_layout   = TARGET_LAYOUT;
            conv->sample_rate = TARGET_RATE;
            conv->nb_samples  = 4096;
            if (av_frame_get_buffer(conv, 0) < 0) break;
            int flushed = swr_convert_frame(track.swr, conv, nullptr);
            if (flushed <= 0) { av_frame_unref(conv); break; }
            const size_t byte_size = (size_t)conv->nb_samples * FRAME_BYTES;
            const uint8_t *src = conv->extended_data[0];
            track.pcm.insert(track.pcm.end(), src, src + byte_size);
            av_frame_unref(conv);
        }
        avcodec_flush_buffers(track.dec);
    }

    av_frame_free(&conv);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avformat_close_input(&fmt_ctx);

    // إرجاع PCM خام مباشرة بدون WAV header
    result.resize(nb_tracks);

    for (int t = 0; t < nb_tracks; t++) {
        TrackContext &track = tracks[t];

        PackedByteArray pcm_out;
        pcm_out.resize((int64_t)track.pcm.size());
        memcpy(pcm_out.ptrw(), track.pcm.data(), track.pcm.size());

        result[t] = pcm_out;

        swr_free(&track.swr);
        avcodec_free_context(&track.dec);
    }

    return result;
}
