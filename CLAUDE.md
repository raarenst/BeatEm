# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & run

Two C executables — `beatem_server` and `beatem_client` — share the same source tree but are built per-platform from separate directories.

**Linux** (uses system `gcc`):
```
cd build_linux && make all          # builds both binaries into build_linux/
make clean                          # removes obj/*.o and *.d
make remove                         # also removes the executable
```

**Windows** (MinGW-w64): `build/makefile` hard-codes a path to `C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\bin\gcc.exe`. Adjust `CC`/`LINKER` if your toolchain lives elsewhere. `build/make.bat` runs `mingw32-make.exe -f makefile all`.

There is no test suite, no linter config, and no formatter config.

**Docker (server)**: `docker/server/Dockerfile` builds an Ubuntu image exposing port 27015. Note the Dockerfile currently `COPY`s `bruno_server` (legacy name) but `ENTRYPOINT`s `beatem_server` — the binary in `docker/server/` is named `beatem_server`, so the `COPY` line is stale and needs the source name fixed before the image will build.

**Running locally** (from README): start the server, then run two clients with matching key pairs:
```
beatem_client <my_d> <my_e> <my_n> <remote_e> <remote_n> [server_ip]
```
The two clients in the README use reciprocal keys so each can decrypt the other.

## Architecture

The project implements a privacy-preserving chat protocol where **every client emits fixed-size packets at a fixed cadence** and **the server broadcasts the union of all client packets to every client**. The point is to hide *who is talking to whom*, not just *what they are saying* — observers can't tell from traffic whether a packet contains real text or random padding.

Three layers, top-down:

**1. Application** (`source/`)
- `beatem_server.c` — single-threaded `select()` loop. Accepts up to `SERVER_MAX_NR_OF_CLIENTS` (16) TCP clients on port `SERVER_PORT` (27015). Every `SERVER_HEART_BEAT_S` (2s) it flushes `g_sendbuf` to all connected clients, padding any unused slots with `rand()` bytes so the broadcast is always exactly `SERVER_PACKET_SIZE` (16 × 128 = 2048 bytes). Incoming client packets are appended to `g_sendbuf` between flushes; if it fills first, it's flushed early.
- `beatem_client.c` — three concurrent flows:
  - **Main thread**: stdin loop; `&genkeys` regenerates RSA keys, any other line is queued via `send_buf_flag`.
  - **Send thread** (`send_thread_func`): every `CLIENT_SEND_DELAY` (2000ms) emits exactly one `CLIENT_PACKET_SIZE` (128 byte) packet — either the queued message (header = recipient's public key string, payload = text, all RSA-encrypted to recipient) or pure random bytes if no message is queued. **The constant cadence is the privacy property** — never short-circuit it.
  - **Receive thread** (`receive_thread_func`): blocks on a full `SERVER_PACKET_SIZE` broadcast, then tries to RSA-decrypt each of the 16 slots with the local private key; if the decrypted header matches this client's own key string (`g_my_key_str`, formatted by `get_key_str()` as `$<10-digit e>:<10-digit n>$`, exactly `CLIENT_HEADER_SIZE` = 23 bytes), the slot is for us and we print the payload.
- `crypto.c` / `include/crypto.h` — toy 16-bit RSA (`rsa_key_gen`, `rsa_encrypt`, `rsa_decrypt`). Keys fit in `uint16_t` because each plaintext byte is encrypted to a `uint16_t` ciphertext word — this is what makes `CLIENT_PACKET_SIZE = 2 × MESSAGE_LEN`. Not cryptographically secure; the project is a protocol demonstration.

**2. Protocol constants** (`include/config.h`) — `CLIENT_PACKET_SIZE`, `CLIENT_HEADER_SIZE`, `SERVER_HEART_BEAT_S`, `SERVER_MAX_NR_OF_CLIENTS`, `SERVER_PACKET_SIZE`. These are tightly coupled: `SERVER_PACKET_SIZE = SERVER_MAX_NR_OF_CLIENTS × CLIENT_PACKET_SIZE`, `MESSAGE_LEN = CLIENT_PACKET_SIZE / 2`, `TEXT_LEN = MESSAGE_LEN − CLIENT_HEADER_SIZE`. Changing one without the others will desync server and client.

**3. Portability layer — `babel/`** — a vendored cross-platform shim. Public headers live in `babel/include/` (`babelsock.h`, `babelthread.h`, `babeltime.h`); per-OS implementations in `babel/source/linux/` and `babel/source/win64/`. The makefiles pick the right `babel/source/$(ARCH)/` directory via the `ARCH` variable (`linux` vs `win64`). When adding a babel symbol used by the app, you must implement it on **both** platforms or the other build will break. Note that some headers (`babeldl.h`, `babelsem.h`, `display.h`) are declared but only implemented on win64.

## Conventions worth knowing

- Keys are exchanged out-of-band — clients are started on the command line with both their own keypair and the remote peer's public key. There is no key exchange or directory service.
- Recipient identification is by **plaintext-equality of the decrypted header**, not by any cryptographic signature. If two clients share a key prefix, both will decode the message.
- All buffers are stack/global fixed arrays sized by the `config.h` constants; nothing is dynamically allocated. Maintain this style.
