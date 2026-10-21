// AC-3-encoder voor de app (LGPL, FFmpeg "ac3"): PCM 16-bit interleaved → één AC-3-frame per 1536 samples.
// De Shield mengt meerkanaals-PCM naar stereo; AC-3 gaat via de directe HDMI-uitgang (passthrough) naar de soundbar.
#include <jni.h>
#include <android/log.h>
#include <cstring>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

#define AC3_TAG "ffmpeg_ac3"
#define AC3_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, AC3_TAG, __VA_ARGS__)

namespace {
struct Ac3Encoder {
  AVCodecContext* ctx = nullptr;
  AVFrame* frame = nullptr;
  AVPacket* pkt = nullptr;
  int channels = 0;
  int64_t pts = 0;
};

void freeEncoder(Ac3Encoder* e) {
  if (e == nullptr) return;
  if (e->pkt) av_packet_free(&e->pkt);
  if (e->frame) av_frame_free(&e->frame);
  if (e->ctx) avcodec_free_context(&e->ctx);
  delete e;
}
}  // namespace

extern "C" JNIEXPORT jlong JNICALL
Java_androidx_media3_decoder_ffmpeg_FfmpegAc3Encoder_ffmpegAc3Init(JNIEnv*, jobject, jint channels, jint sampleRate, jint bitRate) {
  const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AC3);
  if (codec == nullptr) {
    AC3_LOGE("geen ac3-encoder in deze build");
    return 0;
  }
  auto* e = new Ac3Encoder();
  e->channels = channels;
  e->ctx = avcodec_alloc_context3(codec);
  e->ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
  e->ctx->sample_rate = sampleRate;
  e->ctx->bit_rate = bitRate;
  // Android 5.1 = FL FR FC LFE BL BR = FFmpeg "5.1(back)".
  uint64_t mask = channels == 6 ? AV_CH_LAYOUT_5POINT1_BACK : channels == 2 ? AV_CH_LAYOUT_STEREO : 0;
  if (mask == 0 || av_channel_layout_from_mask(&e->ctx->ch_layout, mask) < 0) {
    AC3_LOGE("kanalen niet ondersteund: %d", channels);
    freeEncoder(e);
    return 0;
  }
  int r = avcodec_open2(e->ctx, codec, nullptr);
  if (r < 0) {
    AC3_LOGE("avcodec_open2: %d", r);
    freeEncoder(e);
    return 0;
  }
  e->frame = av_frame_alloc();
  e->frame->nb_samples = e->ctx->frame_size;
  e->frame->format = e->ctx->sample_fmt;
  e->frame->sample_rate = sampleRate;
  av_channel_layout_copy(&e->frame->ch_layout, &e->ctx->ch_layout);
  if (av_frame_get_buffer(e->frame, 0) < 0) {
    freeEncoder(e);
    return 0;
  }
  e->pkt = av_packet_alloc();
  return reinterpret_cast<jlong>(e);
}

extern "C" JNIEXPORT jint JNICALL
Java_androidx_media3_decoder_ffmpeg_FfmpegAc3Encoder_ffmpegAc3FrameSamples(JNIEnv*, jobject, jlong handle) {
  auto* e = reinterpret_cast<Ac3Encoder*>(handle);
  return e == nullptr ? 0 : e->ctx->frame_size;
}

// pcm: direct ByteBuffer met precies frame_size × kanalen samples (16-bit, interleaved). Geeft het aantal bytes in
// out terug, 0 als de encoder nog niets had, of < 0 bij een fout.
extern "C" JNIEXPORT jint JNICALL
Java_androidx_media3_decoder_ffmpeg_FfmpegAc3Encoder_ffmpegAc3Encode(JNIEnv* env, jobject, jlong handle, jobject pcm, jobject out, jint outCapacity) {
  auto* e = reinterpret_cast<Ac3Encoder*>(handle);
  if (e == nullptr) return -1;
  auto* in = static_cast<const int16_t*>(env->GetDirectBufferAddress(pcm));
  auto* dst = static_cast<uint8_t*>(env->GetDirectBufferAddress(out));
  if (in == nullptr || dst == nullptr) return -2;
  if (av_frame_make_writable(e->frame) < 0) return -3;
  const int n = e->frame->nb_samples;
  for (int c = 0; c < e->channels; c++) {
    auto* plane = reinterpret_cast<float*>(e->frame->data[c]);
    for (int i = 0; i < n; i++) plane[i] = in[i * e->channels + c] / 32768.0f;
  }
  e->frame->pts = e->pts;
  e->pts += n;
  int r = avcodec_send_frame(e->ctx, e->frame);
  if (r < 0) return -4;
  r = avcodec_receive_packet(e->ctx, e->pkt);
  if (r == AVERROR(EAGAIN)) return 0;
  if (r < 0) return -5;
  int size = e->pkt->size;
  if (size > outCapacity) {
    av_packet_unref(e->pkt);
    return -6;
  }
  memcpy(dst, e->pkt->data, size);
  av_packet_unref(e->pkt);
  return size;
}

extern "C" JNIEXPORT void JNICALL
Java_androidx_media3_decoder_ffmpeg_FfmpegAc3Encoder_ffmpegAc3Release(JNIEnv*, jobject, jlong handle) {
  freeEncoder(reinterpret_cast<Ac3Encoder*>(handle));
}
