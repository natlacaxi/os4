#!/bin/bash
set -euo pipefail; mkdir -p out
sudo apt-get update -qq
sudo apt-get install -y -qq build-essential bc flex bison libssl-dev libelf-dev xz-utils curl python3 \
  python3-pil qemu-system-x86 qemu-system-arm gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu libc6-dev-arm64-cross
./01_KERNEL/linux/build_linux.sh arm64
cp build/talco-linux-arm64.img out/
python3 17_BUILD_SYSTEM/image_builder/test_mkbootimg.py
python3 17_BUILD_SYSTEM/image_builder/mkbootimg_min.py --kernel build/talco-linux-arm64.img --cmdline "console=ttyS0,115200 panic=-1" --output out/boot-generique-NE-PAS-FLASHER.img
# image au format du Galaxy A34 5G (boot v4 + vendor_boot v4), puis controle par l'outil officiel Android (informatif)
python3 17_BUILD_SYSTEM/image_builder/mkbootimg_min.py --header-version 4 --kernel build/talco-linux-arm64.img \
  --cmdline "console=ttyS0,115200 panic=-1 talco.dpi=420" --output out/boot-a34-v4-NE-PAS-FLASHER.img --vendor-boot out/vendor_boot-a34-v4-NE-PAS-FLASHER.img
if curl -fsSL "https://android.googlesource.com/platform/system/tools/mkbootimg/+/refs/heads/main/unpack_bootimg.py?format=TEXT" | base64 -d > /tmp/unpack_bootimg.py; then
  echo "== Controle par unpack_bootimg.py (outil officiel AOSP) :"
  python3 /tmp/unpack_bootimg.py --boot_img out/boot-a34-v4-NE-PAS-FLASHER.img --out /tmp/u_boot || echo "AVERTISSEMENT : boot v4 non reconnu par l'outil officiel"
  python3 /tmp/unpack_bootimg.py --boot_img out/vendor_boot-a34-v4-NE-PAS-FLASHER.img --out /tmp/u_vendor || echo "AVERTISSEMENT : vendor_boot v4 non reconnu par l'outil officiel"
fi
./18_QUALITY_ASSURANCE/integration_tests/boot_linux_qemu.py arm64 tablet
./18_QUALITY_ASSURANCE/integration_tests/boot_linux_qemu.py arm64 phone
./18_QUALITY_ASSURANCE/integration_tests/boot_linux_qemu.py arm64 a34
