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

PYTHON_CMD="python3"

# Try to create virtual environment
if python3 -m venv venv 2>/dev/null; then
    echo "Virtual environment created."
    ./venv/bin/pip install pygame paho-mqtt
    PYTHON_CMD="./venv/bin/python3"
else
    echo "WARNING: Could not create virtual environment (missing python3-venv?)."
    echo "Attempting to install dependencies to user library..."
    pip3 install --user pygame paho-mqtt
fi

echo "Starting Interface (Local)..."
$PYTHON_CMD interface_mina.py

# 4. Cleanup
echo "Stopping Mosquitto..."
docker compose down
