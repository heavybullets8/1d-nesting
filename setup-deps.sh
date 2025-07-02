#!/bin/bash

# Script to download required single-header libraries

set -e

echo "📦 Downloading required single-header libraries..."

# Create include directory if it doesn't exist
mkdir -p include

# Download cpp-httplib
if [ ! -f include/httplib.h ]; then
  echo "Downloading cpp-httplib..."
  wget -O include/httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
  echo "✅ cpp-httplib downloaded"
else
  echo "ℹ️  cpp-httplib already exists"
fi

# Download nlohmann/json
if [ ! -f include/json.hpp ]; then
  echo "Downloading nlohmann/json..."
  wget -O include/json.hpp https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp
  echo "✅ nlohmann/json downloaded"
else
  echo "ℹ️  nlohmann/json already exists"
fi

echo ""
echo "✨ All dependencies downloaded successfully!"
echo ""
echo "You can now build the project with:"
echo "  make web       # for the web server"
echo "  make          # for the CLI tool"

# Make the script executable
chmod +x setup-deps.sh 2>/dev/null || true
