# FFmpeg in Lucelle — corresponding source (LGPL 2.1)

Lucelle (Android TV app, `app.lucelle.tv`) ships `libffmpegJNI.so`, which contains **FFmpeg 9.0.2** statically
linked into the Media3 FFmpeg decoder extension. FFmpeg is licensed under the **GNU Lesser General Public License
version 2.1** (see `COPYING.LGPLv2.1`). This repository provides everything needed to rebuild that library and replace
it in the app.

## What is in libffmpegJNI.so

| Part | Version | Licence | Source |
| --- | --- | --- | --- |
| FFmpeg | 9.0.2, git tag `n9.0.2`, commit `946fcce07b6dcd0331c8cc609192aeff5e1924f8` | LGPL 2.1 (no `--enable-gpl`, no `--enable-nonfree`) | release asset `ffmpeg-9.0.2.tar.xz` (official tarball, SHA-256 `8c3850283eb25fa026482078a04051e0be17347b09ef81a0849bec15a96e002e`), also https://git.ffmpeg.org/ffmpeg.git |
| Media3 FFmpeg decoder (`ffmpeg_jni.cc`, Java glue) | androidx/media `1.11.1` | Apache 2.0 | https://github.com/androidx/media/tree/1.11.1/libraries/decoder_ffmpeg |
| AC-3 encoder JNI (`ac3/ac3_encoder_jni.cc`, `ac3/FfmpegAc3Encoder.java`) | this repository | LGPL 2.1-compatible (same terms as FFmpeg) | `ac3/` |

FFmpeg itself is **not modified**. Only its build is configured, via Media3's `build_ffmpeg.sh`, with these changes
(applied by `build.sh`):

- decoders enabled: `dca truehd mlp ac3 eac3 flac alac opus vorbis mp3 aac pcm_mulaw pcm_alaw`
- `--enable-encoder=ac3` (5.1 PCM → AC-3 for the HDMI bitstream output)
- `--disable-iconv` (NDK r30 links against API 23)
- `--disable-postproc` removed (libpostproc no longer exists since FFmpeg 8)

## Rebuild and replace

Requirements: Android NDK r30, CMake (Android SDK), JDK 21, macOS (for other hosts change the `darwin-x86_64` argument).

```sh
NDK=/path/to/ndk/30.x SDK=/path/to/android-sdk ./build.sh /tmp/media3-ffmpeg
# result: out/media3-decoder-ffmpeg-1.11.1.aar  → jni/<abi>/libffmpegJNI.so
```

To use your own build in Lucelle: unpack the APK, replace `lib/<abi>/libffmpegJNI.so` with your build, re-sign the
APK with your own key and install it. The JNI interface is not obfuscated.

## Contact

Questions about this source or requests for it: open an issue in this repository. This source is offered for at least
three years after the last distribution of a Lucelle version containing it.
