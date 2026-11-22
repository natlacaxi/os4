#!/usr/bin/env python3
"""Fabrique une image Android boot.img (en-tete version 2). Minimal : noyau + ramdisk + dtb.
Usage : mkbootimg_min.py --kernel Image [--ramdisk r.cpio.gz] [--dtb x.dtb] [--cmdline "..."] [--base 0x10000000] --output boot.img
Limites : en-tetes v0-v2 seulement. Les Samsung recents (Android 11+) utilisent v3/v4 + vendor_boot : a ajouter par modele."""
import argparse, hashlib, struct
ap = argparse.ArgumentParser()
ap.add_argument("--kernel", required=True); ap.add_argument("--ramdisk"); ap.add_argument("--dtb")
ap.add_argument("--cmdline", default=""); ap.add_argument("--base", default="0x10000000")
ap.add_argument("--pagesize", type=int, default=2048); ap.add_argument("--output", required=True)
a = ap.parse_args()
rd = lambda p: open(p, "rb").read() if p else b""
kernel, ramdisk, dtb = rd(a.kernel), rd(a.ramdisk), rd(a.dtb)
base = int(a.base, 0); ps = a.pagesize
pad = lambda b: b + b"\0" * (-len(b) % ps)
cmd = a.cmdline.encode()
assert len(cmd) < 1536, "cmdline trop longue"
sha = hashlib.sha1(kernel + ramdisk + dtb).digest().ljust(32, b"\0")
hdr = struct.pack("<8s10I16s512s32s1024sIQIIQ", b"ANDROID!",
    len(kernel), base + 0x8000, len(ramdisk), base + 0x1000000, 0, base + 0xF00000, base + 0x100, ps,
    2, 0, b"TALCO-OS3", cmd[:512], sha, cmd[512:], 0, 0, 1660, len(dtb), base + 0x1F00000)
assert len(hdr) == 1660
with open(a.output, "wb") as f:
    f.write(pad(hdr)); f.write(pad(kernel)); f.write(pad(ramdisk)); f.write(pad(dtb))
print(f"{a.output} : noyau {len(kernel)} o, ramdisk {len(ramdisk)} o, dtb {len(dtb)} o")
