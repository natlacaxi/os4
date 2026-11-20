#!/usr/bin/env python3
"""Demarre l'image Linux+TALCO dans QEMU, attend le shell, envoie des commandes. Usage : boot_linux_qemu.py x86_64|arm64"""
import os, re, select, subprocess, sys, time
arch = sys.argv[1] if len(sys.argv) > 1 else "x86_64"
img = f"build/talco-linux-{arch}.img"
if not os.path.exists(img):
    print("FAIL: image absente", img); sys.exit(1)
if arch == "x86_64":
    cmd = ["qemu-system-x86_64", "-m", "512", "-cpu", "max", "-kernel", img, "-append", "console=ttyS0 panic=-1", "-nographic", "-no-reboot"]
else:
    cmd = ["qemu-system-aarch64", "-M", "virt", "-cpu", "cortex-a57", "-m", "512", "-kernel", img, "-append", "console=ttyAMA0 panic=-1", "-nographic", "-no-reboot"]
p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
buf = b""
def wait(pat, t):
    global buf
    end = time.time() + t
    while time.time() < end:
        if pat in buf: return True
        r, _, _ = select.select([p.stdout], [], [], 1)
        if r:
            d = os.read(p.stdout.fileno(), 65536)
            if not d: return pat in buf
            buf += d
    return pat in buf
ok = wait(b"[ok] pret", 240)
if ok:
    for c in ["help", "cat /etc/release", "mkdir /x", "write /x/a.txt salut", "cat /x/a.txt", "cpu", "halt"]:
        n = buf.count(b"talco:/")
        wait(b"talco:/", 5)            # laisse le prompt s'afficher
        time.sleep(0.5)
        p.stdin.write((c + "\r").encode()); p.stdin.flush()
        wait(b"\n", 1)
        time.sleep(1.5)
    wait(b"Arret", 10); time.sleep(3)
p.kill()
buf += b""
txt = re.sub(r"\x1b\[[0-9;]*[A-Za-z]", "", buf.decode(errors="replace")).replace("\r", "")
print("---- sortie serie ----"); print(txt[-6000:]); print("----------------------")
res = [("demarrage Linux + TALCO", "[ok] pret" in txt), ("banniere", "TALCO-OS3 0.3.0" in txt),
       ("tourne sur Linux", "sur Linux" in txt), ("fichier ecrit puis relu", any(l.strip() == "salut" for l in txt.split("\n")))]
bad = 0
for n, v in res:
    print(("OK   : " if v else "FAIL : ") + n); bad += not v
sys.exit(1 if bad else 0)
