#!/bin/bash
set -euo pipefail; mkdir -p out
sudo apt-get update -qq
sudo apt-get install -y -qq build-essential bc flex bison libssl-dev libelf-dev xz-utils curl python3 \
  qemu-system-x86 qemu-system-arm gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu libc6-dev-arm64-cross
./01_KERNEL/linux/build_linux.sh arm64
cp build/talco-linux-arm64.img out/
./18_QUALITY_ASSURANCE/integration_tests/boot_linux_qemu.py arm64
