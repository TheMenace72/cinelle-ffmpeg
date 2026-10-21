/*
 * AC-3-encoder (FFmpeg, LGPL 2.1) voor iptv-player: PCM 16-bit interleaved → AC-3-frames van 1536 samples.
 * Wordt bij het bouwen in de Media3 FFmpeg-extensie gezet (tools/ffmpeg/build.sh).
 */
package androidx.media3.decoder.ffmpeg;

import java.nio.ByteBuffer;

public final class FfmpegAc3Encoder {
  private long handle;
  private final int frameSamples;

  /** Gooit IllegalStateException als FFmpeg of de encoder ontbreekt, of de kanalen niet kunnen (2 of 6). */
  public FfmpegAc3Encoder(int channels, int sampleRate, int bitRate) {
    if (!FfmpegLibrary.isAvailable()) throw new IllegalStateException("FFmpeg niet beschikbaar");
    handle = ffmpegAc3Init(channels, sampleRate, bitRate);
    if (handle == 0) throw new IllegalStateException("AC-3-encoder kon niet starten");
    frameSamples = ffmpegAc3FrameSamples(handle);
  }

  /** Samples per kanaal per AC-3-frame (1536). */
  public int frameSamples() {
    return frameSamples;
  }

  /** [pcm] en [out]: direct buffers; pcm bevat precies frameSamples × kanalen samples. Bytes in out, of < 0 bij fout. */
  public int encode(ByteBuffer pcm, ByteBuffer out) {
    return ffmpegAc3Encode(handle, pcm, out, out.capacity());
  }

  public void release() {
    if (handle != 0) {
      ffmpegAc3Release(handle);
      handle = 0;
    }
  }

  private native long ffmpegAc3Init(int channels, int sampleRate, int bitRate);
  private native int ffmpegAc3FrameSamples(long handle);
  private native int ffmpegAc3Encode(long handle, ByteBuffer pcm, ByteBuffer out, int outCapacity);
  private native void ffmpegAc3Release(long handle);
}
