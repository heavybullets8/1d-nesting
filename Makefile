BINARY_NAME=tube-designer
MAIN_PATH=./cmd/tube-designer

# Build-time variables
VERSION ?= 0.1.0
COMMIT=$(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
BUILD_TIME=$(shell date -u +"%Y-%m-%dT%H:%M:%SZ")

# Go build flags
LDFLAGS=-ldflags "-X main.Version=$(VERSION) -X main.Commit=$(COMMIT) -X main.BuildTime=$(BUILD_TIME)"
BUILDFLAGS=-trimpath

.PHONY: all clean build build-32 build-64

all: clean build

build: build-32 build-64

build-32:
	@echo "Building 32-bit binary..."
	@mkdir -p bin
	GOARCH=386 go build $(BUILDFLAGS) $(LDFLAGS) -o bin/$(BINARY_NAME)-32 $(MAIN_PATH)
	@echo "32-bit binary built: bin/$(BINARY_NAME)-32"

build-64:
	@echo "Building 64-bit binary..."
	@mkdir -p bin
	GOARCH=amd64 go build $(BUILDFLAGS) $(LDFLAGS) -o bin/$(BINARY_NAME)-64 $(MAIN_PATH)
	@echo "64-bit binary built: bin/$(BINARY_NAME)-64"

clean:
	@echo "Cleaning..."
	@rm -rf bin/
	@echo "Clean complete"

test:
	go test -v ./...

run: build-64
	@echo "Running application..."
	./bin/$(BINARY_NAME)-64
