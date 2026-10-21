#!/bin/bash
# (Public copy for LGPL compliance; app path replaced by ./out.) Builds media3-decoder-ffmpeg-1.11.1.aar: de officiële Media3 FFmpeg-decoder met
# audiodecoders (DTS, TrueHD, AC3/E-AC3, …) plus de AC-3-encoder (tools/ffmpeg/ac3/), als terugval voor formaten die toestel of tv niet aankunnen
# (gemeten 2026-10-19: DTS 5.1 speelde stil op Shield + LG GX, die geen DTS meer ondersteunt).
# FFmpeg zonder --enable-gpl: LGPL 2.1 (versie: FFMPEG_TAG hieronder). Vereist: Android NDK r30 en CMake (sdkmanager), JDK 21.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK=${1:-$HOME/re/media3-ffmpeg}
NDK=${NDK:-/opt/homebrew/share/android-commandlinetools/ndk/30.0.16248370}
SDK=${SDK:-/opt/homebrew/share/android-commandlinetools}
DECODERS=(dca truehd mlp ac3 eac3 flac alac opus vorbis mp3 aac pcm_mulaw pcm_alaw)
mkdir -p "$WORK" && cd "$WORK"
[ -d media ] || git clone --depth 1 --branch 1.11.1 https://github.com/androidx/media.git media
M="$WORK/media/libraries/decoder_ffmpeg/src/main"
cd "$M/jni"
# Vaste tag (reproduceerbaar; de release/6.0-tak schoof op). n9.0.2: NEON-AC-3-encoder, E-AC-3/TrueHD/DTS-fixes en
# tientallen beveiligingsfixes sinds 6.0 (onderzoek 21-10). Andere versie in de map: weg en opnieuw.
FFMPEG_TAG=n9.0.2
if [ -d ffmpeg ] && [ "$(git -C ffmpeg describe --tags --exact-match 2>/dev/null)" != "$FFMPEG_TAG" ]; then rm -r ffmpeg; fi
[ -d ffmpeg ] || git clone https://git.ffmpeg.org/ffmpeg.git --branch="$FFMPEG_TAG" --depth=1 ffmpeg
# libpostproc bestaat niet meer sinds FFmpeg 8; configure stopt op een onbekende optie.
sed -i '' '/^    --disable-postproc$/d' build_ffmpeg.sh
# iconv uit: NDK r30 heeft het vanaf API 28, de module linkt tegen API 23 (undefined symbol iconv_open).
grep -q -- "--disable-iconv" build_ffmpeg.sh || sed -i '' 's/^    --disable-everything$/    --disable-everything\n    --disable-iconv/' build_ffmpeg.sh
# AC-3-encoder (deelproject 8): AAC 5.1 → AC-3 voor de directe HDMI-uitgang. Eigen JNI + Java-klasse in de extensie.
grep -q -- "--enable-encoder=ac3" build_ffmpeg.sh || sed -i '' 's/^    --disable-iconv$/    --disable-iconv\n    --enable-encoder=ac3/' build_ffmpeg.sh
cp "$HERE/ac3/ac3_encoder_jni.cc" "$M/jni/"
cp "$HERE/ac3/FfmpegAc3Encoder.java" "$M/java/androidx/media3/decoder/ffmpeg/"
grep -q "ac3_encoder_jni.cc" CMakeLists.txt || sed -i '' 's/^            ffmpeg_jni.cc)$/            ffmpeg_jni.cc ac3_encoder_jni.cc)/' CMakeLists.txt
./build_ffmpeg.sh "$M" "$NDK" darwin-x86_64 28 "${DECODERS[@]}"
cd "$WORK/media"
echo "sdk.dir=$SDK" > local.properties
./gradlew -q :lib-decoder-ffmpeg:assembleRelease
mkdir -p "$HERE/out"
cp libraries/decoder_ffmpeg/buildout/outputs/aar/lib-decoder-ffmpeg-release.aar "$HERE/out/media3-decoder-ffmpeg-1.11.1.aar"
