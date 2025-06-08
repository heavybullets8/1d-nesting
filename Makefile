# Makefile for Tube Designer (64-bit only)

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -DNDEBUG
BINARY_NAME = tube-designer

# Use pkg-config to find the HiGHS library.
# The '2>/dev/null' suppresses errors if pkg-config fails or isn't installed.
HIGHS_CFLAGS = $(shell pkg-config --cflags highs 2>/dev/null || echo "")
HIGHS_LIBS = $(shell pkg-config --libs highs 2>/dev/null || echo "-lhighs")

# If pkg-config did not find HiGHS, fall back to common system paths.
ifeq ($(HIGHS_CFLAGS),)
    HIGHS_CFLAGS = -I/usr/local/include -I/usr/include
endif

# Combine base flags with HiGHS-specific flags.
BASE_CXXFLAGS = $(CXXFLAGS) $(HIGHS_CFLAGS)
BASE_LDFLAGS = $(HIGHS_LIBS)

# Source code files
SRCS = main.cpp parse.cpp algorithm.cpp output.cpp

# Phony targets don't represent files and are always executed.
.PHONY: all clean build test run help check-env debug-pkg

# Default target: clean old artifacts and build the new binary.
all: clean build

# Build the 64-bit binary.
build:
	@echo "Building 64-bit binary..."
	@mkdir -p bin
	$(CXX) -m64 $(BASE_CXXFLAGS) $(SRCS) -o bin/$(BINARY_NAME) $(BASE_LDFLAGS)
	@echo "Binary built: bin/$(BINARY_NAME)"

# Remove all build artifacts.
clean:
	@echo "Cleaning..."
	@rm -rf bin/
	@echo "Clean complete."

# Build and run the test suite.
test: build
	@echo "Running tests..."
	./bin/$(BINARY_NAME) --test

# Build and run the application.
run: build
	./bin/$(BINARY_NAME)

# Display a helpful message with instructions and available targets.
help:
	@echo "Tube Designer - C++ Build for Linux (64-bit)"
	@echo ""
	@echo "Prerequisites:"
	@echo "  - A C++17 compatible compiler (e.g., g++)"
	@echo "  - The HiGHS optimization library (64-bit)"
	@echo "  - pkg-config (recommended)"
	@echo ""
	@echo "Available Targets:"
	@echo "  make         - Clean and build the binary (default)."
	@echo "  make build   - Build the binary."
	@echo "  make test    - Build and run tests."
	@echo "  make run     - Build and run the application."
	@echo "  make clean   - Remove all build artifacts."
	@echo "  make check-env - Check the build environment."
	@echo "  make debug-pkg - Show the flags found by pkg-config for debugging."

# A target to help debug pkg-config issues with HiGHS.
debug-pkg:
	@echo "--- Debugging pkg-config ---"
	@echo "HiGHS CFLAGS: $(HIGHS_CFLAGS)"
	@echo "HiGHS LIBS:   $(HIGHS_LIBS)"
	@echo "Checking pkg-config output:"
	@pkg-config --list-all | grep highs || echo "NOTE: HiGHS was not found by pkg-config."

# Check the build environment using an external script.
check-env:
	@if [ -f check-env.sh ]; then \
		chmod +x check-env.sh; \
		bash check-env.sh; \
	else \
		echo "check-env.sh not found."; \
	fi
