# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & run

Two C executables — `beatem_server` and `beatem_client` — share the same source tree. Both link against **libsodium** (X25519 + XSalsa20 + Poly1305 via `crypto_box`) and carry traffic over **plain WebSocket frames** (RFC 6455, implemented in `source/ws.c` — no TLS; reverse-proxy if you need it). A third client lives in `web/` as a static HTML/JS page. The build targets POSIX directly; there is no cross-platform shim layer.

**Linux** (uses system `gcc` and `libsodium-dev`):
```
sudo apt install libsodium-dev    # one-time
cd build_linux && make all        # builds both binaries into build_linux/
make clean                        # removes obj/*.o and *.d
make remove                       # also removes the executable
```
`make clean` removes `obj/` and the built binaries.

**Windows** (`build/makefile`): currently **broken**. The Phase-3 WebSocket migration removed the cross-platform `babel/` shim and source/ws.c, source/beatem_server.c, and source/beatem_client.c now call POSIX socket APIs (`recv`/`send`/`accept`/`select`/`pthread`/`<netdb.h>`) directly. A Windows port needs winsock2 replacements, a pthread shim (MinGW pthreads-win32 or native CreateThread + condition variables), and a `<unistd.h>` stand-in. The makefile retains its libsodium plumbing for a future port but does not build today.

**Tests** live in `tests/` and run via the makefile:
```
make test         # unit tests + end-to-end smoke test
make test-unit    # just the unit tests (no server/client needed)
make test-smoke   # just the smoke test (requires `make all`)
```
`tests/test_unit.c` exercises `crypto_seal`/`crypto_open` (round-trip + 4 failure modes), `proto_handshake_encode`/`_decode` (round-trip + bad magic / bad version), hex helpers, and `proto_counter_check_and_update` (the replay-protection logic). `tests/smoke_test.sh` brings up the server plus two clients with fresh keypairs and asserts reciprocal message delivery. No linter or formatter config is checked in.

**Docker (server)**: `docker/server/Dockerfile` builds an Ubuntu image exposing port 27015. It installs `libsodium23` at runtime and copies in a pre-built `beatem_server` binary, so build the binary on a Linux host first, then drop it next to the Dockerfile before `docker build`.

**Running locally**: start the server, then run two clients with reciprocal keys:
```
beatem_client <my_secret_hex> <my_public_hex> <peer_public_hex> [server_ip]
```
Each key is 64 hex chars (32 bytes). Generate keypairs out-of-band with libsodium's `crypto_box_keypair` (or use the `&genkeys` runtime command on an already-connected client and re-share the new public key manually).

**Web client** (`web/index.html`): single static page using `libsodium-wrappers` via CDN and the browser's native `WebSocket`. Serve over plain HTTP because `ws://` from a `file://` origin is unreliable:
```
cd web && python3 -m http.server 8080
```
Then open `http://localhost:8080/`, paste hex keys (or click *Generate new keypair*), and connect. Implements the exact same wire format as the CLI client.

## Architecture

The project implements a privacy-preserving chat protocol where **every client emits fixed-size packets at a fixed cadence** and **the server broadcasts the union of all client packets to every client**. The point is to hide *who is talking to whom*, not just *what they are saying* — observers can't tell from traffic whether a packet contains real text or random padding.

Three layers, top-down:

