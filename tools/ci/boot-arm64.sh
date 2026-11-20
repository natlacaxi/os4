#!/bin/bash
set -euo pipefail; mkdir -p out
sudo apt-get update -qq && sudo apt-get install -y -qq qemu-system-arm gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu
make os-arm64; cp talco-kernel/talco-kernel-aarch64.elf out/
./18_QUALITY_ASSURANCE/integration_tests/boot_qemu.sh aarch64
