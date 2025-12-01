#!/bin/bash

echo "☢️  NUKING EVERYTHING... ☢️"

# 1. Kill Local Python Processes
echo "🔪 Killing local Python processes..."
pkill -f interface_mina.py || true
pkill -f "python3 interface_mina.py" || true

# 2. Kill Local C++ Binaries (if any ran locally)
echo "🔪 Killing local C++ binaries..."
pkill -f bin/app || true
pkill -f bin/simulador || true
pkill -f bin/cockpit || true
pkill -f bin/interface_simulacao || true

# 3. Stop and Remove Docker Containers
echo "🐳 Stopping Docker containers..."
# Stop all containers related to the project image 'atr_cpp'
docker stop $(docker ps -q --filter ancestor=atr_cpp) 2>/dev/null || true
# Stop Mosquitto container
docker stop mosquitto_broker 2>/dev/null || true
# Remove stopped containers
docker rm $(docker ps -aq --filter ancestor=atr_cpp) 2>/dev/null || true
docker rm mosquitto_broker 2>/dev/null || true
docker rm atr_app 2>/dev/null || true

# 4. Clean Mosquitto Data (Optional but recommended for ghost data)
echo "🧹 Cleaning Mosquitto data..."
rm -rf mosquitto/data/* 2>/dev/null || true
rm -rf mosquitto/log/* 2>/dev/null || true
# Recreate empty log file
touch mosquitto/log/mosquitto.log

# 5. Reset Terminal (Fixes messed up Ncurses state)
echo "🖥️  Resetting terminal..."
reset

echo "✅ NUKE COMPLETE. System is clean."
