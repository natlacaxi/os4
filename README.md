# TALCO-OS3

Squelette d'OS mobile (noyau Linux). Simulation hote fonctionnelle :

```
make        # compile
make run    # demarre la simulation
make shell  # demarre avec le shell TALCO
make test   # tests d'integration
make image  # image rootfs de demo
```

Langages par module : `00_FOUNDATION/specifications/LANGUAGES.md`.

## Vrai noyau bootable (x86)

```
make os     # compile talco-kernel/talco-kernel.elf
make qemu   # lance dans QEMU (qemu-system-i386)
```

ISO : `make -C talco-kernel iso` (necessite grub-mkrescue). Le noyau demarre sur PC reel ou VM, avec ecran VGA, port serie, clavier et un shell.

## v0.2
```
make -C talco-kernel test          # 28 tests du coeur portable
make os / make os-arm64            # noyaux x86 / ARM64
make ui                            # rend les ecrans telephone et tablette (PPM)
```
Voir 21_DOCUMENTATION/hardware/PORTING.md pour l'etat reel par appareil.
