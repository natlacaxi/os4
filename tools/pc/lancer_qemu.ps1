# Windows (PowerShell) : lance TALCO-OS3 (Linux + ecran tactile virtuel) dans QEMU.
# Usage : .\lancer_qemu.ps1 [chemin\talco-linux-x86_64.img] [tablette|telephone]
param([string]$Image = ".\talco-linux-x86_64.img", [string]$Forme = "tablette")
$qemu = "C:\Program Files\qemu\qemu-system-x86_64.exe"
if (-not (Test-Path $qemu)) { $qemu = "qemu-system-x86_64" }
$taille = if ($Forme -eq "telephone") { "xres=720,yres=1560" } else { "xres=1280,yres=800" }
& $qemu -m 512 -cpu max -kernel $Image -append "console=ttyS0 panic=-1" `
  -vga none -device "virtio-gpu-pci,edid=on,$taille" -device virtio-tablet-pci `
  -display "gtk,zoom-to-fit=on" -serial stdio
