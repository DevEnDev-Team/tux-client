# 💻 Client Desktop Tux-It (Qt6)

Ce dépôt contient le client Desktop de l'application **Tux-It**, écrit en **C++** avec le framework **Qt6**. C'est une application de notes adhésives (stickies) moderne, légère et adorable pour Linux.

Il s'intègre parfaitement avec notre serveur de synchronisation Go pour sauvegarder vos notes sur vos différents appareils.

---

## ✨ Fonctionnalités du Client

* **Design épuré** : Look moderne sans bordures système, avec des coins arrondis et la mascotte Renard.
* **Always on Top** : Reste au premier plan de façon robuste (compatible avec Wayland et X11).
* **Rich Text** : Formatage en gras (`Ctrl + B`), lock de l'édition graphique (cadenas), choix de couleur et opacité.
* **Assistant Graphique (SetupWizard)** : Assistant au premier démarrage pour configurer le raccourci d'application, le lancement automatique (Autostart) et la synchronisation en ligne.
* **Statut de Synchronisation** : Un indicateur visuel (nuage) montre si les notes locales sont synchronisées avec le serveur.
* **Design System** : Toutes les règles esthétiques, chartes graphiques et palettes de couleurs sont documentées dans le fichier [DESIGN.md](file:///home/mangoz404/Documents/Projets/Logiciels/tux-it/tux-client/DESIGN.md).

---

## 🛠️ Compilation & Installation

Le client s'installe localement sur votre session Linux sans droits root.

### 📋 Prérequis (Debian / Ubuntu / Pop!_OS)

Installez le compilateur C++, CMake et les bibliothèques Qt6 :
```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-base-private-dev
```

### 🚀 Installation Automatique

Lancez le script d'installation à la racine de ce dossier :
```bash
chmod +x install.sh
./install.sh
```
Ce script va :
1. Compiler l'application en mode Release.
2. Copier l'exécutable final dans `~/.local/bin/tux-it`.

### 🏎️ Lancement

Démarrez l'application depuis votre terminal ou via votre menu d'applications :
```bash
tux-it
```

---

## 🧹 Désinstallation

Pour retirer l'application du système, exécutez :
```bash
chmod +x uninstall.sh
./uninstall.sh
```
*Note : Vos données locales et configurations situées dans `~/.config/DevEnDev/Tux-It/` ne sont pas effacées pour éviter toute perte accidentelle.*

---

## 📄 Licence
Ce projet est sous licence MIT.
