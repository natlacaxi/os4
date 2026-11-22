# Galaxy A34 5G : TALCO-OS3

## Trois niveaux, du plus simple au plus risque
**Niveau 1 : launcher Android (sans risque, utilisable aujourd'hui).** `android-launcher/` construit `talco-launcher.apk`, qui fait tourner
le launcher TALCO (accueil + calculatrice + notes) dans Android : pas de deverrouillage, pas de flash, desinstallable. Les appels,
SMS, Wi-Fi et le web restent ceux d'Android. Voir 21_DOCUMENTATION/hardware/ANDROID_LAUNCHER.md.

**Niveau 2 : simulation fidele dans QEMU.** Le job `linux-arm64` demarre Linux + TALCO sur un ecran virtuel **1080x2340, 420 dpi** (le format
de l'A34) et produit `screen-arm64-a34.png`. Cela valide la mise en page, pas le materiel.

**Niveau 3 : systeme sur l'appareil.** Il faut : deverrouiller le bootloader, le noyau Android de Samsung (5.10, modules vendor, AVB),
des images boot v4 + vendor_boot (le generateur `mkbootimg_min.py` les produit, a verifier avec l'outil officiel), puis un flash par Odin.
Le modem, la camera, le GPU et le Wi-Fi n'ont pas de pilote libre pret : un systeme TALCO sur l'A34 serait sans reseau ni appels.
Estimation honnete : plusieurs mois, resultat partiel, risque de rendre l'appareil inutilisable. **A reserver a un appareil de secours.**

## Verifier avant tout
- Numero de modele exact et version du bootloader (Parametres > A propos > Informations sur le logiciel).
- « Deverrouillage OEM » present dans les Options developpeur (sinon, impossible).
- Sauvegarde complete des donnees (le deverrouillage efface tout ; Samsung Pay/Pass et certaines apps bancaires cessent de fonctionner).
