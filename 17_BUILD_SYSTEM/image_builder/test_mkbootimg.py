#!/usr/bin/env python3
"""Verifie que mkbootimg_min produit un boot.img v2 coherent (relecture de l'en-tete)."""
import os, struct, subprocess, sys, tempfile
here = os.path.dirname(os.path.abspath(__file__))
d = tempfile.mkdtemp()
open(f"{d}/k", "wb").write(b"K" * 5000); open(f"{d}/r", "wb").write(b"R" * 3000); open(f"{d}/d", "wb").write(b"D" * 700)
subprocess.check_call([sys.executable, f"{here}/mkbootimg_min.py", "--kernel", f"{d}/k", "--ramdisk", f"{d}/r", "--dtb", f"{d}/d",
                       "--cmdline", "console=ttyS0 panic=-1", "--output", f"{d}/boot.img"], stdout=subprocess.DEVNULL)
b = open(f"{d}/boot.img", "rb").read()
f = struct.unpack_from("<8s10I16s512s32s1024sIQIIQ", b)
ok = f[0] == b"ANDROID!" and f[1] == 5000 and f[3] == 3000 and f[8] == 2048 and f[9] == 2 and f[-3] == 1660 and f[-2] == 700
ok &= f[12].rstrip(b"\0") == b"console=ttyS0 panic=-1" and len(b) % 2048 == 0
ok &= b[2048:2049] == b"K" and b[2048 + 6144:2048 + 6145] == b"R" and b[2048 + 6144 + 4096:2048 + 6144 + 4097] == b"D"
print("boot.img v2 :", "OK" if ok else "ECHEC"); sys.exit(0 if ok else 1)
