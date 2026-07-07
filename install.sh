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
mkdir -p "$HOME/.local/share/icons"
mkdir -p "$HOME/.local/share/applications"

# 3. Copie de l'exécutable
echo -e "${BLUE}3. Copie de l'exécutable vers ~/.local/bin/...${NC}"
if [ -f "$HOME/.local/bin/tux-it" ]; then
    rm -f "$HOME/.local/bin/tux-it.old" || true
    mv "$HOME/.local/bin/tux-it" "$HOME/.local/bin/tux-it.old" || true
fi
cp build/tux-it "$HOME/.local/bin/tux-it"
chmod +x "$HOME/.local/bin/tux-it"

# 4. Export de l'icône de l'application
echo -e "${BLUE}4. Génération et export de l'icône de bureau...${NC}"
"$HOME/.local/bin/tux-it" --export-icon "$HOME/.local/share/icons/tux-it.png" || true

# 5. Création du lanceur desktop (.desktop)
echo -e "${BLUE}5. Création du raccourci dans le menu des applications...${NC}"
cat <<EOF > "$HOME/.local/share/applications/tux-it.desktop"
[Desktop Entry]
Name=Tux-It
Comment=Vos notes mignonnes sur le bureau
Exec=$HOME/.local/bin/tux-it
Icon=tux-it
Terminal=false
Type=Application
Categories=Utility;Office;
StartupNotify=true
EOF
chmod +x "$HOME/.local/share/applications/tux-it.desktop"

# 6. Mise à jour de la base desktop
if command -v update-desktop-database &> /dev/null; then
    update-desktop-database "$HOME/.local/share/applications" || true
fi

echo -e "${GREEN}=== Installation réussie ! ===${NC}"
echo -e "L'application a été ajoutée à votre lanceur. Vous pouvez la démarrer directement depuis votre menu d'applications sous le nom ${GREEN}Tux-It${NC}."
