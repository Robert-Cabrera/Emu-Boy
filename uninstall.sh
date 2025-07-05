#!/bin/bash
# Script to uninstall the Gameboy Emulator

echo "Uninstalling Gameboy Emulator..."

# Remove desktop entry
if [ -f "${HOME}/.local/share/applications/GameboyEmulator.desktop" ]; then
    echo "Removing desktop entry..."
    rm "${HOME}/.local/share/applications/GameboyEmulator.desktop"
    echo "Desktop entry removed."
else
    echo "Desktop entry not found."
fi

# Optionally, you can also remove the build directory
echo "Do you want to remove the build directory? (y/n)"
read -r response
if [[ "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
    echo "Removing build directory..."
    rm -rf ./build
    echo "Build directory removed."
fi

echo "Uninstallation complete."
