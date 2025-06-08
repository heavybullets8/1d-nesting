CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -DNDEBUG
BINARY_NAME = tube-designer

# Use pkg-config to find HiGHS
HIGHS_CFLAGS = $(shell pkg-config --cflags highs 2>/dev/null || echo "")
HIGHS_LIBS = $(shell pkg-config --libs highs 2>/dev/null || echo "-lhighs")

# If pkg-config didn't find HiGHS, fall back to standard paths
ifeq ($(HIGHS_CFLAGS),)
    HIGHS_CFLAGS = -I/usr/local/include -I/usr/include
endif

# Base flags
BASE_CXXFLAGS = $(CXXFLAGS) $(HIGHS_CFLAGS)
BASE_LDFLAGS = $(HIGHS_LIBS)

# Source files
SRCS = main.cpp parse.cpp algorithm.cpp output.cpp

# Targets
.PHONY: all clean build build-32 build-64 test help check-env

all: clean build

build: build-32 build-64

build-32:
	@echo "Building 32-bit binary..."
	@mkdir -p bin
	@if $(CXX) -m32 $(BASE_CXXFLAGS) $(SRCS) -o bin/$(BINARY_NAME)-32 $(BASE_LDFLAGS) 2>/dev/null; then \
		echo "32-bit binary built: bin/$(BINARY_NAME)-32"; \
	else \
		echo "WARNING: 32-bit build failed. Install g++-multilib and 32-bit libraries."; \
		echo "Continuing with 64-bit build only..."; \
	fi

build-64:
	@echo "Building 64-bit binary..."
	@mkdir -p bin
	$(CXX) -m64 $(BASE_CXXFLAGS) $(SRCS) -o bin/$(BINARY_NAME)-64 $(BASE_LDFLAGS)
	@echo "64-bit binary built: bin/$(BINARY_NAME)-64"

clean:
	@echo "Cleaning..."
	@rm -rf bin/
	@echo "Clean complete"

test-32: build-32
	./bin/$(BINARY_NAME)-32 --test

test-64: build-64
	./bin/$(BINARY_NAME)-64 --test

test: test-64

run: build-64
	./bin/$(BINARY_NAME)-64

help:
	@echo "Tube Designer - C++ Build for Linux"
	@echo ""
	@echo "Prerequisites:"
	@echo "  - C++17 compatible compiler with multilib support (g++-multilib)"
	@echo "  - HiGHS library (32-bit and 64-bit versions)"
	@echo "  - pkg-config"
	@echo ""
	@echo "For 32-bit builds on 64-bit system:"
	@echo "  Ubuntu: sudo apt install g++-multilib lib32stdc++6"
	@echo "  NixOS: Add gcc-multilib to your shell.nix"
	@echo ""
	@echo "Targets:"
	@echo "  make         - Build both 32-bit and 64-bit binaries"
	@echo "  make build-32 - Build 32-bit binary only"
	@echo "  make build-64 - Build 64-bit binary only"
	@echo "  make test    - Run tests with 64-bit binary"
	@echo "  make clean   - Remove build artifacts"
	@echo "  make check-env - Check build environment"
	@echo "  make debug-pkg - Debug pkg-config settings"

# Debug target to check pkg-config
debug-pkg:
	@echo "HiGHS CFLAGS: $(HIGHS_CFLAGS)"
	@echo "HiGHS LIBS: $(HIGHS_LIBS)"
	@echo "Checking pkg-config:"
	@pkg-config --list-all | grep highs || echo "HiGHS not found in pkg-config"

# Check build environment
check-env:
	@chmod +x check-env.sh 2>/dev/null || true
	@bash check-env.sh
