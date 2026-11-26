#!/bin/bash
# Construit talco-launcher.apk sans Gradle : NDK (clang) + aapt2 + javac + d8 + zipalign + apksigner.
# Variables : ANDROID_HOME (SDK), ANDROID_NDK_HOME ou ANDROID_NDK_LATEST_HOME (NDK). Sortie : out/talco-launcher.apk
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SDK="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"; NDK="${ANDROID_NDK_HOME:-${ANDROID_NDK_LATEST_HOME:-}}"
[ -d "$SDK" ] || { echo "ERREUR : SDK Android introuvable (ANDROID_HOME)"; exit 1; }
[ -d "$NDK" ] || { echo "ERREUR : NDK introuvable (ANDROID_NDK_HOME)"; exit 1; }
BT="$SDK/build-tools/$(ls "$SDK/build-tools" | sort -V | tail -1)"
PLAT="$(ls -d "$SDK"/platforms/android-[0-9]* | sort -V | tail -1)"; JAR="$PLAT/android.jar"
echo "== SDK : build-tools $(basename "$BT"), plateforme $(basename "$PLAT"), NDK $(basename "$NDK")"
W="$ROOT/build/android"; rm -rf "$W"; mkdir -p "$W/stage/lib" "$W/classes" "$ROOT/out"
UI="$ROOT/05_TALCO_EXPERIENCE/launcher/ui"

# 1. bibliotheque native : arm64 (telephones, tablettes) + x86_64 (emulateur Android Studio sur PC)
for T in arm64-v8a:aarch64-linux-android24 x86_64:x86_64-linux-android24; do
  ABI="${T%%:*}"; TRIPLE="${T##*:}"
  mkdir -p "$W/stage/lib/$ABI"
  "$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/$TRIPLE-clang" -shared -fPIC -O2 -Wall -Wl,-z,max-page-size=16384 -I"$UI" \
    -o "$W/stage/lib/$ABI/libtalco.so" "$ROOT/android-launcher/native/talco_jni.c" "$UI/gfx.c" "$UI/launcher.c"
done
# 2. Java -> dex
javac --release 8 -Xlint:-options -cp "$JAR" -d "$W/classes" "$ROOT"/android-launcher/java/com/talco/os/*.java
"$BT/d8" --release --lib "$JAR" --min-api 24 --output "$W/stage" $(find "$W/classes" -name '*.class')
# 3. manifeste + archive
"$BT/aapt2" link -o "$W/base.apk" -I "$JAR" --manifest "$ROOT/android-launcher/AndroidManifest.xml" \
  --min-sdk-version 24 --target-sdk-version 34 --version-code 4 --version-name 0.4.0
(cd "$W/stage" && zip -q -r -0 "$W/base.apk" classes.dex lib)
"$BT/zipalign" -p -f 4 "$W/base.apk" "$W/aligned.apk"
# 4. signature (cle de debogage, pour installation manuelle)
keytool -genkeypair -keystore "$W/debug.keystore" -storepass android -keypass android -alias talco \
  -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=TALCO" >/dev/null 2>&1
"$BT/apksigner" sign --ks "$W/debug.keystore" --ks-pass pass:android --key-pass pass:android --out "$ROOT/out/talco-launcher.apk" "$W/aligned.apk"
"$BT/apksigner" verify "$ROOT/out/talco-launcher.apk" && echo "== OK : out/talco-launcher.apk"
