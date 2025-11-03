# Makefile for 1D Nesting Software - Updated for new directory structure

# -----------------
# Configuration
# -----------------

# Compiler
CXX = g++

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Target executable
TARGET = $(BIN_DIR)/nesting

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# Object files (in build directory)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# -----------------
# Build Flags
# -----------------

# Try to find HiGHS installation path
# 1. Use nix-build to find HiGHS (NixOS primary method)
HIGHS_INSTALL_PATH ?= $(shell nix-build --no-out-link '<nixpkgs>' -A highs 2>/dev/null || echo "")

# 2. Fallback to pkg-config if nix-build fails
ifeq ($(HIGHS_INSTALL_PATH),)
    HIGHS_INSTALL_PATH := $(shell pkg-config --variable=prefix highs 2>/dev/null || echo "")
endif

# 3. Check common system paths
ifeq ($(HIGHS_INSTALL_PATH),)
    ifneq ($(wildcard /usr/include/highs),)
        HIGHS_INSTALL_PATH := /usr
    else ifneq ($(wildcard /usr/local/include/highs),)
        HIGHS_INSTALL_PATH := /usr/local
    endif
endif

# 4. Final fallback to hardcoded NixOS path (update with: nix-build --no-out-link '<nixpkgs>' -A highs)
ifeq ($(HIGHS_INSTALL_PATH),)
    HIGHS_INSTALL_PATH := /nix/store/l5kdwqh5i0g5b8xvifmrm1ql5jjbi3p2-highs-1.8.0
endif

# Compiler Flags
CXXFLAGS = -m64 -std=c++17 -Wall -Wextra -O3 -DNDEBUG -Wno-reorder -Wno-unused-parameter

# Include directories - now includes our include/ directory
INCLUDES = -I$(INC_DIR) -I$(HIGHS_INSTALL_PATH)/include/highs

# Linker flags
LDFLAGS = -L$(HIGHS_INSTALL_PATH)/lib -lhighs

# -----------------
# Build Rules
# -----------------

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

# Rule to link the executable from object files
$(TARGET): $(OBJS)
	@echo "Linking executable..."
	$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo "Binary built: $(TARGET)"

# Rule to compile source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Web server target (if building separately)
web: directories
	@echo "Building web server..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) \
		$(SRC_DIR)/web_server.cpp \
		$(SRC_DIR)/parse.cpp \
		$(SRC_DIR)/algorithm.cpp \
		$(SRC_DIR)/output.cpp \
		-o $(BIN_DIR)/nesting-server \
		$(LDFLAGS) -lpthread

# -----------------
# Utility Rules
# -----------------

# Target to run the program
run: all
	./$(TARGET)

# Target to run tests
test: all
	./$(TARGET) --test

# Clean build artifacts
clean:
	@echo "Cleaning..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean complete."

# Deep clean (including any generated files)
distclean: clean
	rm -f cut_plan.html

.PHONY: all clean distclean run test directories web
