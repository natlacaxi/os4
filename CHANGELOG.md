# Changelog

## 0.0.1
- Squelette complet, boot simule, tests, image de demo.

## 0.2.0
- Noyau : HAL, tas, systeme de fichiers RAM, shell etendu, timer x86, port ARM64 (non teste).
- Launcher adaptatif telephone/tablette.

## 0.3.0
- Chemin Linux : build_linux.sh (Linux LTS 6.12 + coeur TALCO en /init), tests QEMU x86_64 et arm64 en CI.

- Calculatrice fonctionnelle dans le launcher (tactile), tests de calcul.

- Correctif : options noyau FB/DRM/virtio pour l ecran graphique, verification des options en CI.

- App Notes avec clavier AZERTY tactile ; heure de Paris dans talco-ui.

## 0.4.0
- talco-ui : rendu DRM/KMS double tampon (page flip vblank) avec repli fbdev.
- Modele generique Samsung Galaxy A, generateur boot.img v2 teste, base noyau ARM64 telephone.

- Galaxy A34 5G : fiche appareil, launcher Android (APK) construit en CI, generateur boot v4 + vendor_boot, rendu 1080x2340 teste.
