# Makefile for Tube-Designer

# -----------------
# Configuration
# -----------------

# Compiler
CXX = g++

# Target executable
TARGET_DIR = bin
TARGET = $(TARGET_DIR)/tube-designer

# Source files
SRCS = main.cpp parse.cpp algorithm.cpp output.cpp

# Object files (derived from source files)
OBJS = $(SRCS:.cpp=.o)

# -----------------
# Build Flags
# -----------------

# Determine the location of the HiGHS headers. If `nix-build` is available we
# use it to discover the installation path. Otherwise fall back to a locally
# vendored copy under `dependencies/highs`.
HIGHS_INSTALL_PATH ?= $(shell command -v nix-build >/dev/null 2>&1 && nix-build --no-out-link '<nixpkgs>' -A highs)
ifeq ($(HIGHS_INSTALL_PATH),)
    HIGHS_INSTALL_PATH = $(CURDIR)/dependencies/highs
endif


# Compiler Flags
# -m64: Build a 64-bit binary
# -std=c++17: Use the C++17 standard
# -Wall -Wextra: Enable most common warnings for better code quality
# -O3: Aggressive optimization for release builds
# -DNDEBUG: Disable assertions (standard for release builds)
# -Wno-reorder: Suppress warnings about member initialization order (for HiGHS library)
# -Wno-unused-parameter: Suppress warnings about unused function parameters (for HiGHS library)
CXXFLAGS = -m64 -std=c++17 -Wall -Wextra -O3 -DNDEBUG -Wno-reorder -Wno-unused-parameter

# Include directory for header files (e.g., highs/Highs.h)
# The local copy extracted under `dependencies/highs` uses `src`.
INCLUDES = -I$(HIGHS_INSTALL_PATH)/include/highs -I$(HIGHS_INSTALL_PATH)/src

# Linker flags for linking against the HiGHS library
LDFLAGS = -L$(HIGHS_INSTALL_PATH)/lib -lhighs


# -----------------
# Build Rules
# -----------------

# Default target: 'make' or 'make all' will build the executable
all: $(TARGET)

# Rule to link the executable from object files
$(TARGET): $(OBJS)
	@echo "Linking executable..."
	@mkdir -p $(TARGET_DIR) # Ensure the bin directory exists
	$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo "Binary built: $(TARGET)"

# Rule to compile a .cpp source file into a .o object file
# The '-c' flag tells the compiler to stop after compilation and not link.
%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# -----------------
# Utility Rules
# -----------------

# Target to run the program with default parameters
run: all
	./$(TARGET)

# Target to run the built-in tests
# The project currently only has parser unit tests, so avoid building the
# main binary (which requires the HiGHS library) when running tests.
# Running `make test` will build and execute the parser tests.
test: parse-test

# Build and run unit tests for parser utilities
tests/parse_test: tests/parse_test.cpp parse.cpp parse.h
	$(CXX) -std=c++17 -Wall -Wextra -I. tests/parse_test.cpp parse.cpp -o $@

parse-test: tests/parse_test
	./tests/parse_test

# Target to clean the build directory of generated files
clean:
	@echo "Cleaning..."
	rm -f $(OBJS) $(TARGET)
	@echo "Clean complete."

# Phony targets are commands that are not actual files.
# This prevents 'make' from getting confused if a file named 'clean' exists.
.PHONY: all clean run test parse-test



# Build WebAssembly version (requires Emscripten and HiGHS built for wasm)
web:
	em++ $(SRCS) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) -s WASM=1 -s EXIT_RUNTIME=1 \
	-s EXPORTED_RUNTIME_METHODS="[\"ccall\",\"cwrap\",\"FS\"]" \
	-s EXPORTED_FUNCTIONS="[\"_main\",\"_wasmSetInput\",\"_wasmGetOutput\"]" \
	-o web/tube.js

