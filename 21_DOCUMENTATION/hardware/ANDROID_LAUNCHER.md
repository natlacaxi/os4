# TALCO Launcher pour Android (Galaxy A34 5G, Lenovo Tab K11, tout appareil Android arm64)

## Obtenir l'APK
Le job `android` de GitHub produit `talco-launcher.apk` (Actions > execution > Artifacts > `android`).
Pour activer ce job : modifier `.github/workflows/ci.yml` et ajouter `, android` a la liste `job: [...]`.

## Installer
1. Dezipper l'artefact, ouvrir `talco-launcher.apk` depuis le gestionnaire de fichiers.
2. Autoriser l'installation depuis cette source si Android le demande ; Play Protect peut avertir (application non publiee, signee en debogage).
3. Ouvrir « TALCO » : accueil, calculatrice, notes (touche = ouvre, bouton Retour ou geste = ferme).

## L'utiliser comme ecran d'accueil
Samsung One UI : Parametres > Applications > Applications par defaut > Application d'ecran d'accueil > TALCO.
Pour revenir : meme menu, choisir « One UI Home ». Desinstaller : Parametres > Applications > TALCO.

## Applications
Les icônes Téléphone, Messages, Contacts, Navigateur, Appareil, Galerie, Calendrier, Mail, Musique, Plans, Boutique et Assistant ouvrent les **vraies applications Android** (les applis par défaut de l’appareil).
Calculatrice, Notes, **Fichiers** et **Réglages** sont celles de TALCO.

### Fichiers (gestionnaire, carte SD, tri)
- Onglets de volumes : **Interne** et **Carte SD** (si une carte est insérée), avec l’espace libre.
- Tri par **Nom, Taille, Date, Type** (touche une puce ; touche-la encore pour inverser). Les dossiers restent en premier.
- Touche un dossier pour l’ouvrir, la flèche **‹** pour remonter, un fichier pour l’ouvrir avec une autre appli. Fais défiler avec le doigt.
- **Appui long** sur un élément : Renommer, Copier, Déplacer, Supprimer. Bouton **+** : nouveau dossier. Bouton **Coller** : apparaît après Copier/Déplacer.
- **Autorisation** : au premier lancement, Android ouvre l’écran « Accès à tous les fichiers » : active-le pour TALCO, puis reviens. Sans cette autorisation, TALCO ne peut pas lire la carte SD. Tu peux la retirer à tout moment dans les réglages d’Android.

### Réglages (gestion de l’appareil)
- État : appareil, version d’Android, stockage interne et carte SD (barres), mémoire, batterie (charge, température), Wi-Fi, **carte SIM** (état et opérateur, jamais les contacts ni les SMS), luminosité.
- **Volume multimédia** : touche la barre pour le régler.
- Raccourcis vers les réglages d’Android : Wi-Fi, Bluetooth, Affichage, Son, Stockage, Batterie, Applications, Date et heure, À propos, Tous les réglages.

## Limites
Rendu logiciel (pas de GPU), pas de widgets Android, notes sans sauvegarde. La carte SD en lecture/écriture demande l’autorisation « Accès à tous les fichiers » (sans elle, TALCO ne voit pas la carte). Les fichiers ouverts depuis TALCO dépendent des applis installées ; certaines ne peuvent pas lire un fichier sur la carte SD. Les contacts de la carte SIM ne sont pas lus (utilise l’appli Contacts d’Android). 
