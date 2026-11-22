#!/usr/bin/env python3
"""Demarre Linux+TALCO dans QEMU, teste le shell serie et (si une forme est donnee) l'affichage graphique avec capture d'ecran.
Usage : boot_linux_qemu.py x86_64|arm64 [tablet|phone]"""
import os, re, select, socket, subprocess, sys, time
arch = sys.argv[1] if len(sys.argv) > 1 else "x86_64"
form = sys.argv[2] if len(sys.argv) > 2 else ""
img = f"build/talco-linux-{arch}.img"
if not os.path.exists(img):
    print("FAIL: image absente", img); sys.exit(1)
os.makedirs("out", exist_ok=True)
sock = f"/tmp/talco-mon-{arch}-{form or 'serial'}.sock"
shot = os.path.abspath(f"out/screen-{arch}-{form}.ppm")
if os.path.exists(sock): os.remove(sock)
size = {"tablet": (1280, 800), "phone": (720, 1560)}.get(form)
if arch == "x86_64":
    cmd = ["qemu-system-x86_64", "-m", "512", "-cpu", "max", "-kernel", img, "-append", "console=ttyS0 panic=-1"]
    if size: cmd += ["-vga", "none"]
else:
    cmd = ["qemu-system-aarch64", "-M", "virt", "-cpu", "cortex-a57", "-m", "512", "-kernel", img, "-append", "console=ttyAMA0 panic=-1"]
if size:
    cmd += ["-device", f"virtio-gpu-pci,edid=on,xres={size[0]},yres={size[1]}", "-device", "virtio-tablet-pci",
            "-display", "none", "-serial", "stdio", "-monitor", f"unix:{sock},server,nowait"]
else:
    cmd += ["-nographic"]
cmd += ["-no-reboot"]
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

def screendump():
    for _ in range(20):
        if os.path.exists(sock): break
        time.sleep(0.5)
    try:
        s = socket.socket(socket.AF_UNIX); s.connect(sock); time.sleep(0.5)
        s.send(f"screendump {shot}\n".encode()); time.sleep(3); s.close()
    except OSError as e:
        print("capture impossible :", e)

ok = wait(b"[ok] pret", 300)
gui_marker = False
if ok:
    if size:
        gui_marker = wait(b"talco-ui:", 60)
        time.sleep(4)
        screendump()
    for c in ["help", "cat /etc/release", "mkdir /x", "write /x/a.txt salut", "cat /x/a.txt", "cpu", "halt"]:
        wait(b"talco:/", 5); time.sleep(0.5)
        p.stdin.write((c + "\r").encode()); p.stdin.flush()
        wait(b"\n", 1); time.sleep(1.5)
    wait(b"Arret", 10); time.sleep(3)
p.kill()
txt = re.sub(r"\x1b\[[0-9;]*[A-Za-z]", "", buf.decode(errors="replace")).replace("\r", "")
print("---- sortie serie ----"); print(txt[-6000:]); print("----------------------")
res = [("demarrage Linux + TALCO", "[ok] pret" in txt), ("banniere", "TALCO-OS3 0.4.0" in txt),
       ("tourne sur Linux", "sur Linux" in txt), ("fichier ecrit puis relu", any(l.strip() == "salut" for l in txt.split("\n")))]
if size:
    want = "tablette" if form == "tablet" else "telephone"
    res.append((f"ecran detecte ({want})", f"talco-ui: ecran {size[0]}x{size[1]}, {want}" in txt))
    good = False
    if os.path.exists(shot) and os.path.getsize(shot) > 0:
        try:
            from PIL import Image
            im = Image.open(shot).convert("RGB")
            im.save(shot.replace(".ppm", ".png")); os.remove(shot)
            good = len(set(im.resize((160, 100)).getdata())) > 60
        except Exception as e:
            print("analyse capture :", e)
    res.append(("capture d'ecran non vide", good))
bad = 0
for n, v in res:
    print(("OK   : " if v else "FAIL : ") + n); bad += not v
sys.exit(1 if bad else 0)
