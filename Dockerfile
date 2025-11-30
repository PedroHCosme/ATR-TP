FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Update and install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    make \
    libmosquitto-dev \
    libncurses5-dev \
    libncursesw5-dev \
    python3 \
    python3-pip \
    python3-tk \
    mosquitto-clients \
    xterm \
    git \
    libboost-all-dev \
    nlohmann-json3-dev \
    fontconfig \
    && rm -rf /var/lib/apt/lists/*

# Install Python dependencies
RUN pip3 install pygame paho-mqtt

# Set working directory
WORKDIR /app

# Copy project files
COPY . /app

# Build the C++ applications
RUN make all

# Create necessary directories for logs if they don't exist
RUN mkdir -p logs bin

# Set environment variable to indicate Docker environment
ENV IS_DOCKER_CONTAINER=true

# Default command (can be overridden in docker-compose)
CMD ["python3", "interface_mina.py"]
