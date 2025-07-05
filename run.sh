#!/bin/bash
# Script to build and run the emulator from the correct working directory

set -e  # Exit if make or emu fails

# Get the absolute path to the directory containing this script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Navigate to the script's directory to ensure paths are correct
cd "$SCRIPT_DIR"

# Check if build directory exists or needs regeneration
if [ ! -d "./build" ] || [ ! -f "./build/Makefile" ]; then
  echo "Generating build directory..."
  rm -rf ./build
  mkdir -p ./build
  cd ./build
  cmake ..
else
  # Navigate into the existing build directory
  cd ./build
fi

# Build and run
echo "Compiling project..."
make

echo "Launching Emulator..."
./emu
