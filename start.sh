#!/bin/bash

# Quick start script for Tube Designer

set -e

echo "🚀 Tube Designer - Quick Start"
echo "=============================="

# Function to check if a command exists
command_exists() {
  command -v "$1" >/dev/null 2>&1
}

# Check for Docker Compose first (preferred method)
if command_exists docker-compose; then
  echo "✅ Docker Compose found. Starting application..."
  echo ""

  # Make sure static directory exists
  mkdir -p static

  # Stop any existing containers and remove them
  echo "Stopping and removing existing Docker Compose services (if any)..."
  docker-compose down --remove-orphans 2>/dev/null || true # Use '|| true' to prevent script from exiting if no services are running

  # Start with docker-compose
  docker-compose up --build

elif command_exists docker; then
  echo "✅ Docker found. Building and starting..."
  echo ""

  # Stop and remove existing container if it exists
  CONTAINER_NAME="tube-designer"
  if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Stopping and removing existing container: ${CONTAINER_NAME}..."
    docker stop ${CONTAINER_NAME} >/dev/null || true
    docker rm ${CONTAINER_NAME} >/dev/null || true
  fi

  # Build the image
  echo "Building Docker image 'tube-designer'..."
  docker build -t tube-designer .

  # Run the container
  echo "Running new container: ${CONTAINER_NAME}..."
  docker run -d -p 8080:8080 --name ${CONTAINER_NAME} tube-designer

  echo ""
  echo "✨ Application started!"
  echo "📱 Open http://localhost:8080 in your browser"
  echo ""
  echo "To view logs: docker logs -f ${CONTAINER_NAME}"
  echo "To stop: docker stop ${CONTAINER_NAME} && docker rm ${CONTAINER_NAME}"

else
  echo "⚠️  Neither Docker nor Make found. Attempting local build..."

  if command_exists make; then
    # Try to build with make
    make -f Makefile.web download-deps
    make -f Makefile.web

    echo ""
    echo "Starting server locally..."
    ./bin/tube-designer-server
  else
    echo "❌ Neither Docker nor Make found."
    echo ""
    echo "Please install one of the following:"
    echo "1. Docker: https://docs.docker.com/get-docker/"
    echo "2. Build tools (g++, make) for local compilation"
    exit 1
  fi
fi
