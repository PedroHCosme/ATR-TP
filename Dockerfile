FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    mosquitto \
    mosquitto-clients \
    libmosquitto-dev \
    libncurses5-dev \
    libncursesw5-dev \
    python3 \
    python3-pip \
    python3-pygame \
    python3-pandas \
    python3-matplotlib \
    x11-apps \
    && rm -rf /var/lib/apt/lists/*

# Create working directory
WORKDIR /app

# Copy source code
COPY . /app

# Build the C++ project
RUN make

# Create a startup script
RUN echo '#!/bin/bash\n\
    # Start Mosquitto in background\n\
    mosquitto -c mosquitto.conf -d\n\
    echo "Mosquitto started."\n\
    \n\
    # Start Simulator in background\n\
    ./bin/simulador > simulator.log 2>&1 &\n\
    SIM_PID=$!\n\
    echo "Simulator started (PID $SIM_PID)."\n\
    \n\
    # Start Pygame Interface in background\n\
    python3 interface_mina.py &\n\
    PY_PID=$!\n\
    echo "Pygame Interface started (PID $PY_PID)."\n\
    \n\
    # Start Control App (Interactive)\n\
    echo "Starting Control App..."\n\
    ./bin/app\n\
    \n\
    # Cleanup on exit\n\
    kill $SIM_PID\n\
    kill $PY_PID\n\
    ' > start.sh && chmod +x start.sh

# Expose MQTT port
EXPOSE 1883

# Default command
CMD ["./start.sh"]
