#!/bin/bash
# Script to install the emulator, checking for dependencies first.

set -e # Exit if any command fails

# --- Dependency Check ---
echo "Checking for required dependencies..."

# We check for a key library like libsdl2-dev. If it's missing, we prompt the user.
if ! dpkg -s libsdl2-dev >/dev/null 2>&1; then
    echo " Required dependencies (build-essential, cmake, SDL2) appear to be missing."
    read -p "Do you want to install them now? (y/n) " response
    if [[ "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
        echo "Installing dependencies via apt-get..."
        if command -v sudo >/dev/null 2>&1; then
            sudo apt-get update -y
            sudo apt-get install -y build-essential cmake libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev
            echo "Dependencies installed."
        else
            echo "Error: 'sudo' command not found. Please install dependencies manually and run this script again."
            exit 1
        fi
    else
        echo "Installation cancelled by user. Please install dependencies manually to proceed."
        exit 1
    fi
else
    echo "Dependencies are already satisfied."
fi

# --- Shortcut Creation ---
echo "Creating desktop shortcut..."

# Get the absolute path to the directory containing this script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Ensure the run script is executable
chmod +x "${SCRIPT_DIR}/run.sh"

# Define the content of the .desktop file
DESKTOP_CONTENT="[Desktop Entry]
Type=Application
Name=Gameboy Emulator
Comment=A custom GameBoy emulator
Exec=${SCRIPT_DIR}/run.sh
Icon=${SCRIPT_DIR}/assets/icon.png
Terminal=false
Categories=Game;Emulator;
Keywords=gameboy;emulator;gb;
StartupNotify=true"

# Define the application directory and create it if it doesn't exist
APP_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
mkdir -p "$APP_DIR"

# Write the .desktop file
echo "$DESKTOP_CONTENT" > "$APP_DIR/GameboyEmulator.desktop"

# Make the .desktop file executable
chmod +x "$APP_DIR/GameboyEmulator.desktop"

echo "Success! The emulator shortcut has been created."
echo "You can now launch it from your application menu."
