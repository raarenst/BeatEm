# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & run

Two C executables — `beatem_server` and `beatem_client` — share the same source tree but are built per-platform from separate directories. Both link against **libsodium** (X25519 + XSalsa20 + Poly1305 via `crypto_box`).

**Linux** (uses system `gcc` and `libsodium-dev`):
```
sudo apt install libsodium-dev    # one-time
cd build_linux && make all        # builds both binaries into build_linux/
make clean                        # removes obj/*.o and *.d
make remove                       # also removes the executable
```
The `make clean` rule contains a known pre-existing bug — its `RMOBJ` substitution mangles Linux paths (`obj/foo.o` → `objfoo.o`). Rebuild after `rm -rf obj` instead.

**Windows** (MinGW-w64): `build/makefile` hard-codes a path to `C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\bin\gcc.exe`. Adjust `CC`/`LINKER` if your toolchain lives elsewhere. It also expects a MinGW-built libsodium at `LIBSODIUM_DIR` (default `C:/libsodium`, with `include/sodium.h` and `lib/libsodium.a`). `build/make.bat` runs `mingw32-make.exe -f makefile all`. **The Windows build is currently untested after the libsodium migration.**

There is no test suite, no linter config, and no formatter config. A bash-driven end-to-end smoke test lives in `/tmp/smoke_test.sh` during development — run server + two clients and verify reciprocal message delivery.

**Docker (server)**: `docker/server/Dockerfile` builds an Ubuntu image exposing port 27015. It installs `libsodium23` at runtime and copies in a pre-built `beatem_server` binary, so build the binary on a Linux host first, then drop it next to the Dockerfile before `docker build`.

**Running locally**: start the server, then run two clients with reciprocal keys:
```
beatem_client <my_secret_hex> <my_public_hex> <peer_public_hex> [server_ip]
```
Each key is 64 hex chars (32 bytes). Generate keypairs out-of-band with libsodium's `crypto_box_keypair` (or use the `&genkeys` runtime command on an already-connected client and re-share the new public key manually).

## Architecture

The project implements a privacy-preserving chat protocol where **every client emits fixed-size packets at a fixed cadence** and **the server broadcasts the union of all client packets to every client**. The point is to hide *who is talking to whom*, not just *what they are saying* — observers can't tell from traffic whether a packet contains real text or random padding.

Three layers, top-down:

**1. Application** (`source/`)
- `beatem_server.c` — single-threaded `select()` loop. Accepts up to `SERVER_MAX_NR_OF_CLIENTS` (16) TCP clients on port `SERVER_PORT` (27015). Every `SERVER_HEART_BEAT_S` (2s) it flushes `g_sendbuf` to all connected clients, padding any unused slots with `rand()` bytes so the broadcast is always exactly `SERVER_PACKET_SIZE` (16 × 128 = 2048 bytes). Incoming client packets are appended to `g_sendbuf` between flushes; if it fills first, it's flushed early. The server is content-agnostic — it doesn't decrypt or inspect packet bodies.
- `beatem_client.c` — three concurrent flows:
  - **Main thread**: stdin loop; `&genkeys` regenerates an X25519 keypair (and prints both halves; peer must be re-told), any other line is queued via `send_buf_flag`.
  - **Send thread** (`send_thread_func`): every `CLIENT_SEND_DELAY` (2000ms) emits exactly one `CLIENT_PACKET_SIZE` (128 byte) packet — either the queued message sealed with `crypto_box_easy(plain, nonce, recipient_pk, sender_sk)`, or pure random bytes if no message is queued. **The constant cadence is the privacy property** — never short-circuit it.
  - **Receive thread** (`receive_thread_func`): blocks on a full `SERVER_PACKET_SIZE` broadcast, then tries `crypto_box_open_easy` on each of the 16 slots with the configured peer's public key and the local secret key. Successful authentication means the slot was sealed *to* this client *by* the expected peer. The decrypted plaintext also embeds the sender's public key (first 32 bytes) — if it equals this client's own pk, the slot is an echo of our own outgoing packet and is silently dropped (see [memory: project-crypto-box-symmetric-key]). Otherwise the remaining 56 bytes of text are printed.
- `crypto.c` / `include/crypto.h` — thin libsodium wrapper: `crypto_init`, `crypto_keygen`, `crypto_seal`, `crypto_open`, plus `crypto_key_to_hex` / `crypto_hex_to_key` helpers. All real crypto lives in libsodium.

**2. Protocol constants** (`include/config.h`) — `CLIENT_PACKET_SIZE`, `CLIENT_NONCE_SIZE`, `CLIENT_MAC_SIZE`, `CLIENT_KEY_SIZE`, `CLIENT_SECRET_SIZE`, `CLIENT_PLAIN_SIZE`, `CLIENT_TEXT_SIZE`, `SERVER_HEART_BEAT_S`, `SERVER_MAX_NR_OF_CLIENTS`, `SERVER_PACKET_SIZE`. Tightly coupled: `SERVER_PACKET_SIZE = SERVER_MAX_NR_OF_CLIENTS × CLIENT_PACKET_SIZE`, `CLIENT_PLAIN_SIZE = CLIENT_PACKET_SIZE − NONCE − MAC`, `CLIENT_TEXT_SIZE = CLIENT_PLAIN_SIZE − CLIENT_KEY_SIZE` (the embedded sender pk). Changing one without the others will desync server and client.

**Wire format** (per client packet, exactly 128 bytes):
```
[ 24-byte nonce ][ 16-byte Poly1305 MAC ][ 88-byte ciphertext ]
                                          ↓ decrypts to
                          [ 32-byte sender_pk ][ 56-byte text ]
```

**3. Portability layer — `babel/`** — a vendored cross-platform shim. Public headers live in `babel/include/` (`babelsock.h`, `babelthread.h`, `babeltime.h`); per-OS implementations in `babel/source/linux/` and `babel/source/win64/`. The makefiles pick the right `babel/source/$(ARCH)/` directory via the `ARCH` variable (`linux` vs `win64`). When adding a babel symbol used by the app, you must implement it on **both** platforms or the other build will break. Note that some headers (`babeldl.h`, `babelsem.h`, `display.h`) are declared but only implemented on win64.

## Conventions worth knowing

- Keys are exchanged out-of-band — clients are started on the command line with both their own keypair (secret + public) and the remote peer's public key. There is no key exchange or directory service.
- Recipient + sender identification is via **authenticated decryption** (`crypto_box_open_easy`), not header equality. A slot decrypts iff sealed *to* this client *by* the configured peer. Self-echoes (own packets reflected back by the server's broadcast) are dropped via the embedded sender-pk check.
- All buffers are stack/global fixed arrays sized by the `config.h` constants; nothing is dynamically allocated. Maintain this style **for now** — Phase 2 of the implementation plan introduces protocol-negotiated sizes which will require an exception here.
