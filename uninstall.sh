#!/usr/bin/env bash
# Couleurs pour le terminal
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}=== Désinstallation de Post-It ===${NC}"

# 1. Suppression du binaire
if [ -f "$HOME/.local/bin/post-it" ]; then
    echo -e "Suppression de l'exécutable..."
    rm "$HOME/.local/bin/post-it"
fi

# 2. Suppression de l'icône
if [ -f "$HOME/.local/share/icons/post-it.png" ]; then
    echo -e "Suppression de l'icône..."
    rm "$HOME/.local/share/icons/post-it.png"
fi

# 3. Suppression du raccourci bureau (.desktop)
if [ -f "$HOME/.local/share/applications/post-it.desktop" ]; then
    echo -e "Suppression du raccourci bureau..."
    rm "$HOME/.local/share/applications/post-it.desktop"
fi

# 4. Mise à jour de la base de données desktop si disponible
if command -v update-desktop-database &> /dev/null; then
    update-desktop-database "$HOME/.local/share/applications" || true
fi

echo -e "${GREEN}=== Désinstallation terminée avec succès ! ===${NC}"
echo -e "Note : Vos notes locales et vos fichiers de configuration (dans ${BLUE}~/.config/DevEnDev/Post-It/${NC}) ont été conservés pour éviter toute perte de données."
echo -e "Si vous souhaitez également supprimer toutes vos notes et configurations, lancez : ${RED}rm -rf ~/.config/DevEnDev/Post-It/${NC}"
