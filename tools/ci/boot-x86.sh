#!/bin/bash
set -euo pipefail; mkdir -p out
sudo apt-get update -qq && sudo apt-get install -y -qq qemu-system-x86 gcc-multilib
make os; cp talco-kernel/talco-kernel-x86.elf out/
./18_QUALITY_ASSURANCE/integration_tests/boot_qemu.sh x86
