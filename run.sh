#!/bin/bash

# Prepare XAuthority for Podman
if [ -z "$XAUTHORITY" ]; then
    export XAUTHORITY=$HOME/.Xauthority
fi
# Ensure file exists to avoid Docker mount error
# Ensure file exists to avoid Docker mount error
touch $XAUTHORITY 2>/dev/null || true

# 0. Nuke everything to prevent zombies
chmod +x nuke.sh
./nuke.sh

# 1. Build the C++ Docker Image
echo "Building C++ Docker Image (atr_cpp)..."
docker build -t atr_cpp .

# 2. Start Mosquitto (using Docker Compose)
echo "Starting Mosquitto..."
# Ensure directories exist
mkdir -p mosquitto/config mosquitto/data mosquitto/log
touch mosquitto/log/mosquitto.log
# Start only mosquitto service
XAUTHORITY=$XAUTHORITY docker compose up -d mosquitto

# 3. Run the Python Interface Locally
echo "Setting up Python environment..."

# Create virtual environment if it doesn't exist
if [ ! -d "venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv venv
    if [ $? -ne 0 ]; then
        echo "venv creation failed. Attempting to install python3-venv..."
        sudo apt-get update && sudo apt-get install -y python3-venv
        python3 -m venv venv
        if [ $? -ne 0 ]; then
            echo "Error: Failed to create virtual environment. Please install python3-venv manually."
            exit 1
        fi
    fi
fi

# Install dependencies
echo "Installing dependencies..."
./venv/bin/pip install pygame paho-mqtt

# Ensure xterm is installed (fallback for gnome-terminal)
if ! command -v xterm &> /dev/null; then
    echo "xterm not found. Attempting to install..."
    sudo apt-get update && sudo apt-get install -y xterm
    if [ $? -ne 0 ]; then
        echo "Error: Failed to install xterm. Please install it manually."
        exit 1
    fi
fi

echo "Starting Interface (Local)..."
# Ensure DISPLAY is set
if [ -z "$DISPLAY" ]; then
    export DISPLAY=:0
fi
./venv/bin/python3 interface_mina.py

# 4. Cleanup
echo "Stopping Mosquitto..."
docker compose down
