# DESIGN.md

# Sticky Notes - Design System

Version: 1.0
Target: Linux Desktop
Framework: Qt 6
Style: Native + Modern
Theme: Light / Dark

---

# Philosophy

L'application doit être :

- Instantanée
- Discrète
- Élégante
- Peu gourmande
- Sans distraction
- Pensée pour une utilisation quotidienne

L'utilisateur ne doit jamais avoir besoin d'apprendre à utiliser le logiciel.

Tout doit être intuitif.

---

# Design Principles

## Minimal First

Chaque fenêtre ne contient que les éléments nécessaires.

Aucun bouton inutile.

Pas d'animations excessives.

---

## Content First

Le texte est toujours plus important que l'interface.

Le post-it est le centre de l'application.

---

## Native Feeling

Respecter les conventions Linux.

- Fenêtres natives
- Police système
- Icônes système
- Raccourcis classiques

---

## Fast Interaction

Toutes les actions importantes doivent demander :

Maximum 1 clic

ou

1 raccourci clavier

---

# Window

Chaque post-it est une vraie fenêtre indépendante.

Pas de fenêtre principale obligatoire.

Chaque note peut vivre seule.

---

# Window Decoration

Deux modes :

## Native

Décoration système

ou

## Borderless

Aspect flottant.

Coins légèrement arrondis.

Ombre discrète.

---

# Header

Hauteur :

32 px

Contient :

☰ Menu

Titre

Espace vide

📌 Toujours devant

🎨 Couleur

⋮ Plus

Le header reste très discret.

---

# Body

Occupe tout l'espace.

Aucune bordure visible.

Padding :

16 px

Texte aligné à gauche.

---

# Typography

Police :

System Default

Fallback :

Noto Sans

---

Titre :

16 px
Semi Bold

---

Texte :

14 px

Interligne :

1.4

---

# Colors

Le logiciel doit proposer plusieurs couleurs prédéfinies.

## Jaune

#FFF59D

---

## Bleu

#90CAF9

---

## Vert

#A5D6A7

---

## Rose

#F8BBD0

---

## Orange

#FFCC80

---

## Violet

#CE93D8

---

## Gris

#E0E0E0

---

## Blanc

#FAFAFA

---

## Couleur personnalisée

L'utilisateur peut choisir n'importe quelle couleur.

Color Picker Qt.

---

# Text Color

Calcul automatique.

Texte noir

ou

Texte blanc

selon le contraste.

Jamais configurable.

Toujours lisible.

---

# Transparency

Chaque note possède :

Opacity

100%

95%

90%

85%

80%

70%

---

# Pin

Chaque note peut être :

Toujours au premier plan

ON

OFF

Utilise :

Qt::WindowStaysOnTopHint

---

# Window Behavior

Chaque note mémorise :

Position

Taille

Couleur

Etat Always On Top

Opacité

Etat verrouillé

Dernière modification

---

# Lock Mode

Une note peut être verrouillée.

Dans ce mode :

Impossible de déplacer

Impossible de redimensionner

Edition désactivée

Icône cadenas affichée.

---

# Resize

Toutes les notes sont redimensionnables.

Taille minimum :

180 x 180

---

# Search

Fenêtre de recherche globale.

Recherche instantanée.

Recherche dans :

Titre

Contenu

Tags

---

# Tags

Chaque note peut contenir plusieurs tags.

Exemple :

Travail

Maison

Urgent

Personnel

Projet

---

# Favorites

Une note peut être marquée favorite.

Icône étoile.

---

# Archive

Une note peut être archivée.

Elle disparaît du bureau.

Accessible via :

Archive

---

# Trash

Suppression douce.

Corbeille.

Suppression définitive uniquement sur demande.

---

# Autosave

Sauvegarde :

Après 500 ms d'inactivité.

Jamais de bouton Sauvegarder.

---

# Markdown

Support léger :

# Titre

## Sous titre

- Liste

**gras**

*italique*

`code`

---

# Checklist

Support :

☐

☑

Cocher met automatiquement à jour la note.

---

# Images

Glisser-déposer une image.

Affichage inline.

Redimensionnement automatique.

---

# Links

Les URLs deviennent cliquables.

mailto:

http

https

file://

---

# Drag & Drop

Support :

Texte

Images

Fichiers

URL

---

# Context Menu

Clic droit :

Nouvelle note

Dupliquer

Changer couleur

Toujours devant

Verrouiller

Exporter

Partager

Archiver

Supprimer

---

# Export

Formats :

TXT

Markdown

PDF

HTML

---

# Import

TXT

Markdown

---

# Keyboard Shortcuts

Ctrl + N

Nouvelle note

---

Ctrl + F

Recherche

---

Ctrl + D

Dupliquer

---

Ctrl + L

Verrouiller

---

Ctrl + Shift + P

Toujours devant

---

Ctrl + +

Zoom

---

Ctrl + -

Dézoom

---

Ctrl + Mouse Wheel

Zoom texte

---

# Animations

Très discrètes.

150 ms maximum.

Utiliser :

QPropertyAnimation

Uniquement pour :

Fade In

Fade Out

Color transition

---

# Shadows

Ombre légère.

Jamais agressive.

---

# Border Radius

12 px

---

# Spacing

Utiliser une grille de 8 px.

Tous les espacements doivent être multiples de :

4

ou

8

---

# Notifications

Aucune notification inutile.

Seulement :

Sauvegarde impossible

Import terminé

Export terminé

---

# Tray Icon

Icône système.

Menu :

Nouvelle note

Afficher toutes

Masquer toutes

Recherche

Paramètres

Quitter

---

# Settings

## Général

Démarrage automatique

Créer une note au lancement

Toujours dans la zone de notification

Confirmation avant suppression

---

## Apparence

Theme

Light

Dark

System

---

Coins arrondis

ON/OFF

---

Ombres

ON/OFF

---

Animations

ON/OFF

---

Police

Choix utilisateur

---

Taille police

10

...

24

---

## Notes

Couleur par défaut

Taille par défaut

Position intelligente

Opacité par défaut

Toujours devant par défaut

---

# Multi Monitor

Les positions doivent être restaurées correctement.

Si un écran disparaît :

Replacer automatiquement les notes sur l'écran principal.

---

# Persistence

Toutes les modifications sont sauvegardées automatiquement.

Aucune perte de données.

---

# Error Handling

En cas d'erreur disque :

Conserver une copie mémoire.

Réessayer automatiquement.

Informer l'utilisateur uniquement si nécessaire.

---

# Accessibility

Navigation complète au clavier.

Contrastes WCAG.

Support HiDPI.

Support Screen Reader.

---

# Performance

Objectifs :

Lancement < 500 ms

Création d'une note < 50 ms

Consommation mémoire :

< 80 Mo au démarrage

< 2 Mo par note supplémentaire

CPU au repos :

≈ 0 %

---

# Future Features

Synchronisation Nextcloud

Synchronisation WebDAV

Synchronisation Git

Historique des versions

Rappels

Notifications programmées

Dessins manuscrits

Enregistrement vocal

OCR

IA locale

Fenêtres groupées

Espaces de travail

Templates

Mode Kanban

Calendrier

Widgets

Plugin System

Support Wayland complet

Support X11 complet

```

Ce design fournit une base moderne tout en restant fidèle à la philosophie Linux : simplicité, rapidité, faible consommation de ressources et comportement natif. Il prévoit également une architecture évolutive avec des fonctionnalités avancées qui pourront être ajoutées sans remettre en cause l'interface existante.
