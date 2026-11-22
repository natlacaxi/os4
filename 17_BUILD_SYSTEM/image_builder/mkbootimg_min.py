#!/usr/bin/env python3
"""Fabrique des images Android boot.img (en-tetes v2 ou v4) et vendor_boot v4.
v2 : mkbootimg_min.py --kernel Image [--ramdisk r.gz] [--dtb x.dtb] --cmdline "..." --output boot.img
v4 : mkbootimg_min.py --header-version 4 --kernel Image [--ramdisk r.gz] --output boot.img \
         --vendor-boot vendor_boot.img [--vendor-ramdisk v.gz] [--dtb x.dtb] [--vendor-cmdline "..."]
ATTENTION : les formats v4 sont ecrits d'apres la specification Android, sans signature AVB. A comparer avec l'outil officiel
(unpack_bootimg.py d'AOSP, lance en CI) avant tout flash. Un appareil Samsung refusera une image non signee si le bootloader est verrouille."""
import argparse, hashlib, struct
ap = argparse.ArgumentParser()
ap.add_argument("--kernel", required=True); ap.add_argument("--ramdisk"); ap.add_argument("--dtb")
ap.add_argument("--cmdline", default=""); ap.add_argument("--base", default="0x10000000")
ap.add_argument("--pagesize", type=int, default=2048); ap.add_argument("--header-version", type=int, default=2, choices=[2, 4])
ap.add_argument("--vendor-boot"); ap.add_argument("--vendor-ramdisk"); ap.add_argument("--vendor-cmdline", default="")
ap.add_argument("--output", required=True)
a = ap.parse_args()
rd = lambda p: open(p, "rb").read() if p else b""
kernel, ramdisk, dtb = rd(a.kernel), rd(a.ramdisk), rd(a.dtb)
base = int(a.base, 0)
def pad(b, ps): return b + b"\0" * (-len(b) % ps)

if a.header_version == 2:
    ps = a.pagesize; cmd = a.cmdline.encode(); assert len(cmd) < 1536, "cmdline trop longue"
    sha = hashlib.sha1(kernel + ramdisk + dtb).digest().ljust(32, b"\0")
    hdr = struct.pack("<8s10I16s512s32s1024sIQIIQ", b"ANDROID!",
        len(kernel), base + 0x8000, len(ramdisk), base + 0x1000000, 0, base + 0xF00000, base + 0x100, ps,
        2, 0, b"TALCO-OS3", cmd[:512], sha, cmd[512:], 0, 0, 1660, len(dtb), base + 0x1F00000)
    assert len(hdr) == 1660
    with open(a.output, "wb") as f:
        f.write(pad(hdr, ps)); f.write(pad(kernel, ps)); f.write(pad(ramdisk, ps)); f.write(pad(dtb, ps))
    print(f"{a.output} : boot v2, noyau {len(kernel)} o, ramdisk {len(ramdisk)} o, dtb {len(dtb)} o")
else:
    ps = 4096; cmd = a.cmdline.encode(); assert len(cmd) <= 1536, "cmdline trop longue"
    hdr = struct.pack("<8s4I4II1536sI", b"ANDROID!", len(kernel), len(ramdisk), 0, 1584, 0, 0, 0, 0, 4, cmd, 0)
    assert len(hdr) == 1584
    with open(a.output, "wb") as f:
        f.write(pad(hdr, ps)); f.write(pad(kernel, ps)); f.write(pad(ramdisk, ps))
    print(f"{a.output} : boot v4, noyau {len(kernel)} o, ramdisk generique {len(ramdisk)} o")
    if a.vendor_boot:
        vr = rd(a.vendor_ramdisk); vcmd = a.vendor_cmdline.encode(); assert len(vcmd) <= 2048
        table = struct.pack("<III32s16I", len(vr), 0, 1, b"talco_platform", *([0] * 16)) if vr else b""
        assert not vr or len(table) == 108
        vh = struct.pack("<8sIIIII2048sI16sIIQIIII", b"VNDRBOOT", 4, ps, base + 0x8000, base + 0x1000000, len(vr),
            vcmd, base + 0x100, b"TALCO", 2128, len(dtb), base + 0x1F00000, len(table), 1 if vr else 0, 108, 0)
        assert len(vh) == 2128
        with open(a.vendor_boot, "wb") as f:
            f.write(pad(vh, ps)); f.write(pad(vr, ps)); f.write(pad(dtb, ps)); f.write(pad(table, ps))
        print(f"{a.vendor_boot} : vendor_boot v4, ramdisk vendor {len(vr)} o, dtb {len(dtb)} o")
