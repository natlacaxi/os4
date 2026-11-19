# Noyau TALCO (talco-kernel/)
- `core/` : code portable (chaines, tas, systeme de fichiers RAM, shell) -- teste sur l'ordinateur : `make -C talco-kernel test`.
- `arch/x86` : Multiboot, VGA, serie, clavier, IDT, PIC, timer 100 Hz.
- `arch/aarch64` : UART PL011, compteur ARM, PSCI (QEMU virt).
- Ajouter une architecture : implementer `include/hal.h`.
Limites actuelles : pas de multitache, pas de memoire virtuelle, pas de pilotes de stockage ni de reseau.
