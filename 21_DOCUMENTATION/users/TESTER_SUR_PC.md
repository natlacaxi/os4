# Tester TALCO-OS3 depuis un PC

## 1. Voir les ecrans (sans rien installer)
GitHub > Actions > derniere execution > Artifacts > `host` : `phone.png`, `tablet.png`, `a34.png`, `calc.png`, `notes.png`, `files.png`, `settings.png`.

## 2. Essayer l'interface a la souris (QEMU)
1. Installer QEMU : Windows `winget install SoftwareFreedomConservancy.QEMU` (ou qemu.org) ; macOS `brew install qemu` ; Linux `sudo apt install qemu-system-x86`.
2. GitHub > Artifacts > `linux-x86_64` : dezipper pour obtenir `talco-linux-x86_64.img`.
3. Lancer : `tools/pc/lancer_qemu.ps1` (Windows) ou `tools/pc/lancer_qemu.sh`. Ajouter `telephone` pour un ecran vertical.
4. Attendre 30 a 90 s (pas d'acceleration) : la fenetre affiche l'accueil, clic = toucher. Le terminal (serial) donne le shell TALCO.
Dans cette version Linux, Calculatrice et Notes fonctionnent ; Fichiers et Reglages sont des pages « Contenu a venir ».

## 3. Tester l'appli Android depuis le PC
**Sur la vraie tablette en USB (recommande)** : activer Options developpeur > Debogage USB, installer « platform-tools » (adb) puis :
```
adb install -r talco-launcher.apk
adb logcat -s AndroidRuntime TALCO
```
`logcat` affiche l'erreur exacte en cas de plantage : la copier dans le chat.
**Emulateur** : Android Studio > Device Manager > Create device (image x86_64, API 34) puis glisser `talco-launcher.apk` dans la fenetre (l'APK contient maintenant arm64 et x86_64).
