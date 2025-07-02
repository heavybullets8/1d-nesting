# Multi-stage build for tube-designer

# Stage 1: Build environment
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
  build-essential \
  cmake \
  git \
  wget \
  pkg-config \
  libboost-all-dev \
  && rm -rf /var/lib/apt/lists/*

# Install HiGHS from source
WORKDIR /tmp
RUN git clone https://github.com/ERGO-Code/HiGHS.git && \
  cd HiGHS && \
  git checkout v1.8.0 && \
  mkdir build && cd build && \
  cmake .. -DCMAKE_BUILD_TYPE=Release && \
  make -j$(nproc) && \
  make install && \
  ldconfig

# Copy application source
WORKDIR /app
COPY src/*.cpp ./src/
COPY include/*.h ./include/

# Download single-header libraries directly where we need them
RUN wget -O httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h && \
  wget -O json.hpp https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp

# Build the web application
RUN g++ -std=c++17 -O3 -DNDEBUG \
  -Iinclude \
  -I/usr/local/include/highs \
  src/web_server.cpp src/parse.cpp src/algorithm.cpp src/output.cpp \
  -o tube-designer-server \
  -L/usr/local/lib -lhighs \
  -lpthread

# Stage 2: Runtime environment
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
  libgomp1 \
  curl \
  && rm -rf /var/lib/apt/lists/*

# Copy HiGHS library
COPY --from=builder /usr/local/lib/libhighs.so* /usr/local/lib/
RUN ldconfig

# Copy the compiled application
WORKDIR /app
COPY --from=builder /app/tube-designer-server .

# Copy static files (frontend)
COPY static ./static

# Expose port
EXPOSE 8080

# Run the server
CMD ["./tube-designer-server"]
