# Samsung Galaxy A : modele generique

## Verite d'abord
- **Un systeme unique pour « tous les Galaxy A » n'existe pas.** Chaque modele a son SoC (Exynos, Snapdragon ou MediaTek), son ecran, son tactile, son modem. Il faut un noyau et un arbre de peripheriques (DTB) par modele.
- **Bootloader** : sur beaucoup de Galaxy A il est verrouille (operateurs, Etats-Unis) ; deverrouiller efface les donnees et declenche Knox (garantie perdue, Samsung Pay/Pass indisponibles). Verifier « Deverrouillage OEM » dans les options developpeur.
- **Linux principal (mainline)** supporte tres peu de Galaxy A. En pratique on part du noyau Android de Samsung (sources sur opensource.samsung.com) et de l'approche Halium/Droidian, pas d'un noyau mainline.
- **Flash** : mode Download + Odin ou Heimdall, pas fastboot. Les images recentes (Android 11+) utilisent boot v3/v4 + vendor_boot + AVB : le generateur fourni ne couvre que v0-v2.

## Methode de rendu (du meilleur au repli)
1. **drm_gpu** : DRM/KMS atomique + GPU via Mesa (Panfrost pour les Mali Exynos/MediaTek, Freedreno pour Adreno Snapdragon), compositeur Wayland, EGL/GLES. Meilleur rendu et meilleure autonomie. A faire quand le GPU est supporte.
2. **drm_dumb** : DRM/KMS, 2 tampons, page flip synchronise sur le vblank, dessin CPU. **Implemente dans talco-ui** : sans dechirure, marche sur tout pilote KMS.
3. **fbdev** : repli pour un noyau sans DRM. Implemente aussi.
`talco-ui` choisit automatiquement 2 puis 3 et affiche « (rendu drm) » ou « (rendu fbdev) ».
Sur un noyau Android de Samsung (pilote d'ecran proprietaire, pas de DRM mainline), la voie realiste est Halium/libhybris : le rendu passe alors par le HWC d'Android.

## Pour ajouter un modele
1. `cp -r template galaxy_a/a54` puis remplir `device.yaml` (SoC, GPU, ecran, dpi).
2. Recuperer defconfig + DTB du modele (sources Samsung) ; completer `01_KERNEL/configuration/talco_arm64_phone.config`.
3. Construire : `mkbootimg_min.py --kernel Image --dtb modele.dtb --cmdline ... --output boot.img` (v2) puis emballer en `.tar` pour Odin.
4. Tester sur QEMU d'abord (job `linux-arm64`), puis sur l'appareil, **de secours uniquement**.
