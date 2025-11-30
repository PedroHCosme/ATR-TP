#!/bin/bash

# 1. Build the C++ Docker Image
echo "Building C++ Docker Image (atr_cpp)..."
docker build -t atr_cpp .

# 2. Start Mosquitto (using Docker Compose)
echo "Starting Mosquitto..."
# Ensure directories exist
mkdir -p mosquitto/config mosquitto/data mosquitto/log
touch mosquitto/log/mosquitto.log
# Start only mosquitto service
docker compose up -d mosquitto

# 3. Run the Python Interface Locally
echo "Setting up Python environment..."

# Create virtual environment if it doesn't exist
if [ ! -d "venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv venv
fi

# Install dependencies
echo "Installing dependencies..."
./venv/bin/pip install pygame paho-mqtt

echo "Starting Interface (Local)..."
./venv/bin/python3 interface_mina.py

# 4. Cleanup
echo "Stopping Mosquitto..."
docker compose down
