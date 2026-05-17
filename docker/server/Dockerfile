# syntax=docker/dockerfile:1.7
#
# Multi-stage build for beatem_server. Build from the repo root:
#
#   docker build -f docker/server/Dockerfile -t beatem-server .
#   docker run --rm -p 27015:27015 beatem-server
#
# To use a config file, bind-mount it and pass --config:
#
#   docker run --rm -p 27015:27015 \
#     -v "$PWD/beatem_server.conf:/etc/beatem/server.conf:ro" \
#     beatem-server --config /etc/beatem/server.conf
#
# Multi-arch (amd64 + arm64):
#
#   docker buildx build --platform linux/amd64,linux/arm64 \
#     -f docker/server/Dockerfile -t beatem-server .

# ----- Builder stage -----
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        libsodium-dev \
        make \
        xxd \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
# Copy only what the build needs. Source, headers, web assets, makefile.
COPY include/ ./include/
COPY source/  ./source/
COPY web/     ./web/
COPY build_linux/makefile ./build_linux/makefile

# Build only the server (skip CLI client and tests).
RUN cd build_linux && make ./beatem_server

# ----- Runtime stage -----
FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
        libsodium23 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build_linux/beatem_server /usr/local/bin/beatem_server

EXPOSE 27015/tcp
ENTRYPOINT ["beatem_server"]
