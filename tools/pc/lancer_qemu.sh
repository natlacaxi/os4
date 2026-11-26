#!/bin/bash
# Linux / macOS : lance TALCO-OS3 (Linux + ecran tactile virtuel) dans QEMU.
# Usage : ./lancer_qemu.sh [talco-linux-x86_64.img] [tablette|telephone]
IMG="${1:-./talco-linux-x86_64.img}"; FORME="${2:-tablette}"
[ "$FORME" = "telephone" ] && T="xres=720,yres=1560" || T="xres=1280,yres=800"
exec qemu-system-x86_64 -m 512 -cpu max -kernel "$IMG" -append "console=ttyS0 panic=-1" \
  -vga none -device "virtio-gpu-pci,edid=on,$T" -device virtio-tablet-pci -display "gtk,zoom-to-fit=on" -serial stdio
