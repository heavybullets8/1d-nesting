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

  # Stop any existing containers
  docker-compose down 2>/dev/null

  # Start with docker-compose
  docker-compose up --build

elif command_exists docker; then
  echo "✅ Docker found. Building and starting..."
  echo ""

  # Build the image
  docker build -t tube-designer .

  # Run the container
  docker run -d -p 8080:8080 --name tube-designer tube-designer

  echo ""
  echo "✨ Application started!"
  echo "📱 Open http://localhost:8080 in your browser"
  echo ""
  echo "To view logs: docker logs -f tube-designer"
  echo "To stop: docker stop tube-designer && docker rm tube-designer"

else
  echo "⚠️  Docker not found. Attempting local build..."

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
