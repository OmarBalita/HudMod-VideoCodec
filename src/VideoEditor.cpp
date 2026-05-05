#include "VideoEditor.hpp"

namespace godot {

    void VideoEditor::_bind_methods() {
        ClassDB::bind_static_method(godot::StringName("VideoEditor"), D_METHOD("resize_video_file", "input_path", "output_path", "target_width", "target_height"), &VideoEditor::resize_video_file);
    }

    VideoEditor::VideoEditor() {}
    VideoEditor::~VideoEditor() {}

    bool VideoEditor::resize_video_file(const String &input_path, const String &output_path, int target_width, int target_height) {
        const char* input_path_c = input_path.utf8().get_data();
        const char* output_path_c = output_path.utf8().get_data();

        AVFormatContext* ifmt_ctx = NULL;
        AVFormatContext* ofmt_ctx = NULL;
        AVCodecContext *dec_ctx = NULL, *enc_ctx = NULL;
        struct SwsContext* sws_ctx = NULL;
        AVFrame *frame = NULL, *scaled_frame = NULL;
        int ret;

        int video_stream_idx = -1;

        if (avformat_open_input(&ifmt_ctx, input_path_c, NULL, NULL) < 0) return false;
        if (avformat_find_stream_info(ifmt_ctx, NULL) < 0) {goto cleanup;}

        for (unsigned int i = 0; i < ifmt_ctx->nb_streams; i++) 
            if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) video_stream_idx = i;
        
        if (video_stream_idx == -1) {goto cleanup;}

        {
            const AVCodec* decoder = avcodec_find_decoder(ifmt_ctx->streams[video_stream_idx]->codecpar->codec_id);
            dec_ctx = avcodec_alloc_context3(decoder);
            avcodec_parameters_to_context(dec_ctx, ifmt_ctx->streams[video_stream_idx]->codecpar);
            if (avcodec_open2(dec_ctx, decoder, NULL) < 0) {goto cleanup;}
        }

        avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, output_path_c);
        if (!ofmt_ctx) {goto cleanup;}

        {
            const AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
            AVStream* out_stream = avformat_new_stream(ofmt_ctx, NULL);
            enc_ctx = avcodec_alloc_context3(encoder);
            
            enc_ctx->height = target_height;
            enc_ctx->width = target_width;
            enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
            enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

            enc_ctx->time_base = av_inv_q(dec_ctx->framerate); 
            out_stream->time_base = enc_ctx->time_base;
            enc_ctx->bit_rate = 2000000;

            if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            if (avcodec_open2(enc_ctx, encoder, NULL) < 0) {goto cleanup;}
            avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
        }

        if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            if (avio_open(&ofmt_ctx->pb, output_path_c, AVIO_FLAG_WRITE) < 0) {goto cleanup;}
        }
        if (avformat_write_header(ofmt_ctx, NULL) < 0) {goto cleanup;}

        sws_ctx = sws_getContext(
            dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
            target_width, target_height, enc_ctx->pix_fmt,
            SWS_BICUBIC, NULL, NULL, NULL
        );

        frame = av_frame_alloc();
        scaled_frame = av_frame_alloc();
        scaled_frame->format = enc_ctx->pix_fmt;
        scaled_frame->width = target_width;
        scaled_frame->height = target_height;
        av_frame_get_buffer(scaled_frame, 0);

        AVPacket pkt;
        while (av_read_frame(ifmt_ctx, &pkt) >= 0) {
            if (pkt.stream_index == video_stream_idx) {
                if (avcodec_send_packet(dec_ctx, &pkt) >= 0) {
                    while (avcodec_receive_frame(dec_ctx, frame) >= 0) {
                        sws_scale(sws_ctx, frame->data, frame->linesize, 0, dec_ctx->height, scaled_frame->data, scaled_frame->linesize);
                        
                        scaled_frame->pts = frame->best_effort_timestamp;
                        
                        if (avcodec_send_frame(enc_ctx, scaled_frame) >= 0) {
                            AVPacket out_pkt = {0};
                            while (avcodec_receive_packet(enc_ctx, &out_pkt) >= 0) {
                                av_packet_rescale_ts(&out_pkt, enc_ctx->time_base, ofmt_ctx->streams[0]->time_base);
                                out_pkt.stream_index = 0;
                                av_interleaved_write_frame(ofmt_ctx, &out_pkt);
                                av_packet_unref(&out_pkt);
                            }
                        }
                    }
                }
            }
            av_packet_unref(&pkt);
        }

        av_write_trailer(ofmt_ctx);

    cleanup:
        if (frame) av_frame_free(&frame);
        if (scaled_frame) av_frame_free(&scaled_frame);
        if (sws_ctx) sws_freeContext(sws_ctx);
        if (dec_ctx) avcodec_free_context(&dec_ctx);
        if (enc_ctx) avcodec_free_context(&enc_ctx);
        if (ifmt_ctx) avformat_close_input(&ifmt_ctx);
        if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) avio_closep(&ofmt_ctx->pb);
        if (ofmt_ctx) avformat_free_context(ofmt_ctx);

        return true;
    }
}
