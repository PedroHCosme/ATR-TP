# Use Ubuntu as base image
FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies, Boost, MQTT and JSON libraries
RUN apt-get update && apt-get install -y \
    build-essential \
    libboost-all-dev \
    mosquitto \
    mosquitto-clients \
    libmosquitto-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy project files
COPY . .

# Clean any existing build artifacts and build the project
RUN make clean && make

# Set the entrypoint to run the application
CMD ["./bin/app"]
