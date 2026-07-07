#!/usr/bin/env bash
set -e

# Couleurs pour le terminal
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== Installation de Tux-It ===${NC}"

# 1. Compilation en mode Release
echo -e "${BLUE}1. Compilation du projet...${NC}"
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
cd ..

# 2. Création des dossiers utilisateur locaux
echo -e "${BLUE}2. Configuration des dossiers locaux...${NC}"
mkdir -p "$HOME/.local/bin"

# 3. Copie de l'exécutable
echo -e "${BLUE}3. Copie de l'exécutable vers ~/.local/bin/...${NC}"
if [ -f "$HOME/.local/bin/tux-it" ]; then
    rm -f "$HOME/.local/bin/tux-it.old" || true
    mv "$HOME/.local/bin/tux-it" "$HOME/.local/bin/tux-it.old" || true
fi
cp build/tux-it "$HOME/.local/bin/tux-it"
chmod +x "$HOME/.local/bin/tux-it"

echo -e "${GREEN}=== Installation réussie ! ===${NC}"
echo -e "Veuillez lancer l'application avec : ${GREEN}tux-it${NC} (ou depuis ~/.local/bin/tux-it) pour terminer la configuration graphique."
