# Use Ubuntu Plucky (25.04) as the base image, where HiGHS packages are available
FROM ubuntu:25.10 AS builder

# Set frontend noninteractive to prevent prompts during apt install
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
  build-essential \
  cmake \
  git \
  wget \
  pkg-config \
  ca-certificates \
  libhighs-dev \
  libhighs1 \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY src/ ./src/
COPY include/ ./include/

RUN wget -O include/httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
RUN wget -O include/json.hpp https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp

RUN g++ -std=c++17 -O3 -DNDEBUG \
  -I./include \
  -I/usr/include/highs \
  src/web_server.cpp \
  src/parse.cpp \
  src/algorithm.cpp \
  src/output.cpp \
  -o nesting-server \
  -L/usr/lib -lhighs \
  -lpthread

FROM ubuntu:25.10

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
  libhighs1 \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/nesting-server .

COPY static/ ./static/

EXPOSE 8080

CMD ["./nesting-server"]
