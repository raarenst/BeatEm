FROM ubuntu:latest
RUN apt-get update && apt-get install -y --no-install-recommends libsodium23 \
    && rm -rf /var/lib/apt/lists/*
COPY beatem_server /usr/local/bin/
ENTRYPOINT ["beatem_server"]
EXPOSE 27015/tcp
