#!/usr/bin/env python3
"""Verifie que mkbootimg_min produit des images v2 et v4 coherentes (relecture des en-tetes)."""
import os, struct, subprocess, sys, tempfile
here = os.path.dirname(os.path.abspath(__file__)); d = tempfile.mkdtemp(); bad = 0
def mk(*args): subprocess.check_call([sys.executable, f"{here}/mkbootimg_min.py", *args], stdout=subprocess.DEVNULL)
def check(name, cond):
    global bad; print(("OK   : " if cond else "FAIL : ") + name); bad += not cond
open(f"{d}/k", "wb").write(b"K" * 5000); open(f"{d}/r", "wb").write(b"R" * 3000)
open(f"{d}/v", "wb").write(b"V" * 1500); open(f"{d}/d", "wb").write(b"D" * 700)
# v2
mk("--kernel", f"{d}/k", "--ramdisk", f"{d}/r", "--dtb", f"{d}/d", "--cmdline", "console=ttyS0 panic=-1", "--output", f"{d}/b2.img")
b = open(f"{d}/b2.img", "rb").read(); f = struct.unpack_from("<8s10I16s512s32s1024sIQIIQ", b)
check("boot v2 : en-tete", f[0] == b"ANDROID!" and f[1] == 5000 and f[3] == 3000 and f[9] == 2 and f[17] == 1660 and f[18] == 700)
check("boot v2 : cmdline et alignement", f[12].rstrip(b"\0") == b"console=ttyS0 panic=-1" and len(b) % 2048 == 0)
check("boot v2 : disposition", b[2048:2049] == b"K" and b[2048 + 6144:2049 + 6144] == b"R" and b[2048 + 6144 + 4096:2049 + 6144 + 4096] == b"D")
# v4 + vendor_boot
mk("--header-version", "4", "--kernel", f"{d}/k", "--ramdisk", f"{d}/r", "--cmdline", "androidboot.selinux=permissive",
   "--output", f"{d}/b4.img", "--vendor-boot", f"{d}/vb.img", "--vendor-ramdisk", f"{d}/v", "--dtb", f"{d}/d", "--vendor-cmdline", "console=ttyS0")
b = open(f"{d}/b4.img", "rb").read(); h = struct.unpack_from("<8s4I4II1536sI", b)
check("boot v4 : en-tete", h[0] == b"ANDROID!" and h[1] == 5000 and h[2] == 3000 and h[4] == 1584 and h[9] == 4 and h[11] == 0)
check("boot v4 : cmdline", h[10].rstrip(b"\0") == b"androidboot.selinux=permissive")
check("boot v4 : pages de 4096", len(b) == 4096 + 8192 + 4096 and b[4096:4097] == b"K" and b[4096 + 8192:4097 + 8192] == b"R")
v = open(f"{d}/vb.img", "rb").read(); g = struct.unpack_from("<8sIIIII2048sI16sIIQIIII", v)
check("vendor_boot v4 : en-tete", g[0] == b"VNDRBOOT" and g[1] == 4 and g[2] == 4096 and g[5] == 1500 and g[9] == 2128 and g[10] == 700)
check("vendor_boot v4 : table de ramdisk", g[12] == 108 and g[13] == 1 and g[14] == 108 and g[15] == 0)
check("vendor_boot v4 : disposition", v[4096:4097] == b"V" and v[8192:8193] == b"D" and v[12288:12289] != b"\0" and len(v) == 16384)
sys.exit(1 if bad else 0)
