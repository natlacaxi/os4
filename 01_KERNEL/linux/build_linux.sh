#!/bin/bash
# Telecharge le noyau Linux LTS 6.12, le compile avec le coeur TALCO comme /init, produit build/talco-linux-<arch>.img
# Usage : build_linux.sh x86_64|arm64
set -euo pipefail
ARCH="${1:-x86_64}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK="$ROOT/build/linux-$ARCH"
mkdir -p "$WORK" && cd "$WORK"

if [ "$ARCH" = "x86_64" ]; then
  CC=gcc; CROSS=""; KARCH=x86_64; TARGET=bzImage; IMG=arch/x86/boot/bzImage; EXTRA=kvm_guest.config
else
  CC=aarch64-linux-gnu-gcc; CROSS="aarch64-linux-gnu-"; KARCH=arm64; TARGET=Image; IMG=arch/arm64/boot/Image; EXTRA=""
fi

# 1. derniere version 6.12 LTS
VER=$(python3 - <<'PY'
import json, urllib.request
d = json.load(urllib.request.urlopen("https://www.kernel.org/releases.json"))
lts = [r["version"] for r in d["releases"] if r["moniker"] == "longterm"]
pick = [v for v in lts if v.startswith("6.12.")] or lts
print(pick[0])
PY
)
echo "== Noyau Linux $VER ($ARCH)"
[ -f "linux-$VER.tar.xz" ] || curl -fsSL -o "linux-$VER.tar.xz" "https://cdn.kernel.org/pub/linux/kernel/v${VER%%.*}.x/linux-$VER.tar.xz"
[ -d "linux-$VER" ] || tar -xf "linux-$VER.tar.xz"

# 2. userspace : le coeur TALCO (shell + fichiers) devient /init
K="$ROOT/talco-kernel"
$CC -O2 -static -Wall -Wextra -I"$K/include" -DTALCO_LINUX -o talco-init \
  "$K/core/kstring.c" "$K/core/console.c" "$K/core/heap.c" "$K/core/ramfs.c" "$K/core/shell.c" "$K/core/kmain.c" \
  "$K/arch/linux/hal_linux.c"
cat > initramfs.list <<LIST
dir /dev 0755 0 0
nod /dev/console 0600 0 0 c 5 1
nod /dev/null 0666 0 0 c 1 3
dir /proc 0755 0 0
dir /sys 0755 0 0
dir /tmp 0755 0 0
file /init $WORK/talco-init 0755 0 0
LIST

# 3. configuration puis compilation
cd "linux-$VER"
MK=(make ARCH=$KARCH ${CROSS:+CROSS_COMPILE=$CROSS})
"${MK[@]}" defconfig $EXTRA
ARCH=$KARCH scripts/kconfig/merge_config.sh -m .config "$ROOT/01_KERNEL/configuration/talco_linux.config"
scripts/config --set-str INITRAMFS_SOURCE "$WORK/initramfs.list"
"${MK[@]}" olddefconfig
"${MK[@]}" -j"$(nproc)" $TARGET
cp "$IMG" "$ROOT/build/talco-linux-$ARCH.img"
echo "== OK : build/talco-linux-$ARCH.img"
