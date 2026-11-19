# Porter TALCO-OS3 sur telephone et tablette

## Etat reel (v0.2.0)
| Cible | Etat |
|---|---|
| PC x86 / QEMU i386 | noyau + shell + fichiers (non teste en execution ici) |
| QEMU ARM64 (virt) | code ecrit, **non compile ni execute** (pas de chaine ARM64 dispo) |
| Telephone / tablette reels | **pas encore** : aucun driver (ecran, tactile, modem, GPU...) |
| Interface adaptive telephone/tablette | fonctionne en rendu logiciel, verifiee par captures (tests/phone.png, tablet.png) |

## Chemin realiste vers de vrais appareils
1. Valider le noyau ARM64 sur QEMU : `make os-arm64 && make -C talco-kernel ARCH=aarch64 qemu`.
2. Passer sur une carte ouverte et documentee (ex. Raspberry Pi 4/5 : UART, framebuffer, tactile USB) : c'est la vraie premiere tablette.
3. Pour Samsung/Pixel/Xiaomi/OnePlus : bootloader deverrouille + noyau Linux de `01_KERNEL/linux` avec le device tree de `16_DEVICE_SUPPORT`, puis TALCO_CORE et TALCO_EXPERIENCE en espace utilisateur. Un noyau maison ne fera pas fonctionner les drivers proprietaires (modem, GPU, camera).
4. Image de demarrage Android (`boot.img`) : a produire depuis `17_BUILD_SYSTEM/image_builder`.

## Adaptation telephone / tablette
`ui_form_factor(w, h, dpi)` applique la regle de la plus petite largeur : >= 600 dp = tablette.
Telephone : grille 4 colonnes, dock en bas, geste. Tablette : colonne de widgets + grille 6 colonnes + dock centre. Pliable : bascule entre les deux selon le depliage (`foldable_layouts` dans device.yaml).
