# Noyau Linux pour TALCO-OS3
`build_linux.sh x86_64|arm64` telecharge Linux LTS 6.12 depuis kernel.org, y integre le coeur TALCO comme `/init`
(shell, fichiers, tas : voir `talco-kernel/`) et produit `build/talco-linux-<arch>.img`, demarrable dans QEMU.
Config : `01_KERNEL/configuration/talco_linux.config` (initramfs, devtmpfs, virtio GPU/clavier-tactile).
Etape suivante : lancer le launcher (05_TALCO_EXPERIENCE) sur /dev/fb0 et lire le tactile via /dev/input.
