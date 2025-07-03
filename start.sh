#!/bin/bash

# Quick start script for 1D Nesting Software

set -e

echo "🚀 1D Nesting Software - Quick Start"
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
  # Use -f to specify the docker-compose file location
  docker-compose -f docker/docker-compose.yml down --remove-orphans 2>/dev/null || true

  # Start with docker-compose from the docker directory
  docker-compose -f docker/docker-compose.yml up --build

elif command_exists docker; then
  echo "✅ Docker found. Building and starting..."
  echo ""

  # Stop and remove existing container if it exists
  CONTAINER_NAME="one-d-nesting"
  if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Stopping and removing existing container: ${CONTAINER_NAME}..."
    docker stop ${CONTAINER_NAME} >/dev/null || true
    docker rm ${CONTAINER_NAME} >/dev/null || true
  fi

  # Build the image using the Dockerfile in the docker directory
  echo "Building Docker image 'one-d-nesting'..."
  docker build -t one-d-nesting -f docker/Dockerfile .

  # Run the container
  echo "Running new container: ${CONTAINER_NAME}..."
  docker run -d -p 8080:8080 --name ${CONTAINER_NAME} one-d-nesting

  echo ""
  echo "✨ Application started!"
  echo "📱 Open http://localhost:8080 in your browser"
  echo ""
  echo "To view logs: docker logs -f ${CONTAINER_NAME}"
  echo "To stop: docker stop ${CONTAINER_NAME} && docker rm ${CONTAINER_NAME}"

else
  echo "⚠️  Docker not found!"
  echo ""
  echo "You can still build locally with Make:"
  echo "  make web"
  echo "  ./bin/nesting-server"
  echo ""
  echo "Or install Docker: https://docs.docker.com/get-docker/"
  exit 1
fi