**1. Application** (`source/`)
- `beatem_server.c` — single-threaded `select()` loop. Accepts up to `SERVER_MAX_NR_OF_CLIENTS` (16) TCP clients on port `SERVER_PORT` (27015). **Immediately after `accept()` it writes a 16-byte handshake** (see `include/protocol.h`) describing its `max_clients`, `heartbeat_ms`, and `client_packet_size` so the client can size its own buffers and align its send cadence. Every `SERVER_HEART_BEAT_S` (2s) it flushes `g_sendbuf` to all connected clients, padding any unused slots with `rand()` bytes so the broadcast is always exactly `SERVER_PACKET_SIZE` (16 × 128 = 2048 bytes). Incoming client packets are appended to `g_sendbuf` between flushes; if it fills first, it's flushed early. The server is content-agnostic — it doesn't decrypt or inspect packet bodies.
- `beatem_client.c` — three concurrent flows:
  - **Main thread**: connects, reads the server handshake, validates and allocates buffers from the negotiated sizes, then runs an stdin loop. `&genkeys` regenerates an X25519 keypair (and prints both halves; peer must be re-told). Any other line is queued via `send_buf_flag`.
  - **Send thread** (`send_thread_func`): every `g_heartbeat_ms` (from handshake) emits exactly one `g_client_packet_size`-byte packet — either the queued message sealed with `crypto_box_easy(plain, nonce, recipient_pk, sender_sk)`, or pure random bytes if no message is queued. **The constant cadence is the privacy property** — never short-circuit it.
  - **Receive thread** (`receive_thread_func`): blocks on a full `g_server_packet_size` broadcast, then tries `crypto_box_open_easy` on each of the `g_max_clients` slots with the configured peer's public key and the local secret key. Successful authentication means the slot was sealed *to* this client *by* the expected peer. The decrypted plaintext embeds the sender's public key (first 32 bytes) — if it equals this client's own pk, the slot is an echo of our own outgoing packet and is silently dropped (see [memory: project-crypto-box-symmetric-key]). The next 4 bytes are the sender's monotonic counter; packets with counter ≤ the highest already accepted are dropped as replays. Otherwise the remaining `g_text_size` bytes of text are printed.
- `crypto.c` / `include/crypto.h` — thin libsodium wrapper: `crypto_init`, `crypto_keygen`, `crypto_seal`, `crypto_open`, plus `crypto_key_to_hex` / `crypto_hex_to_key` helpers. All real crypto lives in libsodium.
- `protocol.c` / `include/protocol.h` — connect-time handshake codec. Pack/unpack a 16-byte struct over the wire so server and client agree on cadence and slot sizes without sharing compile-time constants.

**2. Wire format**

Per client packet (size negotiated; default 128 bytes):
```
[ 24-byte nonce ][ 16-byte Poly1305 MAC ][ ciphertext ]
                                          ↓ decrypts to
              [ 32-byte sender_pk ][ 4-byte counter ][ text ]
```
The 4-byte big-endian counter is monotonic per sender (initialized from
wall-clock seconds so it survives restarts). Recipients reject any
packet whose counter is `<=` the highest they've already accepted from
that peer — replay protection without server cooperation.

Connect-time handshake (16 bytes, server → client, big-endian, see `include/protocol.h`):
```
[ 4B "BEAT" ][ 2B version ][ 2B max_clients ][ 4B heartbeat_ms ][ 4B client_packet_size ]
```

**3. Protocol constants** (`include/config.h`) — the server's compile-time defaults (`CLIENT_PACKET_SIZE`, `SERVER_HEART_BEAT_S`, `SERVER_MAX_NR_OF_CLIENTS`, `SERVER_PACKET_SIZE`, `SERVER_MAX_NR_OF_PACKETS`). The client no longer reads these at runtime — it derives its own sizes from the handshake. Sodium-derived constants (`CLIENT_NONCE_SIZE`, `CLIENT_MAC_SIZE`, `CLIENT_KEY_SIZE`, `CLIENT_SECRET_SIZE`) are used by both sides since they're fixed by the crypto primitive.

**4. Transport — `source/ws.c`** — minimal RFC 6455 WebSocket framing over a connected TCP socket. Supports the HTTP upgrade (both directions, with a vendored public-domain SHA-1 for the `Sec-WebSocket-Accept` value), unmasked server→client frames, masked client→server frames, and short + u16-extended payload lengths. Plain `ws://` only; TLS belongs behind a reverse proxy.

## Conventions worth knowing

- Keys are exchanged out-of-band — clients are started on the command line with both their own keypair (secret + public) and the remote peer's public key. There is no key exchange or directory service.
- Recipient + sender identification is via **authenticated decryption** (`crypto_box_open_easy`), not header equality. A slot decrypts iff sealed *to* this client *by* the configured peer. Self-echoes (own packets reflected back by the server's broadcast) are dropped via the embedded sender-pk check.
- The **server** still uses fixed stack/global arrays sized by `config.h` constants; nothing is dynamically allocated on the server.
- The **client** dynamically allocates its packet buffers, plaintext scratch, and stdin input buffer from the handshake-negotiated sizes (`g_text_buffer`, `g_send_buffer`, `g_send_plaintext`, `g_recv_buffer`, `g_recv_plaintext`). This is the intentional exception to the "fixed arrays" style — it's what lets a client built today still work against a server whose `CLIENT_PACKET_SIZE` is bumped tomorrow.
