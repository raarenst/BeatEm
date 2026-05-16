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
- `beatem_server.c` — single-threaded `select()` loop. Accepts up to `SERVER_MAX_NR_OF_CLIENTS` (16) TCP clients on port `SERVER_PORT` (27015). **Immediately after `accept()` it writes a 16-byte handshake** (see `include/protocol.h`) describing its `max_clients`, `heartbeat_ms`, and `client_packet_size` so the client can size its own buffers and align its send cadence. Every `SERVER_HEART_BEAT_S` (2s) it flushes `g_sendbuf` to all connected clients, padding any unused slots with `rand()` bytes so the broadcast is always exactly `SERVER_PACKET_SIZE` (16 × 128 = 2048 bytes). Incoming client packets are appended to `g_sendbuf` between flushes; if it fills first, it's flushed early. The server is content-agnostic — it doesn't decrypt or inspect packet bodies.
- `beatem_client.c` — three concurrent flows:
  - **Main thread**: connects, reads the server handshake, validates and allocates buffers from the negotiated sizes, then runs an stdin loop. `&genkeys` regenerates an X25519 keypair (and prints both halves; peer must be re-told). Any other line is queued via `send_buf_flag`.
  - **Send thread** (`send_thread_func`): every `g_heartbeat_ms` (from handshake) emits exactly one `g_client_packet_size`-byte packet — either the queued message sealed with `crypto_box_easy(plain, nonce, recipient_pk, sender_sk)`, or pure random bytes if no message is queued. **The constant cadence is the privacy property** — never short-circuit it.
  - **Receive thread** (`receive_thread_func`): blocks on a full `g_server_packet_size` broadcast, then tries `crypto_box_open_easy` on each of the `g_max_clients` slots with the configured peer's public key and the local secret key. Successful authentication means the slot was sealed *to* this client *by* the expected peer. The decrypted plaintext also embeds the sender's public key (first 32 bytes) — if it equals this client's own pk, the slot is an echo of our own outgoing packet and is silently dropped (see [memory: project-crypto-box-symmetric-key]). Otherwise the remaining `g_text_size` bytes of text are printed.
- `crypto.c` / `include/crypto.h` — thin libsodium wrapper: `crypto_init`, `crypto_keygen`, `crypto_seal`, `crypto_open`, plus `crypto_key_to_hex` / `crypto_hex_to_key` helpers. All real crypto lives in libsodium.
- `protocol.c` / `include/protocol.h` — connect-time handshake codec. Pack/unpack a 16-byte struct over the wire so server and client agree on cadence and slot sizes without sharing compile-time constants.

**2. Wire format**

Per client packet (size negotiated; default 128 bytes):
```
[ 24-byte nonce ][ 16-byte Poly1305 MAC ][ ciphertext ]
                                          ↓ decrypts to
                          [ 32-byte sender_pk ][ text ]
```

Connect-time handshake (16 bytes, server → client, big-endian, see `include/protocol.h`):
```
[ 4B "BEAT" ][ 2B version ][ 2B max_clients ][ 4B heartbeat_ms ][ 4B client_packet_size ]
```

**3. Protocol constants** (`include/config.h`) — the server's compile-time defaults (`CLIENT_PACKET_SIZE`, `SERVER_HEART_BEAT_S`, `SERVER_MAX_NR_OF_CLIENTS`, `SERVER_PACKET_SIZE`, `SERVER_MAX_NR_OF_PACKETS`). The client no longer reads these at runtime — it derives its own sizes from the handshake. Sodium-derived constants (`CLIENT_NONCE_SIZE`, `CLIENT_MAC_SIZE`, `CLIENT_KEY_SIZE`, `CLIENT_SECRET_SIZE`) are used by both sides since they're fixed by the crypto primitive.

**4. Portability layer — `babel/`** — a vendored cross-platform shim. Public headers live in `babel/include/` (`babelsock.h`, `babelthread.h`, `babeltime.h`); per-OS implementations in `babel/source/linux/` and `babel/source/win64/`. The makefiles pick the right `babel/source/$(ARCH)/` directory via the `ARCH` variable (`linux` vs `win64`). When adding a babel symbol used by the app, you must implement it on **both** platforms or the other build will break. Note that some headers (`babeldl.h`, `babelsem.h`, `display.h`) are declared but only implemented on win64.

## Conventions worth knowing

- Keys are exchanged out-of-band — clients are started on the command line with both their own keypair (secret + public) and the remote peer's public key. There is no key exchange or directory service.
- Recipient + sender identification is via **authenticated decryption** (`crypto_box_open_easy`), not header equality. A slot decrypts iff sealed *to* this client *by* the configured peer. Self-echoes (own packets reflected back by the server's broadcast) are dropped via the embedded sender-pk check.
- The **server** still uses fixed stack/global arrays sized by `config.h` constants; nothing is dynamically allocated on the server.
- The **client** dynamically allocates its packet buffers, plaintext scratch, and stdin input buffer from the handshake-negotiated sizes (`g_text_buffer`, `g_send_buffer`, `g_send_plaintext`, `g_recv_buffer`, `g_recv_plaintext`). This is the intentional exception to the "fixed arrays" style — it's what lets a client built today still work against a server whose `CLIENT_PACKET_SIZE` is bumped tomorrow.
