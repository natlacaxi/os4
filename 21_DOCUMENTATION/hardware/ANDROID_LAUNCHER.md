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

## Limites
Rendu logiciel (pas de GPU), pas de widgets Android, apps TALCO = celles du depot (calculatrice, notes sans sauvegarde). Pas de boutons d'appel ni de SMS reels.
