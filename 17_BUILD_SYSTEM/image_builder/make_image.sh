#!/bin/sh
# Construit une image rootfs de demonstration : build/talco-os3.tar.gz
set -e
B=build/rootfs
rm -rf "$B" && mkdir -p "$B"/bin "$B"/etc "$B"/usr/share
cp build/talco-os3 "$B/bin/talco-init"
cp VERSION "$B/etc/talco-release"
cp 16_DEVICE_SUPPORT/*/*/device.yaml "$B/usr/share/" 2>/dev/null || true
tar -czf build/talco-os3.tar.gz -C "$B" .
echo "Image creee : build/talco-os3.tar.gz"
