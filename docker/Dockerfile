# Alternative Alpine-based Dockerfile for 1D Nesting Software
# Use this if you're having network issues with Ubuntu base image

# Stage 1: Build environment
FROM alpine:3.18 AS builder

# Install build dependencies
RUN apk add --no-cache \
  build-base \
  cmake \
  git \
  wget \
  boost-dev \
  curl

# Install HiGHS from source
WORKDIR /tmp
RUN git clone https://github.com/ERGO-Code/HiGHS.git && \
  cd HiGHS && \
  git checkout v1.8.0 && \
  mkdir build && cd build && \
  cmake .. -DCMAKE_BUILD_TYPE=Release && \
  make -j$(nproc) && \
  make install

# Copy application source
WORKDIR /app
COPY src/ ./src/
COPY include/ ./include/

# Download single-header libraries
RUN wget -O include/httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h && \
  wget -O include/json.hpp https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp

# Build the web application
RUN g++ -std=c++17 -O3 -DNDEBUG \
  -I./include \
  -I/usr/local/include/highs \
  src/web_server.cpp \
  src/parse.cpp \
  src/algorithm.cpp \
  src/output.cpp \
  -o tube-designer-server \
  -L/usr/local/lib -lhighs \
  -lpthread

# Stage 2: Runtime environment
FROM alpine:3.18

# Install runtime dependencies
RUN apk add --no-cache \
  libstdc++ \
  libgomp \
  curl

# Create non-root user
RUN adduser -D -u 1000 appuser

# Copy HiGHS library
COPY --from=builder /usr/local/lib/libhighs.so* /usr/local/lib/
RUN ldconfig /usr/local/lib

# Copy the compiled application
WORKDIR /app
COPY --from=builder /app/tube-designer-server .

# Copy static files
COPY --chown=appuser:appuser static/ ./static/

# Change ownership
RUN chown -R appuser:appuser /app

# Switch to non-root user
USER appuser

# Expose port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD curl -f http://localhost:8080/api/health || exit 1

# Run the server
CMD ["./tube-designer-server"]
